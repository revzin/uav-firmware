#include <stdlib.h>
#include <string.h>

#include "stm32f4xx.h"
#include "stm32f4xx_hal_dma.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_rcc.h"
#include "stm32f4xx_hal_uart.h"

#define BUF_SIZE (32)

#define UART_RX UART5

/* -------- прототипы локальных функций -------- */

int parseid(void);
int parsetof(void);
int parsestat(void);
int parselat(void);
int parselatd(void);
int parselon(void);
int parselons(void);

void setfail(int code);

void resetbuf(void);
void next(char c);

void nextstate(void);
void idlestate(void);

void threadlock(void);
void threadunlock(void);

void uart_rxne_handler(void);

/* --------------------------- */

/* состояния парсинга NMEA-строки */
typedef enum
{
	IDLE,
	ID,
	TOF,
	STAT,
	LAT,
	LATD,
	LON,
	LOND
} rxstate_n;

/* состояние данных */
typedef enum 
{
	GOOD, 							/* достоверно */
	NMEA_PARSE_FAILED, 				/* строка не распарсилась */
	RECIEVER_REPORTS_WARNING, 		/* V в строке */
	POWER_UP 						/* с момента подачи питания ничего не произошло */
} failure_mode_n;


/* --------- состояние парсинга */
typedef struct 
{
	float lat, lon;
}	
nav_data;

nav_data g_navdata;
int g_lock = 0;						/* флаг, что nav_data занята */

char g_buffer[BUF_SIZE]; 			/* буфер чтения */
int g_counter = 0; 					/* счётчик буфера чтения */
int g_state = 0;   					/* состояние парсинга */
int g_failure_mode = POWER_UP; 		/* состояние строки */



/* --------- интерфейс -------- */

void RMC_Enable(void)
{
	__GPIOC_CLK_ENABLE();
	__GPIOD_CLK_ENABLE();
	
	__UART5_CLK_ENABLE();
	/* при частоте APB1 42 МГц для бодрейта 9600 в BRR 
	 * записывается 273.4375 */
	int frac = 7;
	int z = 273;
	
	UART5->BRR = (z << 4) | frac;
	
	GPIO_InitTypeDef GPIO_InitStruct;
	
	GPIO_InitStruct.Pin = GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF8_UART5;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF8_UART5;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
	
	SET_BIT(UART5->CR1, USART_CR1_UE | USART_CR1_RE | USART_CR1_TE);
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

int RMC_GetStatus(void) 
{
	return g_failure_mode;
}

/* ------------------------------ */


/* обработчик прерывания по приходу */
void uart_rxne_handler(void)
{
	/* конечный автомат, пробегает по RMC-строке, приходящей по одному байту с UART */
	char rx = UART_RX->DR;
	
	switch (g_state) {
		case IDLE:
		{
			if (rx == '$') 
				nextstate();
			return;
		}
		/* --------------------- */
		case ID:
		{
			if (rx == ',') {
				if (!parseid()) {
					nextstate();
				} 
				else {
					setfail(NMEA_PARSE_FAILED);
					idlestate();
				}
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
				if (!parsetof()) {
					nextstate();
				} 
				else {
					setfail(NMEA_PARSE_FAILED);
					idlestate();
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
					/* всё хорошо, приёмник сказал 'A' = достоверные данные */
					nextstate();
					threadlock();
				} 
				else {
					setfail(stat);
					idlestate();
				}
			}
			else {
				setfail(NMEA_PARSE_FAILED);
				idlestate();
				/* больше одного символа в поле статуса быть не может */
			}
			return;
		}
		case LAT:
		{
			if (rx == ',') {
				if (!parselat()) {
					nextstate();
				} 
				else {
					setfail(RECIEVER_REPORTS_WARNING);
					idlestate();
				}
			}
			else {
				next(rx);
			}
			return;				
		}
		case LATD:
		{
			if (rx == ',') {
				if (!parselatd()) {
					nextstate();
				} 
				else {
					setfail(NMEA_PARSE_FAILED);
					idlestate();
				}
			}
			else {
				setfail(NMEA_PARSE_FAILED);
				idlestate();
				/* больше одного символа в поле обозначения широты быть не может */
			}
			return;				
		}
		case LON:
		{
			if (rx == ',') {
				if (!parselon()) {
					nextstate();
				} 
				else {
					setfail(RECIEVER_REPORTS_WARNING);
					idlestate();
				}
			}
			else {
				next(rx);
			}
			return;		
		}
		case LOND:
		{
			if (rx == ',') {
				if (!parselatd()) {
					nextstate();
				} 
				else {
					setfail(NMEA_PARSE_FAILED);
					idlestate();
				}
			}
			else {
				setfail(NMEA_PARSE_FAILED);
				idlestate();
				/* больше одного символа в поле обозначения дологты быть не может */
			}
			return;				
		}
	}
	
}

void resetbuf() 
{
	memset((void *) g_buffer, '\0', 25 * sizeof(char));
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
	return (strncmp("GPRMC", g_buffer, strlen("GPRMC")));
}

int parsestat()
{
	if (g_buffer[0] == 'A')
		return 0;
	if (g_buffer[0] == 'V')
		return RECIEVER_REPORTS_WARNING;
	else
		return NMEA_PARSE_FAILED;
}

int parsetof()
{
	return !(g_counter > 0);
}

int parselat() 
{		
	g_navdata.lat = atof(g_buffer);
	if (g_navdata.lat == 0.0f)
		return 1;
	return 0;
}

int parselatd() 
{		
	if (g_buffer[0] == 'S')
	{
		g_navdata.lat *= -1.0f;
	}
	else if (g_buffer[0] == 'N')
	{
		/* ничего не происходит */
	}
	else 
	{
		return NMEA_PARSE_FAILED;
	}
	return 0;
}

int parselon() 
{		
	g_navdata.lon = atof(g_buffer);
		if (g_navdata.lon == 0.0f)
		return 1;
	return 0;
}

int parselond() 
{		
	if (g_buffer[0] == 'W')
	{
		g_navdata.lon *= -1.0f;
	}
	else if (g_buffer[0] == 'E')
	{
		/* ничего не происходит */
	}
	else 
	{
		return NMEA_PARSE_FAILED;
	}
	return 0;
}


void threadlock()
{
	g_lock = 1;
}

void threadunlock()
{
	g_lock = 0;
}

void setfail(int c) 
{
	g_failure_mode = c;
}
