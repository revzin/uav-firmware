#ifndef __GPTICK_H__
#define __GPTICK_H__

/* GPTICK - TIM6 */ 

int GPTICK_Setup(int prescale, int val, void (*Callback)(void));

void GPTICK_EnableTIM6Interrupt(void);

void GPTICK_DisableTIM6Interrupt(void);

void GPTICK_TIM6_InterruptHandler(void);

#endif 
