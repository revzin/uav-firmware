#include "stm32f4xx.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_rcc.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_spi.h"
#include "system_stm32f4xx.h"

#include "ownassert.h"

#include "mpu9250.h"

#define NELEMS(x)  (sizeof(x) / sizeof(x[0]))

#define STACK_SIZE 32

typedef enum {
	STATE_READ,
	STATE_WRITE
} mpu9250_state;

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

/* ==== Датчик A (DD4) находится на SPI4 ==== */

void HAL_SPI_MspInit(SPI_HandleTypeDef* hspi)
{

  GPIO_InitTypeDef GPIO_InitStruct;
  if(hspi->Instance==SPI1)
  {
  /* USER CODE BEGIN SPI1_MspInit 0 */

  /* USER CODE END SPI1_MspInit 0 */
    /* Peripheral clock enable */
    __SPI1_CLK_ENABLE();
  
    /**SPI1 GPIO Configuration    
    PA5     ------> SPI1_SCK
    PA6     ------> SPI1_MISO
    PA7     ------> SPI1_MOSI 
    */
    GPIO_InitStruct.Pin = GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN SPI1_MspInit 1 */

  /* USER CODE END SPI1_MspInit 1 */
  }

}


HAL_StatusTypeDef HAL_SPI_Init(SPI_HandleTypeDef *hspi)
{
  /* Check the SPI handle allocation */
  if(hspi == NULL)
  {
    return HAL_ERROR;
  }

  /* Check the parameters */
  assert_param(IS_SPI_MODE(hspi->Init.Mode));
  assert_param(IS_SPI_DIRECTION_MODE(hspi->Init.Direction));
  assert_param(IS_SPI_DATASIZE(hspi->Init.DataSize));
  assert_param(IS_SPI_CPOL(hspi->Init.CLKPolarity));
  assert_param(IS_SPI_CPHA(hspi->Init.CLKPhase));
  assert_param(IS_SPI_NSS(hspi->Init.NSS));
  assert_param(IS_SPI_BAUDRATE_PRESCALER(hspi->Init.BaudRatePrescaler));
  assert_param(IS_SPI_FIRST_BIT(hspi->Init.FirstBit));
  assert_param(IS_SPI_TIMODE(hspi->Init.TIMode));
  assert_param(IS_SPI_CRC_CALCULATION(hspi->Init.CRCCalculation));
  assert_param(IS_SPI_CRC_POLYNOMIAL(hspi->Init.CRCPolynomial));

  if(hspi->State == HAL_SPI_STATE_RESET)
  {
    /* Init the low level hardware : GPIO, CLOCK, NVIC... */
    HAL_SPI_MspInit(hspi);
  }
  
  hspi->State = HAL_SPI_STATE_BUSY;

  /* Disble the selected SPI peripheral */
  __HAL_SPI_DISABLE(hspi);

  /*----------------------- SPIx CR1 & CR2 Configuration ---------------------*/
  /* Configure : SPI Mode, Communication Mode, Data size, Clock polarity and phase, NSS management,
  Communication speed, First bit and CRC calculation state */
  hspi->Instance->CR1 = (hspi->Init.Mode | hspi->Init.Direction | hspi->Init.DataSize |
                         hspi->Init.CLKPolarity | hspi->Init.CLKPhase | (hspi->Init.NSS & SPI_CR1_SSM) |
                         hspi->Init.BaudRatePrescaler | hspi->Init.FirstBit  | hspi->Init.CRCCalculation);

  /* Configure : NSS management */
  hspi->Instance->CR2 = (((hspi->Init.NSS >> 16) & SPI_CR2_SSOE) | hspi->Init.TIMode);

  /*---------------------------- SPIx CRCPOLY Configuration ------------------*/
  /* Configure : CRC Polynomial */
  hspi->Instance->CRCPR = hspi->Init.CRCPolynomial;

  /* Activate the SPI mode (Make sure that I2SMOD bit in I2SCFGR register is reset) */
  hspi->Instance->I2SCFGR &= (uint32_t)(~SPI_I2SCFGR_I2SMOD);

  hspi->ErrorCode = HAL_SPI_ERROR_NONE;
  hspi->State = HAL_SPI_STATE_READY;
  
  return HAL_OK;
}

/* готовит SPI1 к использованию, возвращает частоту обмена 
 * div --- делитель, степень двойки 2..256. SPI4 находится на APB2 */
