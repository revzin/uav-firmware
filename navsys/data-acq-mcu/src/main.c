#include "stm32f4xx.h"

#include "board-clocks.h"

#include "mpu9250.h"

/* Точка входа микроконтроллера-сборщика данных */

int main(void) 
{
	/* настраиваем часы на 168 МГц */
	BRD_SetupMainClock();

	IMU_2_Setup(256);
	CLEAR_BIT(SPI4->CR2, SPI_CR2_TXEIE);
	IMU_2_ReadCommand(RA_WHO_AM_I);
	
	for (;;) {
		
		//IMU_2_ReadCommand(RA_WHO_AM_I);
		//for (volatile unsigned int a = 0; a < 1000000; ++a);
	}
	return 0;
}
