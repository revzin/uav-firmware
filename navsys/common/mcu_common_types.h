#ifndef __MCU_COMMON_TYPES__
#define __MCU_COMMON_TYPES__

/* состояние данных */
typedef enum 
{
	GOOD, 							/* достоверно */
	NMEA_PARSE_FAILED, 				/* строка не разобралась */
	RECIEVER_REPORTS_WARNING, 		/* приёмник предупреждает о проблеме */
	POWER_UP 						/* после подачи питания на МК ещё не пришло ни одного пакета*/
} RMC_Status_n;

/* "упакованные" данные для передачи по шине */
typedef struct
{
	float lat, lon, alt, hdop;
	char timeh, timem, times;
	char status; // RMC_Status_n
	char numsat;
	char dpgs_on;
} PackedNavdata;

#endif
