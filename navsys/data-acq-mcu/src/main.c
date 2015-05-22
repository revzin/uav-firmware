#include "stm32f4xx.h"

#include "board-clocks.h"
#include "ublox_rmc_parser.h"
#include "gptick.h"
#include "ownassert.h"
#include "mcu_common_types.h"
#include "mcu_comm_bus.h"

void send_navdata(void);

/* Точка входа микроконтроллера-сборщика данных */
int main(void) 
{	
	/* настраиваем часы на 168 МГц */
	BRD_SetupMainClock();
	BRD_ClockFreqs cf;
	BRD_GetClockFrequences(&cf);
	
	/* запускаем КА парсинга RMC-строчек */
	RMC_Enable();
	
	MCB_SetupBUS1();
	
	/* 1 КГц - часы таймера, считаем до 500, 2 Гц - прерывание */
	int prescale = 500 * cf.abp1_tim_clk;
	
	
	GPTICK_Setup(prescale, 1000, send_navdata);
	GPTICK_EnableTIM6Interrupt();
	
	for (;;) {
		__NOP();
	}
}


void send_navdata(void)
{
	static PackedNavdata tx;
	RMC_FixTime fxt;
	RMC_GetFixTime(&fxt);
	
	tx.lat = RMC_GetLat();
	tx.lon = RMC_GetLon();
	tx.status = RMC_GetStatus();
	tx.timeh = fxt.h;
	tx.timem = fxt.m;
	tx.times = fxt.s;
	
	/* Передача */
	MCB_TxBUS1(&tx, sizeof(PackedNavdata));
}
	


