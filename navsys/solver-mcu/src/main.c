#include "stm32f4xx.h"

#include "board-clocks.h"
#include "board-status-led.h"

#include "mcu_common_types.h"
#include "mcu_comm_bus.h"
#include "string.h"


void NavdataRecieved(void);

PackedNavdata g_rxnavdata;

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
	
	MCB_SetupBUS1();
	MCB_SetupRxBUS1(&g_rxnavdata, sizeof(PackedNavdata), NavdataRecieved);
	
	
	for (;;) {
		__NOP();
	}
	
}

/* Коллбек, когда приходят навигационные данные */
void NavdataRecieved(void)
{
	__NOP();
}
