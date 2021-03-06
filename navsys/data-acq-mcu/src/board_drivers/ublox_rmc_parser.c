#include <stdlib.h>
#include <string.h>

#include "stm32f4xx.h"
#include "stm32f4xx_hal_dma.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_rcc.h"
#include "stm32f4xx_hal_uart.h"

#include "ublox_rmc_parser.h"
#include "board-clocks.h"
#include "ownassert.h"


#define BUF_SIZE (32)

/* -------- прототипы локальных функций -------- */

int parseid(void);
RMC_Status_n parsetof(void);
RMC_Status_n parsestat(void);
RMC_Status_n parselat(void);
RMC_Status_n parselatd(void);
RMC_Status_n parselon(void);
RMC_Status_n parselond(void);
RMC_Status_n parsehdop(void);
RMC_Status_n parsealt(void);
RMC_Status_n parsenumsat(void);

void setfail(RMC_Status_n code);

void resetbuf(void);
void next(char c);

void nextstate(void);
void idlestate(void);

void threadlock(void);
void threadunlock(void);

/* --------------------------- */

/* состояния парсинга GGA-строки */
typedef enum
{
	IDLE,
	ID,
	TOF,
	LAT,
	LATD,
	LON,
	LOND,
	STAT,
	NUMSAT,
	HDOP,
	ALT
} rxstate_n;


typedef struct 
{
	float lat, lon, 
	height, 
	hdop_acc;
	int numsat;
	RMC_FixTime fixtime;
}	nav_data;



nav_data g_navdata;

int 			g_lock = 0;						/* флаг, что nav_data занята */

char 			g_buffer[BUF_SIZE]; 			/* буфер чтения */
int 			g_counter = 0; 					/* счётчик буфера чтения */
rxstate_n 		g_state = IDLE;   				/* состояние парсинга */
RMC_Status_n 	g_status = POWER_UP; 		/* состояние строки */


/* --------- интерфейс -------- */

void RMC_Enable(void)
{
	BRD_ClockFreqs cf;
	BRD_GetClockFrequences(&cf);
	
	assert(cf.apb1_clk == 42.0f, "Bad APB1 clock frequency");
	
	__GPIOD_CLK_ENABLE();
	__UART5_CLK_ENABLE();
	/* при частоте APB1 42 МГц для бодрейта 9600 в BRR 
	 * записывается 273.4375 */
	int frac = 7;
	int z = 273;
	
	UART5->BRR = (z << 4) | frac;
	
	GPIO_InitTypeDef GPIO_InitStruct;
	
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF8_UART5;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
	
	SET_BIT(UART5->CR1, USART_CR1_UE | USART_CR1_RE | USART_CR1_RXNEIE);
	
	NVIC_SetPriority(UART5_IRQn, 1);
	NVIC_EnableIRQ(UART5_IRQn);
}

float RMC_GetLat(void)
{
	/* ждём, пока данные по навигации не заняты */
	while (g_lock != 0) {__NOP();}
	return g_navdata.lat;
}

float RMC_GetLon(void)
{
	while (g_lock != 0) {__NOP();}
	return g_navdata.lon;
}

void RMC_GetFixTime(RMC_FixTime *ft)
{
	memcpy(ft, &g_navdata.fixtime, sizeof(RMC_FixTime));
}

RMC_Status_n RMC_GetStatus(void) 
{
	return  g_status;
}

/* ------------------------------ */


