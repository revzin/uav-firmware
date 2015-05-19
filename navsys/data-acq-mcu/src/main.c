#include "stm32f4xx.h"

#include "board-clocks.h"

#include "ublox_rmc_parser.h"

#include "gptick.h"

/* Точка входа микроконтроллера-сборщика данных */

int main(void) 
{
	float lat, lon;
	RMC_Status_n stat;
	
	/* настраиваем часы на 168 МГц */
	BRD_SetupMainClock();
	
	BRD_ClockFreqs cf;
	
	BRD_GetClockFrequences(&cf);
	
	/* запускаем КА парсинга RMC-строчек */
	RMC_Enable();
	

	for (;;) {
		__NOP();
		stat = RMC_GetStatus();
		lat = RMC_GetLat();
		lon = RMC_GetLat();
		
		if (stat == POWER_UP)
			lat += lon;
		
	}
	
	return 0;
}
