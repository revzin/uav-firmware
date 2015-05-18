#include "stm32f4xx.h"

#include "board-clocks.h"

#include "mpu9250.h"
#include "ublox_rmc_parser.h"

#include "gptick.h"

/* Точка входа микроконтроллера-сборщика данных */

int main(void) 
{
	/* настраиваем часы на 168 МГц */
	BRD_SetupMainClock();
	
	BRD_ClockFreqs cf;
	
	BRD_GetClockFrequences(&cf);
	
	/* запускаем КА парсинга RMC-строчек */
	RMC_Enable();
	

	for (;;) {
		__NOP();
	}
	return 0;
}