/* обработчик прерывания по приходу */
void RMC_UART5_Handler(void)
{
	/* конечный автомат, пробегает по RMC-строке, приходящей по одному байту с UART */
	char rx = UART5->DR;
		
	switch (g_state) {
		case IDLE:
		{
			if (rx == '$') {
				nextstate();
				threadlock();
			}
			return;
		}
		/* --------------------- */
		case ID:
		{
			if (rx == ',') {
				if (!parseid()) {
					nextstate();
				} 
				else 
					idlestate();
			}
			else {
				next(rx);
			}
			return;
		}
		/* --------------------- */	
		case TOF:
		{
			if (rx == ',') {
				RMC_Status_n stat = parsetof();
				nextstate();
				if (!stat) {
					g_status = GOOD;
				} 
				else {
					setfail(stat);
					//memset(&g_navdata.fixtime, 0, sizeof(RMC_FixTime));
				}
			}
			else {
				next(rx);
			}
			return;				
		}
		
		/* --------------------- */
		case LAT:
		{
			if (rx == ',') {
				RMC_Status_n s = parselat();
				nextstate();
				if (!s) {
					g_status = GOOD;
				} 
				else {
					setfail(s);
				}
			}
			else {
				next(rx);
			}
			return;				
		}
		/* --------------------- */
		case LATD:
		{
			if (rx == ',') {
				RMC_Status_n s = parselatd();
				if (!s) {
					g_status = GOOD;
				} 
				else {
					setfail(s);
				}
				nextstate();
			}
			else {
				next(rx);
			}
			return;				
		}
		/* --------------------- */
		case LON:
		{
			if (rx == ',') {
				RMC_Status_n s = parselon();
				nextstate();
				if (!s) {
					g_status = GOOD;
				} 
				else {
					setfail(s);
				}
			}
			else {
				next(rx);
			}
			return;		
		}
		/* --------------------- */
		case LOND:
		{
			if (rx == ',') {
				RMC_Status_n s = parselond();
				nextstate();
				if (!s) {
					g_status = GOOD;
				} 
				else {
					setfail(s);
				}
			}
			else {
				next(rx);
			}
			return;				
		}
		/* --------------------- */
		case STAT:
		{
			if (rx == ',') {
				int stat = parsestat();
				if (!stat) {
					/* всё хорошо */			
					g_status = GOOD;
				} 
				else {
					setfail((RMC_Status_n) stat);
				}
				nextstate();
			}
			else {
				next(rx);
			}
			return;
		}
		/* --------------------- */
		case NUMSAT:
		{
			if (rx == ',') {

				RMC_Status_n stat = parsenumsat();
				if (!stat) {
					nextstate();
				} 
				else {
					setfail((RMC_Status_n) stat);
					idlestate();
				}
			}
			else {
				next(rx);
			}
			return;
		}
		/* --------------------- */
		case HDOP:
		{
			if (rx == ',') {
				RMC_Status_n stat = parsehdop();
				if (!stat) {
					nextstate();
				} 
				else {
					setfail((RMC_Status_n) stat);
					idlestate();
				}
			}
			else {
				next(rx);
				/* больше одного символа в поле статуса быть не может */
			}
			return;
		}
		/* --------------------- */
		case ALT:
		{
			if (rx == ',') {
				RMC_Status_n stat = parsealt();
				if (!stat) {
					idlestate();
				} 
				else {
					setfail(stat);
					idlestate();
				}
			}
			else {
				next(rx);
			}
			return;
		}
	}
	
}

void resetbuf() 
{
	memset((void *) g_buffer, '\0', BUF_SIZE * sizeof(char));
	g_counter = 0;
}

void next(char ch) 
{
	g_buffer[g_counter] = ch;
	g_counter++;
}

void nextstate() {
	g_state++;
	resetbuf();
}

void idlestate() {
	threadunlock();
	g_state = IDLE;
}

int parseid() {
	/* отбрасываюся все сообщения, кроме GPRMC */
	return  strncmp("GPGGA", g_buffer, strlen("GPGGA"));
}

RMC_Status_n parsestat()
{
	if (g_buffer[0] == '1' || g_buffer[0] == '2')
		return GOOD;
	if (g_buffer[0] == '0')
		return RECIEVER_REPORTS_WARNING;
	else
		return NMEA_PARSE_FAILED;
}

