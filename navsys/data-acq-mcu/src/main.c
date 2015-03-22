#include "stm32f4xx.h"

#include "board-clocks.h"

/* Точка входа микроконтроллера-сборщика данных */

int main(void) 
{
	/* настраиваем часы на 168 МГц */
	BRD_SetupMainClock();
	
	
	for (;;) {

	}
	return 0;
}