void IMU1_Setup(int div) 
{
	int a = 0;
	
	/* включаем SPI1 */
	__SPI1_CLK_ENABLE();
	//__SPI1_RELEASE_RESET();
	
	/* старший бит уходит первым */
	CLEAR_BIT(SPI1->CR1, SPI_CR1_LSBFIRST);
	
	for (; a <= NELEMS(baud_div_lookup); ++a) {
		if (a == NELEMS(baud_div_lookup))
			assert(0, "Invalid divisor");
		if (a == baud_div_lookup[a])
			break;
	}
	
	/* данные устанавливаются на восходящем фронте */
	CLEAR_BIT(SPI1->CR1, SPI_CR1_CPOL);
	CLEAR_BIT(SPI1->CR1, SPI_CR1_CPHA);
	
	/* установили делитель на максимум */
	//SET_BIT(SPI4->CR1, SPI_CR1_BR | (POSITION_VAL(SPI_CR1_BR) << a));
	SET_BIT(SPI1->CR1, SPI_CR1_BR);
	SET_BIT(SPI1->CR1, SPI_CR1_MSTR);	
	
	SET_BIT(SPI1->CR2, SPI_CR2_SSOE);
	
	SET_BIT(SPI1->CR1, SPI_CR1_SPE);
	
	/* включаем PA */
	__GPIOA_CLK_ENABLE();
	__GPIOA_RELEASE_RESET();
	
	/* включаем PC */
	__GPIOC_CLK_ENABLE();
	__GPIOC_RELEASE_RESET();
	
	/* Настраиваем порта ввода-вывода на линиях SPI */
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.Pin = GPIO_PIN_7 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	
	
	// Эта ножка PC4 -- программный NSS, вечно включённый
	GPIO_InitStruct.Pin = GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
	SET_BIT(GPIOC->BSRRL, GPIO_BSRR_BR_4);
	
	/*
	SPI_HandleTypeDef hspi1;
	hspi1.Instance = SPI1;
	hspi1.Init.Mode = SPI_MODE_MASTER;
	hspi1.Init.Direction = SPI_DIRECTION_2LINES;
	hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
	hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
	hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
	hspi1.Init.NSS = SPI_NSS_SOFT;
	hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
	hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
	hspi1.Init.TIMode = SPI_TIMODE_DISABLED;
	hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLED;
	HAL_SPI_Init(&hspi1);
	NVIC_EnableIRQ(SPI1_IRQn);	
	*/
	SET_BIT(SPI1->CR1, SPI_CR1_SPE);
}

void sleep(int a) 
{
	for (volatile int b = 0; b < a; ++b);
}

void cshigh()
{
	SET_BIT(GPIOC->ODR, GPIO_ODR_ODR_4);
}

void cslow()
{
	CLEAR_BIT(GPIOC->ODR, GPIO_ODR_ODR_4);
}
	

int IMU1_TestSensor(void)
{
	int temp; //= SPI1->DR;
	while (READ_BIT(SPI1->SR, SPI_SR_BSY)) {};
	cslow();
	SPI1->DR = RA_USER_CONTROL;
	while (READ_BIT(SPI1->SR, SPI_SR_BSY)) {};
	sleep(1000);
	temp = SPI1->DR;
	SPI1->DR = 0x10;
	while (READ_BIT(SPI1->SR, SPI_SR_BSY)) {};
	cshigh();
	temp = SPI1->DR;
	//////////////////////	
		
	//////////////////////	
	cslow();
	SPI1->DR = 0x80 | RA_WHO_AM_I;
	while (READ_BIT(SPI1->SR, SPI_SR_BSY)) {};
	temp = SPI1->DR;
	//SET_BIT(SPI1->CR1, SPI_CR1_DFF);
	SPI1->DR = 0x00;
	while (READ_BIT(SPI1->SR, SPI_SR_BSY)) {};
	temp = SPI1->DR;
	SPI1->DR = 0x00;
	while (READ_BIT(SPI1->SR, SPI_SR_BSY)) {};
	//CLEAR_BIT(GPIOA->ODR, GPIO_ODR_ODR_4);
	//SET_BIT(GPIOC->BSRRL, GPIO_BSRR_BS_4);
	temp = SPI1->DR;
	cshigh();
	return temp;
}