RMC_Status_n parsetof()
{
	static char h[3] = {'\0'}, 
				m[3] = {'\0'}, 
			    *s = g_buffer + 4;
	
	if (strlen(g_buffer) < 6)
		return RECIEVER_REPORTS_WARNING;
	
	memset(h, '\0', 3);
	memset(m, '\0', 3);
	
	memcpy(h, g_buffer, 2 * sizeof(char));
	memcpy(m, g_buffer + 2, 2 * sizeof(char));
	
	
	g_navdata.fixtime.h = (char) atoi(h);
	g_navdata.fixtime.m = (char) atoi(m);
	g_navdata.fixtime.s = (char) atoi(s);
	
	if (g_navdata.fixtime.h < 0 && g_navdata.fixtime.h > 24)
		return RECIEVER_REPORTS_WARNING;
	
	if (g_navdata.fixtime.m < 0 && g_navdata.fixtime.m > 60)
		return RECIEVER_REPORTS_WARNING;	
	
	if (g_navdata.fixtime.s < 0 && g_navdata.fixtime.s > 60)
		return RECIEVER_REPORTS_WARNING;
	
	return GOOD;
}

/* RMC-угол ("5544.2222") в нормальный float в градусах */
/*            ГГММ.ММММ  								*/  
float getangle(char *b, int lat)
{
	char deg[4] = {'\0'},  /* +1 байт на завершение строки */
				*min;	
	
	/* широта всегда 2 символа, долгота 3 */
	if (lat) {
		min =  b + 2; 	/* теперь указывает на минуты */
		memcpy(deg, b, 2 * sizeof(char));
	}
	
	if (!lat) { 
		min = b + 3;
		memcpy(deg, b, 3 * sizeof(char));
	}
	
	float z = atof(deg),
		  f = atof(min);
	return z + (f / 60.0f); 
}

RMC_Status_n parselat() 
{		
	g_navdata.lat = getangle(g_buffer, 1);
	if (g_navdata.lat == 0.0f)
		return RECIEVER_REPORTS_WARNING;
	return GOOD;
}

RMC_Status_n parselatd() 
{		
	if (g_buffer[0] == 'S') {
		g_navdata.lat *= -1.0f;
	}
	else if (g_buffer[0] == 'N') {
		/* ничего не происходит */
	}
	else {
		return RECIEVER_REPORTS_WARNING;
	}
	return GOOD;
}

RMC_Status_n parsealt()
{
	g_navdata.height = atof(g_buffer);
	if (g_navdata.height == 0.0f)
		return RECIEVER_REPORTS_WARNING;
	return GOOD;
}

RMC_Status_n parsehdop()
{
	g_navdata.hdop_acc = atof(g_buffer);
	if (g_navdata.hdop_acc == 0.0f)
		return RECIEVER_REPORTS_WARNING;
	return GOOD;
}

RMC_Status_n parsenumsat()
{
	int ns = atoi(g_buffer);
	if (ns > 2 && ns < 16)	{
		g_navdata.numsat = ns;
		return GOOD;
	} 
	else if (ns >= 0 && ns < 3)	{
		g_navdata.numsat = ns;
		return RECIEVER_REPORTS_WARNING;
	} 
	else {
		g_navdata.numsat = 0;
		return NMEA_PARSE_FAILED;
	}
}

RMC_Status_n parselon() 
{	
	g_navdata.lon = getangle(g_buffer, 0);
		if (g_navdata.lon == 0.0f)
		return RECIEVER_REPORTS_WARNING;
	return GOOD;
}

RMC_Status_n parselond() 
{		
	if (g_buffer[0] == 'W') {
		g_navdata.lon *= -1.0f;
	}
	else if (g_buffer[0] == 'E') {
		/* ничего не происходит */
	}
	else {
		return RECIEVER_REPORTS_WARNING;
	}
	return GOOD;
}

/* высота ASL в метрах */
float RMC_GetASL(void)
{
	return g_navdata.height;
}

/* ошибка в метрах */
float RMC_GetHDOP(void)
{
	return g_navdata.hdop_acc;
}

/* количество спутников */
int RMC_GetNumSat(void)
{
	return g_navdata.numsat;
}

void threadlock()
{
	g_lock = 1;
}

void threadunlock()
{
	g_lock = 0;
}

int RMC_IsLocked(void)
{
	return g_lock;
}

void setfail(RMC_Status_n c) 
{
	g_status = c;
}


