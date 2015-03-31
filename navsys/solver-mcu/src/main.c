#include "stm32f4xx.h"

#include "board-clocks.h"
#include "board-status-led.h"

/* Точка входа микроконтроллера-решателя */

int main(void) 
{
	/* настраиваем часы на 168 МГц */
	BRD_SetupMainClock();
	
	/* */
	BRD_InitStatusLedControl();
	
	BRD_StatusLedOn();
	BRD_MCO_Enable();
	
	BRD_ClockFreqs a;
	
	BRD_GetClockFrequences(&a);
	
	for (;;) {

	}
	
	//return 0;
}

