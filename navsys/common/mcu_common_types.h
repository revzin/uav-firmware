#ifndef __MCU_COMMON_TYPES__
#define __MCU_COMMON_TYPES__

typedef struct
{
	float lat, lon;
	char timeh, timem, times;
	char status;
	
} PackedNavdata;

#endif
