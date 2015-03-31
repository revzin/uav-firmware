#include "stm32f4xx.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_rcc.h"
#include "stm32f4xx_hal.h"
#include "system_stm32f4xx.h"

#include "ownassert.h"

#include "mpu9250.h"

#define NELEMS(x)  (sizeof(x) / sizeof(x[0]))

#define STACK_SIZE 32

typedef enum {
	STATE_READ,
	STATE_WRITE
} mpu9250_state;

volatile char imu2_tx_stack[STACK_SIZE] = {0};
volatile int imu2_tx_nelems 			= 0;
volatile int imu2_lock_tx_stack 		= 0;

volatile char imu2_rx_stack[STACK_SIZE] = {0};
volatile int imu2_rx_nelems 			= 0;
volatile int imu2_lock_rx_stack 		= 0;

volatile int imu2_state;

#define REG_READ(x) ((x) | 0x40)		/* Добавляет к адресу регистра команду на чтение */
/* Указателем на чтение является 1 в 7 разряде адреса регистра */
#define REG_WRITE(x) ((x) & (~0x40))	/* Добавляет к адресу регистра команду на запись */


inline void wait_imu2_tx_free()
{
	for(; imu2_lock_tx_stack; );
}


inline void wait_imu2_rx_free()
{
	for(; imu2_lock_rx_stack; );
}


void imu2_tx_push(char data)
{
	assert(imu2_tx_nelems < STACK_SIZE, "MPU9250 tx stack overflow");
	imu2_tx_stack[imu2_tx_nelems] = data;
	imu2_tx_nelems++;
}

char imu2_tx_pop()
{
	assert(imu2_tx_nelems, "MPU9250 tried popping empty tx stack");
	--imu2_tx_nelems;
	return imu2_tx_stack[imu2_tx_nelems + 1];
}

void imu2_rx_push(char data)
{
	assert(imu2_rx_nelems < STACK_SIZE, "MPU9250 rx stack overflow");
	imu2_rx_stack[imu2_rx_nelems] = data;
	imu2_rx_nelems++;
}

char imu2_rx_pop()
{
	assert(imu2_rx_nelems, "MPU9250 tried popping empty rx stack");
	--imu2_rx_nelems;
	return imu2_rx_stack[imu2_tx_nelems + 1];
}


void IMU2_ReadReg(MPU9250_REG_ADDRn reg)
{		
	wait_imu2_tx_free();
	
	/* Атомарный доступ к стеку передачи */	
	imu2_lock_tx_stack = 1;
	imu2_tx_push(REG_READ(reg));
	imu2_lock_tx_stack = 0;
	/* Конец атомарного доступ к стеку передачи */
	
	/* Если SPI не задействован, задействуем. 
	 * Если уже задействован, эта инструкция ничего не сделает. */
	SET_BIT(SPI4->CR1, SPI_CR1_SPE);
}

void IMU2_WriteReg(MPU9250_REG_ADDRn reg, char data)
{
	wait_imu2_tx_free();
		
	/* Атомарный доступ к стеку передачи */	
	imu2_lock_tx_stack = 1;
	imu2_tx_push(REG_WRITE(reg));
	imu2_tx_push(data);
	imu2_lock_tx_stack = 0;
	/* Конец атомарного доступ к стеку передачи */
	
	/* Если SPI не задействован, задействуем. 
	 * Если уже задействован, эта инструкция ничего не сделает. */
	SET_BIT(SPI4->CR1, SPI_CR1_SPE);
}


/* Обработчик прерывания */
void IMU2_SPI4InterruptHandler()
{
	if (READ_BIT(SPI4->SR, SPI_SR_TXE)) {
		/* Регистр отправки пуст! */
		
		/* ждём атомарного доступа к стеку передачи */
		wait_imu2_tx_free();
		
		/* забрали себе стек */
		imu2_lock_tx_stack = 1;
		
		if (imu2_state == STATE_READ) {
			/* Если мы хотим прочесть регистр:
			 * 1) 	если есть, что ещё отправлять, то сразу это и отправляем
				При этом прочтётся результат предыдущего запроса, если он был.
			 * 2) 	если нет, то отправляем 0x40 --- запрос на чтение нулевого регистра */
			if (imu2_tx_nelems) 
				SPI4->DR = imu2_tx_pop();
			else
				SPI4->DR = 0x40;
		}
		else {
			/* Если мы хотим записать регистр: просто отправляем дальше */
			if (imu2_tx_nelems)
				SPI4->DR = imu2_tx_pop();	
		}
			
		/* отдали стек */		
		imu2_lock_tx_stack = 0;	
	}

	if (READ_BIT(SPI4->SR, SPI_SR_RXNE)) {
		/* Регистр прихода полон! */

		if (imu2_state == STATE_WRITE) {
			/* пустое чтение, не важно, что там прочлось */
			int discard = SPI4->DR;
		} 
		else {
			/* ждём атомарного доступа к стеку приёма */
			wait_imu2_rx_free();
			
			/* забрали себе стек */
			imu2_lock_rx_stack = 1;
			
			imu2_rx_push(SPI4->DR);
			
			/* отдали стек */
			imu2_lock_rx_stack = 0;
		}
	}
	
	/* настраиваем состояние */
	/* 1. было WRITE. Если дальше команда записи, оставляем WRITE */
	if (imu2_state == STATE_WRITE) {
		
	}
	
	if (!imu2_tx_nelems && !READ_BIT(SPI4->SR, SPI_SR_BSY)) 
		CLEAR_BIT(SPI4->CR1, SPI_CR1_SPE);
		/* Если больше передач нет и SPI свободно, выключаемся */
	
}



