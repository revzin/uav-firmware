#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include "gptick.h"

/* GPTICK - TIM6 */ 

int GPTICK_Setup(int prescale, int val)
{
	if (prescale == 1) 
		return 1;
	
	__TIM6_CLK_ENABLE();
	
	TIM6->PSC = prescale - 1;
	TIM6->ARR = val;
	SET_BIT(TIM6->CR1, TIM_CR1_ARPE);
	
	SET_BIT(TIM6->DIER, TIM_DIER_UIE);
	
	return 0;
}

void GPTICK_EnableTIM6Interrupt()
{
	SET_BIT(TIM6->CR1, TIM_CR1_CEN);
	NVIC_EnableIRQ(TIM6_DAC_IRQn);
}

void GPTICK_DisableTIM6Interrupt()
{
	CLEAR_BIT(TIM6->CR1, TIM_CR1_CEN);
	NVIC_DisableIRQ(TIM6_DAC_IRQn);
}

