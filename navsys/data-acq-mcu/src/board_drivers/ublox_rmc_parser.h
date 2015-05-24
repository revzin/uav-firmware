#ifndef __UBLOX_RMC_PARSER_H__
#define __UBLOX_RMC_PARSER_H__

#include "mcu_common_types.h"

/* старт парсинга GPS-сообщений */
void RMC_Enable(void);

int RMC_IsLocked(void);

/* широта в градусах. Южная широта - отрицателная */
float RMC_GetLat(void);

/* долгода в градусах. Западная долгота - отрицателная */
float RMC_GetLon(void);


RMC_Status_n RMC_GetStatus(void);

/* Время получения навигацонной информации, часы, минуты, секунды */
typedef struct
{
	char h, m, s;
	char _reserved; /* паддинг до 32 бит */
} RMC_FixTime;

/* получить время записи */
void RMC_GetFixTime(RMC_FixTime *ft);

/* обработчик прерывания UART5 */
void RMC_UART5_Handler(void);

#endif