typedef struct 
{
	int txstate;			/* Требуется ли что-то передавать после передачи текущего сообщения? */
	char txdata;			/* Если да, то что? */
	char rxdata;
	int active_register;	/* Регистр, запрошенный на чтение */
} MPU9250;

static MPU9250 imu1;
static MPU9250 imu2;

/* табличка делителей  (биты SPI_CR1_BR) 	*/ 
/* 										000 	001 	010 	011 	100 	101		110		111 */
static const int baud_div_lookup[] = {	2,		4,		8,		16,		32,		64,		128,	256};


void nss_select(SPI_TypeDef *SPI)
{
	SET_BIT(SPI->CR1, SPI_CR1_SSI); 
}

void nss_deselect(SPI_TypeDef *SPI)
{
	CLEAR_BIT(SPI->CR1, SPI_CR1_SSI);
}

/* ==== Датчик B (DD5) находится на SPI4 ==== */


/* готовит SPI4 к использованию, возвращает частоту обмена 
 * div --- делитель, степень двойки 2..256. SPI4 находится на APB2 */
void IMU_2_Setup(int div) 
{
	int a = 0;
	
	/* включаем SPI4 */
	__SPI4_CLK_ENABLE();
	__SPI4_RELEASE_RESET();
	
	/* управление nCS в ручной режим */
	//SET_BIT(SPI4->CR1, SPI_CR1_SSM);
	
	
	/* старший бит уходит первым */
	CLEAR_BIT(SPI4->CR1, SPI_CR1_LSBFIRST);
	
	for (; a <= NELEMS(baud_div_lookup); ++a) {
		if (a == NELEMS(baud_div_lookup))
			assert(0, "Invalid divisor");
		if (a == baud_div_lookup[a])
			break;
	}
	
	/* данные устанавливаются на восходящем фронте */
	CLEAR_BIT(SPI4->CR1, SPI_CR1_CPOL);
	CLEAR_BIT(SPI4->CR1, SPI_CR1_CPHA);
	
	/* установили делитель на максимум */
	//SET_BIT(SPI4->CR1, SPI_CR1_BR | (POSITION_VAL(SPI_CR1_BR) << a));
	SET_BIT(SPI4->CR1, SPI_CR1_BR);
	SET_BIT(SPI4->CR1, SPI_CR1_MSTR);
	
	/* разрешаем прерывание по приёму */
	SET_BIT(SPI4->CR2, SPI_CR2_RXNEIE);
	
	
	/* включаем PE */
	__GPIOE_CLK_ENABLE();
	__GPIOE_RELEASE_RESET();
	
	/* переключаем выходы PE2, PE4, PE5, PE6 в AFIO */
	MODIFY_REG(GPIOE->MODER,  GPIO_MODER_MODER2_0 | GPIO_MODER_MODER4_0 | GPIO_MODER_MODER5_0 | GPIO_MODER_MODER6_0, 
								GPIO_MODER_MODER2_1 | GPIO_MODER_MODER4_1 | GPIO_MODER_MODER5_1 | GPIO_MODER_MODER6_1);
	
	/* номер AFIO -- 5 для PE2, PE4..6. */
	SET_BIT(GPIOE->AFR[0], (8 << GPIO_AF5_SPI4) | (16 << GPIO_AF5_SPI4) | (20 << GPIO_AF5_SPI4) | (24 << GPIO_AF5_SPI4));
	
	/* переключаем выходы PE2, PE4, PE5, PE6 в High Speed */
	SET_BIT(GPIOE->OSPEEDR, GPIO_OSPEEDER_OSPEEDR2 | GPIO_OSPEEDER_OSPEEDR4 | GPIO_OSPEEDER_OSPEEDR5 | GPIO_OSPEEDER_OSPEEDR6);
	
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_4|GPIO_PIN_5|GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI4;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
	
	nss_select(SPI4);
	
	NVIC_EnableIRQ(SPI4_IRQn);
	SET_BIT(SPI4->CR1, SPI_CR1_SPE);
}

