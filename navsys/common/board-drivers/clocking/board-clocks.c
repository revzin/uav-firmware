#include <stdint.h>
#include <limits.h>

#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include "system_stm32f4xx.h"

#include "board-clocks.h"

/* На плате установлен часовой генератор KC2520C26.0000C2LE00@AVX на 26 МГц. */

/** Устанавливает основные часы так:
 * 		Участок 	| 	Частота, МГц
 *    	Вход					26
 *		Предделитель  			1
 *		ФАПЧ					336
 *		AHB						168
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
                         7);  	/* делитель шины часов USB */

    /* резонатор (26 МГц) -> делитель (1 МГц) -> ФАПЧ (336 МГц) 	-> AHB (168 МГц) */
    /* 																-> USB (48 МГц)  */

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
    /* not implemented yet */
}


void rcc_reset(void)
{
    RCC->CR |= (uint32_t)0x00000001;

    /* Reset CFGR register */
    RCC->CFGR = 0x00000000;

    /* Reset HSEON, CSSON and PLLON bits */
    RCC->CR &= (uint32_t)0xFEF6FFFF;

    /* Reset PLLCFGR register */
    RCC->PLLCFGR = 0x24003010;

    /* Reset HSEBYP bit */
    RCC->CR &= (uint32_t)0xFFFBFFFF;

    /* Disable all interrupts */
    RCC->CIR = 0x00000000;

    /* ожидаем сброса */
    while (!(RCC->CFGR & RCC_CFGR_SWS));
}
