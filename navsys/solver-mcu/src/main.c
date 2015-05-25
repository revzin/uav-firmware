#include "stm32f4xx.h"

#include "board-clocks.h"
#include "board-status-led.h"

#include "mcu_common_types.h"
#include "mcu_comm_bus.h"
#include "tm_stm32f4_usb_vcp.h"

#include "string.h" // memcpy

void NavdataRecieved(void);

PackedNavdata g_rxnavdata;

/* Точка входа микроконтроллера-решателя */
int main(void) 
{
	/* настраиваем часы на 168 МГц */
	BRD_SetupMainClock();
	
	BRD_ClockFreqs cf;
	BRD_GetClockFrequences(&cf);
	
	TM_USB_VCP_Init();
	TM_USB_VCP_Result a = TM_USB_VCP_GetStatus();
	
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
	static char sbuf[100] = {'\0'};
	static char sz_good[] = {"GOOD"};
	static char sz_powerup[] = {"POWER_UP"};
	static char sz_parse_failed[] = {"NMEA_PARSE_FAILED"};
	static char sz_warning[] = {"RECIEVER_REPORTS_WARNING"};
	static char sz_messedup[] = {"DATA IS MESSED UP!"};
	
	static int mode = 0;
	
	char *psz;
	
	if (TM_USB_VCP_GetStatus() == TM_USB_VCP_CONNECTED) {
		if (mode == 0) {

			char modec;

			if (TM_USB_VCP_Getc((uint8_t *) &modec) == TM_USB_VCP_DATA_OK){
				if (modec == 'b')
					mode = 1;
			}
			
			switch (g_rxnavdata.status) {
				case GOOD:
				{
					psz = sz_good;
					break;
				}
				case RECIEVER_REPORTS_WARNING:
				{
					psz = sz_warning;
					break;
				}
				case NMEA_PARSE_FAILED:
				{
					psz = sz_parse_failed;
					break;
				}
				case POWER_UP:
				{
					psz = sz_powerup;
					break;
				}
				default:
					psz = sz_messedup;
			}
			
			BRD_ToggleStatusLed();
			
			
			
			snprintf(sbuf, 200, "%2d:%2d:%2d: %2dSAT>> LAT = %.7f; LON = %.7f; HEIGHT = %.2f ASL;\n" 
								"                     HDOP = %.2f; STATUS = %s;\n", 
						g_rxnavdata.timeh, g_rxnavdata.timem, g_rxnavdata.times, g_rxnavdata.numsat,
						g_rxnavdata.lat, g_rxnavdata.lon, g_rxnavdata.alt, g_rxnavdata.hdop, psz);
		

			TM_USB_VCP_Puts(sbuf);
			

			
		}
		else {
			
			uint8_t modec;
			if (TM_USB_VCP_Getc(&modec) == TM_USB_VCP_DATA_OK){
				if (modec == 'a')
					mode = 0;
			}
			
			TM_USB_VCP_Puts("IU4R");
			TM_USB_VCP_Write((char *) &g_rxnavdata, sizeof(PackedNavdata));
			
		}
	} 
	else
		TM_USB_VCP_Init();
	
}
