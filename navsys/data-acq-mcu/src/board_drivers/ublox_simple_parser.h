
#ifndef __UBLOX_SIMPLE_PARSER_H__
#define __UBLOX_SIMPLE_PARSER_H__

typedef enum
{
	STATE_SYSTEM_FAILURE,
	STATE_GPSRX_WARN,
	STATE_GOOD
} nav_state_n;

typedef struct 
{
	int state;
	float lat, lon;
} nav_data;

int UBSP_parse_rmc_string(char *rx, nav_data *navdata);

void USBP_setup();

void USBP_enable_interrupts();

#endif
