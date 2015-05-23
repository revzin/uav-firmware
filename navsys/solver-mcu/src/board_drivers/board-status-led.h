#ifndef __BOARD_STATUS_LED__
#define __BOARD_STATUS_LED__

/* Функции управления светодиодиком на PA7 */

/* подготавливает порт к использованию */
void BRD_InitStatusLedControl(void);

/* зажигает светодиод */
void BRD_StatusLedOn(void);

/* гасит светодиод */
void BRD_StatusLedOff(void);

/* светодиод горит? */
int BRD_IsStatusLedOn(void);

/* переключение */
void BRD_ToggleStatusLed(void);

#endif /* __BOARD_STATUS_LED__ */
