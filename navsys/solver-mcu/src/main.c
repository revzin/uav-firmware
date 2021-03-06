#include "stm32f4xx.h"

#include "board-clocks.h"
#include "board-status-led.h"

#include "mcu_common_types.h"
#include "mcu_comm_bus.h"
#include "tm_stm32f4_usb_vcp.h"

#include "string.h" // memcpy


/* JSON-отчёт о навигации */
#define JSON_STRING "{\"navdata\": {\n"			\
					"    \"numsat\" : \"%d\",\n"\
					"    \"status\" : \"%s\",\n"\
					"    \"time\": {\n"			\
					"        \"h\": %d,\n"		\
					"        \"m\": %d,\n"		\
					"        \"s\": %d\n "		\
					"   },\n"					\
					"   \"lat\": %.6f,\n"		\
					"   \"lon\": %.6f,\n"		\
					"   \"height\": %.6f,\n"	\
					"   \"hdop\": %.6f\n"		\
					"}}\n"

void NavdataRecieved(void);

PackedNavdata g_rxnavdata;

/* Точка входа микроконтроллера-решателя */
int main(void) 
{
	/* настраиваем часы на 168 МГц */
	BRD_SetupMainClock();
	
	BRD_ClockFreqs cf;
	BRD_GetClockFrequences(&cf);
	
	/* запускаем USB-драйвер */
	TM_USB_VCP_Init();
	TM_USB_VCP_Result a = TM_USB_VCP_GetStatus();
	
	/* настраиваем порт светодиода */
	BRD_InitStatusLedControl();
	
	/* зажигаем светодиод */
	BRD_StatusLedOn();
	
	/* настраиваем USART2 для приёма */
	MCB_SetupBUS1();
	/* настраиваем, что по приёму PackedNavdata вызовется функция NavdataRecieved */
	MCB_SetupRxBUS1(&g_rxnavdata, sizeof(PackedNavdata), NavdataRecieved);
	
	for (;;) {
		__NOP();
		
	}
}

/* Коллбек, когда приходят навигационные данные. Печатает их по USB. */
void NavdataRecieved(void)
{
	static char sbuf[500] = {'\0'}; /* буфер для информационной строки */
	
	/* статусы приёмника */
	static char sz_good[] = {"GOOD"};
	static char sz_powerup[] = {"POWER_UP"};
	static char sz_parse_failed[] = {"NMEA_PARSE_FAILED"};
	static char sz_warning[] = {"RECIEVER_REPORTS_WARNING"};
	static char sz_messedup[] = {"DATA IS MESSED UP!"};
	
	/* режим вывода, 0 = текстовый, 1 = двоичный */
	/* чтобы переключить, надо прислать b(inary) или a(scii) в порт устройства */
	static int mode = 1;
	
	char *status_string;
	
	/* если есть USB-соединение... */
	if (TM_USB_VCP_GetStatus() == TM_USB_VCP_CONNECTED) {
		/* мигаем светодиодом */
		BRD_ToggleStatusLed();	
		if (mode == 0) {
			/* проверяем, не пришла ли команда переключиться в двоичный режим */
			char modec;
			if (TM_USB_VCP_Getc((uint8_t *) &modec) == TM_USB_VCP_DATA_OK){
				if (modec == 'b')
					mode = 1;
			}
			
			/* ставим правильную строку статуса для печати в порт */
			switch (g_rxnavdata.status) {
				case GOOD:
				{
					status_string = sz_good;
					break;
				}
				case RECIEVER_REPORTS_WARNING:
				{
					status_string = sz_warning;
					break;
				}
				case NMEA_PARSE_FAILED:
				{
					status_string = sz_parse_failed;
					break;
				}
				case POWER_UP:
				{
					status_string = sz_powerup;
					break;
				}
				default:
					status_string = sz_messedup;
			}
			

			/* готовим информационную строку (JSON) */
			snprintf(sbuf, 1000, JSON_STRING,
						g_rxnavdata.numsat,
						status_string,
						g_rxnavdata.timeh, g_rxnavdata.timem, g_rxnavdata.times,
						g_rxnavdata.lat, g_rxnavdata.lon, 
						g_rxnavdata.alt, 
						g_rxnavdata.hdop);
		
			/* печатаем в порт иформационную строку */
			TM_USB_VCP_Puts(sbuf);	
		}
		else {
			uint8_t modec;
			if (TM_USB_VCP_Getc(&modec) == TM_USB_VCP_DATA_OK) {
				if (modec == 'a') {
					/* переключаемся в текстовый режим */
					mode = 0;
					TM_USB_VCP_Puts("\n\n\n");
				}
			} 
			else {
				/* печатаем заголовок сообщения */
				TM_USB_VCP_Puts("IU4R");
				
				/* печатаем в порт PackedNavdata */
				TM_USB_VCP_Write((char *) &g_rxnavdata, sizeof(PackedNavdata));
			}
			
		}
	} 
	else
		/* пытаемся заново инициализировать USB */
		TM_USB_VCP_Init();
}
