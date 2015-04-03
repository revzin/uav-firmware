#include "stm32f4xx.h"

#include "board-clocks.h"

#include "mpu9250.h"
#include "ublox.h"

/* Точка входа микроконтроллера-сборщика данных */

int main(void) 
{
	/* настраиваем часы на 168 МГц */
	BRD_SetupMainClock();
	
	BRD_ClockFreqs cf;
	
	BRD_GetClockFrequences(&cf);
	
	UB_MCUConfig();
	
	

	
	//UB_SendPollHWConf();
	for (;;) {
		//UB_SendPollHWConf();
		//for (int i = 0; i < 10000; ++i) {};

		for (int i = 0; i < 255; ++i) {
			UART5->DR = i;
		}
	}
	return 0;
}
