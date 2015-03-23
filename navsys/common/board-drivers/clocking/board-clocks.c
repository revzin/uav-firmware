#include <stdint.h>
#include <limits.h>

#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include "system_stm32f4xx.h"

#include "board-clocks.h"
#include "ownassert.h"

/* На плате установлен часовой генератор KC2520C26.0000C2LE00@AVX на 26 МГц. */

/** Устанавливает основные часы так:
 * 		Участок 	| 	Частота, МГц
 *		Вход			26
 *		Предделитель	1
 *		ФАПЧ			336
 *		AHB				168
 */

/* сброс подсистемы часов в стандартное состояние */
void rcc_reset(void);

int BRD_SetupMainClock(void)
{
	const unsigned int nops = 20000; /* количество NOPов, чтобы прождать примерно 20 мс на стандартных
									  * HSI-часах после сброса */
	volatile unsigned int a = 0; /* volatile - чтобы компилятор не выкинул NOPы при оптимизации */

	/* сброс системы тактирования в стандартное исходное состояние */
	rcc_reset();

	/* включаем тактирование регулятора управления питанием */
	__PWR_CLK_ENABLE();

	/* регулятор управления питания в режим высокого энергопотребления */
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

	/* настраиваем ФАПЧ */
	__HAL_RCC_PLL_CONFIG(RCC_PLLSOURCE_HSE, /* внешний резонатор для ФАПЧ */
						26, 	/* входной делитель ФАПЧ равен номиналу резонатора в МГц */
						336, 	/* множитель ФАПЧ */
						2, 	 	/* делитель основной шины часов AHB */
						7);		/* делитель шины часов USB */

	/* резонатор (26 МГц) -> делитель (1 МГц) -> ФАПЧ (336 МГц) 	-> AHB (168 МГц) */
	/* 																-> USB (48 МГц)	*/

	/* настраиваем делители шин */
	CLEAR_BIT(RCC->CFGR, RCC_CFGR_HPRE_3);
	/* AHB без делителя */
	MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE1_1, RCC_CFGR_PPRE1_0 | RCC_CFGR_PPRE1_2);
	/* APB1 с делителем 4 */
	MODIFY_REG(RCC->CFGR, RCC_CFGR_PPRE2_1 | RCC_CFGR_PPRE2_0, RCC_CFGR_PPRE2_2);
	/* APB2 с делителем 2 */

	/* включаем внеший осциллятор и ФАПЧ */
	__HAL_RCC_HSE_CONFIG(RCC_HSE_ON);
	__HAL_RCC_PLL_ENABLE();

	/* ждём готовности ФАПЧ и внешнего резонатора */
	while (!(READ_BIT(RCC->CR, RCC_CR_PLLRDY) && READ_BIT(RCC->CR, RCC_CR_HSERDY))) {};

	/* включаем кэширование инструкций и данных */
	SET_BIT(FLASH->ACR, FLASH_ACR_ICEN | FLASH_ACR_DCEN);

	/* особо важно: по табл. 10 Main Reference Manual задаём количество состояний ожидания FLASH-памяти, */
	/* если это не сделать, память нормально читаться перестанет и МК будет вести себя очень забавно		 */
	SET_BIT(FLASH->ACR, FLASH_ACR_LATENCY_5WS);

	/* включаем ФАПЧ в системную шину часов */
	MODIFY_REG(RCC->CFGR, RCC_CFGR_SW_0, RCC_CFGR_SW_1);

	/* ждём, пока не включилось (в битах статуса должно зажечься RCC_CFGR_SWS = 0x08) */
	while (0x08 != (RCC->CFGR & RCC_CFGR_SWS)) {
		++a;
		__NOP();
		if (a == nops) /* часы не запустились */
			return 1;
	};
	return 0;
}

