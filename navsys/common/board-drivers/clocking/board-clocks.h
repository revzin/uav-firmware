#ifndef __BOARD_CLOCKS_H__
#define __BOARD_CLOCKS_H__

/* функции работы с часовыми сигналами на плате навигационной системы */

/* частоты основных шин часов в МГц */
typedef struct {
	float 	sysclk,
			ahb_clk,
			apb1_clk,
			apb2_clk,
			usb_clk,
			abp1_tim_clk,
			abp2_tim_clk;
} BRD_ClockFreqs;

/* частота внешнего генератора, МГц */
#define HSE_MHZ 26.0f 

/* частота внутреннего генератора, МГц */
#define HSI_MHZ 16.0f

/* рассчитывает и получает частоты основных шин */
void BRD_GetClockFrequences(BRD_ClockFreqs *s);

/** Выставляет основные часы микроконтроллера согласно схеме в папке с файлом (sysclk = 168 МГц).
 *	Возвращает 1, если всё хорошо, 0, если запустить не удалось.
 *	Микроконтроллер останавливается на 20 мс при вызове функции.
 */
int BRD_SetupMainClock(void);

/* Включает выход MCO1 на МК, при этом SYSCLK делится на 4 */
void BRD_MCO_Enable(void);

/* Выключает */
void BRD_MCO_Disable(void);

#endif /* __BOARD_CLOCKS_H__ */
