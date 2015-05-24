#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include "gptick.h"
#include "ownassert.h"

/* GPTICK - TIM6 */ 

void (*g_callback)(void);

int GPTICK_Setup(int prescale, int val, void (*callback)(void), unsigned char priority)
{
	if (prescale == 1) 
		return 1;
	
	__TIM6_CLK_ENABLE();
	
	assert(prescale > 0 && prescale < 65536, "Bad timer prescaler value");
	assert(prescale > 0 && val < 65536, "Bad timer auto-reload value");
	
	g_callback = callback;
	
	TIM6->PSC = prescale - 1;
	TIM6->ARR = val;
	SET_BIT(TIM6->CR1, TIM_CR1_ARPE);
	SET_BIT(TIM6->DIER, TIM_DIER_UIE);
	
	NVIC_SetPriority(TIM6_DAC_IRQn, priority);
	
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

void GPTICK_TIM6_InterruptHandler(void)
{
	CLEAR_BIT(TIM6->SR, TIM_SR_UIF);
	g_callback();
}

