#include <stdlib.h>
#include <string.h>

#include "stm32f4xx.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_rcc.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_usart.h"
#include "system_stm32f4xx.h"

#include "ownassert.h"

#include "ublox_simple_parser.h"


int UBSP_parse_rmc_string(char *rx, nav_data *navdata) 
{
	/* заголовок */
	
	if (strncmp(rx, "$GPRMC", 6)) {
		return -1;
	}
	
	char *header = strtok(NULL, ",");
	
	if (strcmp(rx, "$GPRMC")) {
		navdata->state = STATE_SYSTEM_FAILURE;
		return -1;
	}
	
	/* время захвата */
	
	char *tof = strtok(NULL, ",");
	
	if (!tof) {
		navdata->state = STATE_SYSTEM_FAILURE;
		return -1;
	}
	
	/* статус */
	
	char *status = strtok(NULL, ",");
		
	if (!status) {
		navdata->state = STATE_SYSTEM_FAILURE;
		return -1;
	}
	
	if (!strcmp(status, "V")) {
		navdata->state = STATE_GPSRX_WARN;
	}
	
	if (!strcmp(status, "A")) {
		navdata->state = STATE_GOOD;
	}
	
	
	/* широта */
	
	char *lat = strtok(NULL, ",");
	
	if (!lat) {
		navdata->state = STATE_SYSTEM_FAILURE;
		return -1;
	}
	
	navdata->lat = atof(lat);
	
	char *lats = strtok(NULL, ",");
	
	if (!lats) {
		navdata->state = STATE_SYSTEM_FAILURE;
		return -1;
	}
	
	if (lats[0] == 'S')
		navdata->lat *= -1.0f;
	
	/* долгота */
	
	char *lon = strtok(NULL, ",");
	
	if (!lon) {
		navdata->state = STATE_SYSTEM_FAILURE;
		return -1;
	}
	
	navdata->lat = atof(lon);
	
	char *lons = strtok(NULL, ",");
	
	if (!lons) {
		navdata->state = STATE_SYSTEM_FAILURE;
		return -1;
	}
	
	if (lons[0] == 'W')
		navdata->lat *= -1.0f;
	
	return 0;
}


#include "stm32f4xx_hal_dma.h"







