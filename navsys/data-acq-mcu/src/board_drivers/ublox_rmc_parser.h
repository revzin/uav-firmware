void RMC_Enable(void);

float RMC_GetLat(void);

float RMC_GetLon(void);

/* состояние данных */
typedef enum 
{
	GOOD, 							/* достоверно */
	NMEA_PARSE_FAILED, 				/* строка не распарсилась */
	RECIEVER_REPORTS_WARNING, 		/* V в строке */
	POWER_UP 						/* с момента подачи питания ничего не произошло */
} RMC_Status_n;

 RMC_Status_n RMC_GetStatus(void);

void RMC_UART5_Handler(void);
