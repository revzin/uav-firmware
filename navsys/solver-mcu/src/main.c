#include "stm32f4xx.h"

#include "board-clocks.h"
#include "board-status-led.h"

#include "mcu_common_types.h"
#include "mcu_comm_bus.h"

void NavdataRecieved(void);

PackedNavdata g_rxnavdata;

/* Точка входа микроконтроллера-решателя */
int main(void) 
{
	/* настраиваем часы на 168 МГц */
	BRD_SetupMainClock();
	
	/* настраиваем порт светодиода */
	BRD_InitStatusLedControl();
	
	/* зажигаем светодиод */
	BRD_StatusLedOn();
	
	/* настраиваем USART2 для приёма */
	MCB_SetupBUS1();
	MCB_SetupRxBUS1(&g_rxnavdata, sizeof(PackedNavdata), NavdataRecieved);
	
	for (;;) {
		__NOP();
	}
	
}

/* Коллбек, когда приходят навигационные данные */
void NavdataRecieved(void)
{
	BRD_ToggleStatusLed();
	__NOP();
}
