#include "stm32f4xx.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_rcc.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_usart.h"
#include "system_stm32f4xx.h"

#include "ownassert.h"
#include "ublox.h"


#ifdef USE_FULL_UBLOX_PROTOCOL

#define LEA_HEADER 0xB5, 0x62
#define CLASS_NAV 0x01
#define CLASS_RXM 0x02
#define CLASS_INF 0x04
#define CLASS_ACK 0x05
#define CLASS_CFG 0x06
#define CLASS_MON 0x0A
#define CLASS_AID 0x0B
#define CLASS_TIM 0x0D
#define CLASS_ESF 0x10

#define ACK_ACK 0x01
#define ACK_NAK 0x00

#define MON_HW 0x09

#define CK_A 0x00
#define CK_B 0x00


LEA_MSG_FOOTER footer;

LEA_MSG_HEADER MON_HW_HEADER = 
{
	LEA_HEADER,
	CLASS_MON,
	MON_HW,
	0
};

LEA_MSG_HEADER LEA_MSG_HEADER_ACK_ACK = 
{
	LEA_HEADER,
	CLASS_MON,
	ACK_ACK,
	0
};

LEA_MSG_HEADER LEA_MSG_HEADER_ACK_NAK = 
{
	LEA_HEADER,
	CLASS_MON,
	ACK_NAK,
	0
};


void UB_SendPollHWConf()
{
	UB_StartMessage(&MON_HW_HEADER, 0);
}

void UB_MCUConfig()
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

int txmsgpos, txtotlen;
char txbuffer[255] = {0};

int rxmsgpos = 0;
char rxbuffer[255] = {0};



void ublox_checksum(LEA_MSG_HEADER *msg_header, 
					char *msg_payload,
					LEA_MSG_FOOTER *msg_footer)
{
	int a;
	char *msg = ((char *) msg_header) + 2;
	msg_footer->ck_a = 0;
	msg_footer->ck_b = 0;
	for (a = 0; a < msg_header->len + 2; ++a) {
		msg_footer->ck_a = msg_footer->ck_a + msg[a];
		msg_footer->ck_b = msg_footer->ck_b + msg_footer->ck_a;
	}
}

void UB_StartMessage(LEA_MSG_HEADER *h, char *payload)
{
	int a, b;
	char *pp;
	static LEA_MSG_FOOTER f;
	ublox_checksum(h, payload, &f);
	
	pp = (char *) h;
	for (b = a = 0; a < sizeof(LEA_MSG_HEADER); ++a, ++b) {
		txbuffer[a] = pp[b]; 
	}
	
	pp = payload;
	for (b = 0; b < h->len; ++b, ++a) {
		txbuffer[a] = pp[b];
	}
	
	pp = (char *) &f;
	for (b = 0; b < sizeof(LEA_MSG_FOOTER); ++b, ++a) {
		txbuffer[a] = pp[b];
	}
	
	txmsgpos = 0;
	
	txtotlen = a;
	
	UART5->DR = txbuffer[0];
	SET_BIT(UART5->CR1, USART_CR1_TXEIE);	
}


enum {
	RX_HEADER,
	RX_PAYLOAD,
	RX_CHECKSUM
} rx_state;

int rxstate = 0;


void UB_InterruptHandler() 
{
	if (READ_BIT(UART5->SR, USART_SR_TXE)) {
		txmsgpos++;
		if (txmsgpos == txtotlen) {
			CLEAR_BIT(UART5->CR1, USART_CR1_TXEIE);
			return;
		}
		UART5->DR = txbuffer[txmsgpos];
	} 
	/*
	if (READ_BIT(UART5->SR, USART_SR_RXNE)) {
		rxbuffer[rxmsgpos] = UART5->DR;
		if (rxmsgpos == 1 && rxbuffer[rxmsgpos] == 0x62 
								&& rxbuffer[rxmsgpos - 1] == 0xB5) 
		{
			rxstate = RX_
		}
	}*/
}

#endif

#ifdef USE_SIMPLE_LEA_PARSER

#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 100

typedef enum 
{
	NAVSTATE_SHIT,
	NAVSTATE_GOOD
} navstate_n;

typedef struct
{
	float lat, lon;
	int timeoffix;
	navstate_n navstate;
} navdata;

navdata nav_data;

// Конечный автомат приёма 
// ========================================= //

typedef enum 
{
	STATE_FUCKED,
	STATE_RX_ID,
	STATE_RX_TIME,
	STATE_RX_STATUS,
	STATE_RX_LAT,
	STATE_RX_LAT_S,
	STATE_RX_LON,
	STATE_RX_LON_S;
} parser_state_n;


int parser_state = STATE_FUCKED;

char parser_buffer[100] = {'\0'};

int buffer_counter = 0;

void HandleParserInterrupt()
{
	char rxd = READ_REG(UART5->DR);
	switch (parser_state) {
		case STATE_FUCKED:
		{
			if (rxd == '$') {
				/* пришёл доллар, значит, новое соообщение и следующим будет заголовок */
				parser_state = STATE_RXID;
				buffer_counter = 0;
				goto ret;
			}
			break;
		}
		
		case STATE_RX_ID:
		{
			if (rxd == ',') {
				/* пришёл полностью заголовок. игнорируем все сообщения, кроме GPRMC */
				if (!strncmp(parser_buffer, "GPRMC", 5)) {
					parser_state = STATE_RX_TIME;
					buffer_counter = 0;
					/* дальше ловим время приёма */
				}
				else 
					/* грустнявка */
					state = STATE_FUCKED;
				
				goto ret;
			}
			else {
				/* складываем пришедший байт в буфер */
				parser_buffer[buffer_counter] = rxd;
				buffer_counter++;
			}
			break;
		}
		
		case STATE_RX_TIME:
		{
			if (rxd == ',') {
				char secondary_buffer[10];
				/* пришло полностью время */
				nav_data.time_of_fix = atoi(parser_buffer);
				buffer_counter = 0;
				state = STATE_RX_STATUS;
				goto ret;
			}
			else {
				
			}
		}
		
		
	}
	
	
	ret:
	
}


#endif