void BRD_GetClockFrequences(BRD_ClockFreqs *a)
{	
	/* читаем содержимое битов RCC_CFGR_SWS */
	int sysclk_source = READ_BIT(RCC->CFGR, RCC_CFGR_SWS) >> POSITION_VAL(RCC_CFGR_SWS);

	/* табличка делителей APB (биты RCC_CFGR_PPRE) 	*/ 
	/* 										000 	001 	010 	011 	100 	101		110		111 */
	static const int apb_div_lookup[] = {	1,		1,		1,		1,		2,		4,		8,		16};
	
	/* табличка делителей AHB (биты RCC_CFGR_HPRE) 	*/ 
	static const int ahb_div_lookup[] = {
		/* 0000 */ 1,
		/* 0001 */ 1,
		/* 0010 */ 1,
		/* 0011 */ 1,
		/* 0100 */ 1,
		/* 0101 */ 1,
		/* 0110 */ 1,
		/* 0111 */ 1,
		/* 1000 */ 2,
		/* 1001 */ 4,
		/* 1010 */ 8,
		/* 1011 */ 16,
		/* 1100 */ 64,
		/* 1101 */ 128,
		/* 1110 */ 256,
		/* 1111 */ 512
	};
	
	
	/* биты RCC_CFGR_PLLP */		/*	00	01	10	11 */
	static const int pllp_lookup[] = {	2, 	4, 	6, 	8	};
	
	a->ahb_clk = a->apb1_clk = a->apb2_clk = a->sysclk = a->usb_clk = 0.0f;
	
	switch (sysclk_source) {
		case 0x00: /* HSI */
		{
			a->sysclk = HSI_MHZ;
			break;
		}
		case 0x01: /* HSE */
		{
			a->sysclk = HSE_MHZ;
			break;
		}
		case 0x02: /* PLL */
		{
			short pll_mult = READ_BIT(RCC->PLLCFGR, RCC_PLLCFGR_PLLN) 
										>> POSITION_VAL(RCC_PLLCFGR_PLLN);
			short pll_div_sysclk = pllp_lookup[READ_BIT(RCC->PLLCFGR, RCC_PLLCFGR_PLLP) 
										>> POSITION_VAL(RCC_PLLCFGR_PLLP)];
			short pll_div_input = READ_BIT(RCC->PLLCFGR, RCC_PLLCFGR_PLLM) 
										>> POSITION_VAL(RCC_PLLCFGR_PLLM);
			short pll_div_usbee = READ_BIT(RCC->PLLCFGR, RCC_PLLCFGR_PLLQ) 
										>> POSITION_VAL(RCC_PLLCFGR_PLLQ);
	
			float pll_out;
			if (READ_BIT(RCC->PLLCFGR, RCC_PLLCFGR_PLLSRC)) {
				/* ФАПЧ питается от HSE */
				pll_out = HSE_MHZ / pll_div_input * pll_mult;
			} 
			else {
				/* ФАПЧ питается от HSI */
				pll_out = HSI_MHZ / pll_div_input * pll_mult;
			}
			
			a->sysclk = pll_out / pll_div_sysclk;
			a->usb_clk = pll_out / pll_div_usbee;
			break;
		}
		case 0x03: /* WTF */
		{
			assert(0, "WTF, impossible on this hardware");
		}
	}
	
	short ahb_div = ahb_div_lookup[READ_BIT(RCC->CFGR, RCC_CFGR_HPRE) >> POSITION_VAL(RCC_CFGR_HPRE)];
	short apb1_div = apb_div_lookup[READ_BIT(RCC->CFGR, RCC_CFGR_PPRE1) >> POSITION_VAL(RCC_CFGR_PPRE1)];
	short apb2_div = apb_div_lookup[READ_BIT(RCC->CFGR, RCC_CFGR_PPRE2) >> POSITION_VAL(RCC_CFGR_PPRE2)];
	
	a->ahb_clk = a->sysclk / ahb_div;
	a->apb1_clk = a->ahb_clk / apb1_div;
	a->apb2_clk = a->ahb_clk / apb2_div;
	a->abp1_tim_clk = a->apb1_clk * 2.0f;
	a->abp2_tim_clk = a->apb2_clk * 2.0f;
}


void rcc_reset(void)
{
	RCC->CR |= (uint32_t) 0x00000001;

	/* Reset CFGR register */
	RCC->CFGR = 0x00000000;

	/* Reset HSEON, CSSON and PLLON bits */
	RCC->CR &= (uint32_t) 0xFEF6FFFF;

	/* Reset PLLCFGR register */
	RCC->PLLCFGR = 0x24003010;

	/* Reset HSEBYP bit */
	RCC->CR &= (uint32_t) 0xFFFBFFFF;

	/* Disable all interrupts */
	RCC->CIR = 0x00000000;

	/* ожидаем сброса */
	//while (!(RCC->CFGR & RCC_CFGR_SWS));
}


void assert_sdio(void)
{
	assert(!(READ_BIT(RCC->APB2ENR, RCC_APB2ENR_SDIOEN)), "Fatal: Cannot enable MCO while SDIO is in operation");
}

/* Включает выход MCO2 на МК (PC9), при этом SYSCLK делится на 4 */
void BRD_MCO_Enable(void)
{
	/* проверяем, что SDIO не в работе */
	assert_sdio();
	
	/* Даём часы на PC */
	__GPIOC_CLK_ENABLE();
	__GPIOC_RELEASE_RESET();
	
	/* Режим порта в Alternate Function */
	SET_BIT(GPIOC->MODER, GPIO_MODER_MODER9_1);
	CLEAR_BIT(GPIOC->MODER, GPIO_MODER_MODER9_0);
	
	/* Скорость порта в высокую */
	SET_BIT(GPIOC->OSPEEDR, GPIO_OSPEEDER_OSPEEDR9_1 | 
								GPIO_OSPEEDER_OSPEEDR9_0);
	
	/* Выход делим на 4 */
	MODIFY_REG(RCC->CFGR, RCC_CFGR_MCO2PRE_0, 
							RCC_CFGR_MCO2PRE_1 | RCC_CFGR_MCO2PRE_2);
	
	/* Источник часов -- PLL (SYSCLK) */
	CLEAR_BIT(RCC->CFGR, RCC_CFGR_MCO2);
		
	return;
}


/* Выключает */
void BRD_MCO_Disable(void)
{
	assert_sdio();
	/* выключаем AFIO */
	CLEAR_BIT(GPIOC->MODER, GPIO_MODER_MODER9);
}
