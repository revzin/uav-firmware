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
	GPTICK_Setup(prescale, 2000, send_navdata, 10);
	GPTICK_EnableTIM6Interrupt();
	
	/* важно: приоритет прерывания GPTICK должен быть ниже, чем у 
	 * прерывания КА парсинга. иначе происиходит такой сценарий:
	 * 1) шёл парсинг, вызвался threadlock() 
	 * 2) между приёмом букв срабатывает GPTICK 
	 * 3) GPTICK теперь ждёт вызова threadunlock()
	 * 4) КА парсинга не может перехватить управление у 
	 *    прерывания с тем же приоритетом, чтобы вызывать threadunlock()
	 * 5) deadlock
	*/
	
	
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
	tx.hdop = RMC_GetHDOP();
	tx.numsat = RMC_GetNumSat();
	tx.alt = RMC_GetASL();
	
	/* Передача на главный МК */
	MCB_TxBUS1(&tx, sizeof(PackedNavdata));
}
	


