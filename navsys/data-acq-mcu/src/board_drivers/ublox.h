#ifndef __UBLOX__H__
#define __UBLOX__H__

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


typedef struct 
{
	char header[2];
	char msgclass;
	char msgid;
	short len;
} LEA_MSG_HEADER;

typedef struct 
{
	char ck_a;
	char ck_b;
} LEA_MSG_FOOTER;

void UB_MCUConfig(void);

void UB_SendPollHWConf(void);

void UB_StartMessage(LEA_MSG_HEADER *h, char *payload);

void UB_InterruptHandler(void);


#endif // __UBLOX__H__
