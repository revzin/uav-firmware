#ifndef __BOARD_CLOCKS_H__
#define __BOARD_CLOCKS_H__

/* функции работы с часовыми сигналами на плате навигационной системы */

/* частоты основных шин часов в Гц */
typedef struct {
		float 	sysclk,
						ahb_clk,
						apb1_clk,
						apb2_clk,
						usb_clk;
} BRD_ClockFreqs;

/* рассчитывает и получает частоты основных шин */
void BRD_GetClockFrequences(BRD_ClockFreqs *s);

/** Выставляет основные часы микроконтроллера согласно схеме в папке с файлом (sysclk = 168 МГц).
 *	Возвращает 1, если всё хорошо, 0, если запустить не удалось.
 *	Микроконтроллер останавливается на 20 мс при вызове функции.
 */
int BRD_SetupMainClock(void);

#endif /* __BOARD_CLOCKS_H__ */