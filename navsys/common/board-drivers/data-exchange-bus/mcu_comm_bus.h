#ifndef __MCU_COMM_BUS_H__
#define __MCU_COMM_BUS_H__

void MCB_SetupBUS1(void);
void MCB_TxBUS1(void *data, unsigned short size);
void MCB_TxBUS1_DMAInterruptHandler(void);
void MCB_TxBUS1_UART5_Handler(void);

#endif
