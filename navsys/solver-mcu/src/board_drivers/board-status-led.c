#include "stm32f4xx.h"
#include "stm32f4xx_hal_rcc.h"

/* управление светодиодом на PA7 */

/* подготовка светодиода */
void BRD_InitStatusLedControl()
{
	__GPIOA_CLK_ENABLE();
	__GPIOA_RELEASE_RESET();
	
	/* PA7 - в режим Push-Pull Output */
	SET_BIT(GPIOA->MODER, GPIO_MODER_MODER7_0);
	CLEAR_BIT(GPIOA->MODER, GPIO_MODER_MODER7_1);
	
	/* средняя скорость работы */
	SET_BIT(GPIOA->OSPEEDR, GPIO_OSPEEDER_OSPEEDR7_0);
	CLEAR_BIT(GPIOA->OSPEEDR, GPIO_OSPEEDER_OSPEEDR7_1);
}

void BRD_StatusLedOn()
{
	SET_BIT(GPIOA->BSRRL, GPIO_BSRR_BS_7);
}

void BRD_StatusLedOff()
{
	SET_BIT(GPIOA->BSRRL, GPIO_BSRR_BR_7);
	CLEAR_BIT(GPIOA->ODR, GPIO_ODR_ODR_7);
}


int BRD_IsStatusLedOn()
{
	return READ_BIT(GPIOA->ODR, GPIO_ODR_ODR_7);
}

void BRD_ToggleStatusLed(void)
{
	if (BRD_IsStatusLedOn())
		BRD_StatusLedOff();
	else
		BRD_StatusLedOn();
}
