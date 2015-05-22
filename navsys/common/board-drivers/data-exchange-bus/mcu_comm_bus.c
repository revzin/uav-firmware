#include <stdint.h>
#include <limits.h>

#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_dma.h"
#include "system_stm32f4xx.h"

#include "board-clocks.h"
#include "ownassert.h"


/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
//		|--------|               |--------|
//		| USRT2  | ----->BUS1>---| USRT2  |
//		|		 |               |        | --------- USBFS ---->
//		| USRT3  | ----->BUS2>---| USRT3  | 	 	  
//		|--------|               |--------|
//         MCUA                     MCUB
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////

void MCB_SetupBUS1(void) 
{
	BRD_ClockFreqs cf;
	BRD_GetClockFrequences(&cf);
	
	assert(cf.apb1_clk == 42.0f, "Bad APB1 clock frequency");
	
	__GPIOA_CLK_ENABLE();
	__USART2_CLK_ENABLE();
	
	/* Бодрейт ставим такой, какой хотим */
	/* int frac = 0;
	int z = 82;
	*/
	int frac = 7;
	int z = 273;
	
	USART2->BRR = (z << 4) | frac;
	
	GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin = GPIO_PIN_2 |  GPIO_PIN_3 |  GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
	
	SET_BIT(USART2->CR1, USART_CR1_UE | USART_CR1_RE | USART_CR1_TE);
	//SET_BIT(USART2->CR3, USART_CR3_DMAT | USART_CR3_DMAR);

	NVIC_EnableIRQ(USART2_IRQn);

#if 0
	/* прерывание USART2_TX находится на Stream 6, Cnannel 4 DMA1 */
	
	__DMA1_CLK_ENABLE();
	
	CLEAR_BIT(DMA1_Stream6->CR, DMA_SxCR_EN);
	while (READ_BIT(DMA1_Stream6->CR, DMA_SxCR_EN)) {__NOP();}

	/* выбираем канал 4 */
	DMA1_Stream6->CR |= 4 << POSITION_VAL(DMA_SxCR_CHSEL_0);
	
	/* размеры регистров/ячеек памяти по 8 бит */
	CLEAR_BIT(DMA1_Stream6->CR, DMA_SxCR_PSIZE); 
	CLEAR_BIT(DMA1_Stream6->CR, DMA_SxCR_MSIZE); 
	
	/* инкремент памяти */
	SET_BIT(DMA1_Stream6->CR, DMA_SxCR_MINC);
	
	/* периферия шлёт запросы */
	SET_BIT(DMA1_Stream6->CR, DMA_SxCR_PFCTRL);
	
	/* Адрес, куда будем "класть" данные */
	DMA1_Stream6->PAR = (uint32_t) &(USART2->DR);
	
	/* Из памяти в периферию */
	MODIFY_REG(DMA1_Stream6->CR, DMA_SxCR_DIR_1, DMA_SxCR_DIR_0);
	
	/* разрешаем прерывание по полной передаче и по ошибке*/
	SET_BIT(DMA1_Stream6->CR, DMA_SxCR_TCIE | DMA_SxCR_TEIE);
	NVIC_EnableIRQ(DMA1_Stream6_IRQn);
#endif
}

#if 0
void MCB_TxBUS1_DMAInterruptHandler(void)
{
	
#ifdef MCUA
	/* очищаем флаг о том, что передача прошла */
	SET_BIT(DMA1->HIFCR, DMA_HIFCR_CTCIF6);
	while (READ_BIT(DMA1_Stream6->CR, DMA_SxCR_EN)) {__NOP();}
	
	/* выключаем DMA */
	CLEAR_BIT(DMA1_Stream6->CR, DMA_SxCR_EN);
#endif
	
	if (READ_BIT(DMA1->HISR, DMA_HISR_TEIF6)) {
		assert(0, "DMA transfer error");
	}
}
#endif

int g_pc_character = 0;
int g_size = 0;
char *g_data = 0;


void MCB_TxBUS1_UART5_Handler(void)
{
	USART2->DR = g_data[g_pc_character];
	g_pc_character++;
	if (g_size ==  g_pc_character) {
		CLEAR_BIT(USART2->CR1, USART_CR1_TXEIE);
		g_pc_character = 0;
	}
}


void MCB_TxBUS1(void *data, unsigned short size)
{
	while (g_pc_character) {__NOP();}
	g_pc_character = 0;
	g_size = size;
	g_data = (char *) data;
	SET_BIT(USART2->CR1, USART_CR1_TXEIE);
}
