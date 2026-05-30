
#ifndef	__MYEFFECT__
#define	__MYEFFECT__

#include <fcntl.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event_loop.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_now_config.h"

#define PATH_EFFECT "/spiffs/effect.bin"
#define PATH_EFFECT_TMP "/spiffs/effect_tmp.bin"

#define MAX_NUM_CHANNEL 8

typedef struct 
{
	uint8_t numPin;				/**< number of pins of object */
	uint8_t* p_pin;				/**< pin list of object, total number of elements = numPin */
	uint8_t* brNessofState;			/**< brightness of object at each state, total number of elements = numState */
} object_t;	

typedef struct
{
	uint16_t numState;		/**< number of states of this group */
	uint8_t* p_timExistOfSta;	/**< time of each state, total number of elements = numState */
	uint8_t numObject;		/**< number of objects of this group */
	object_t* p_object;		/**< total number of elements = numObject */
} group_t;

typedef struct
{
	uint8_t brNess;			/**< synchronization brightness for groups, devices */
	uint8_t speedEf;		/**< synchronization speed for groups, devices */
	uint8_t numGroup;		/**< number of Group */
	uint32_t* offStartGr;	/**< list of offset start of groups, total number of elements = numGroup */
	uint32_t *totByteGr;	/**< list total byte of groups */
	group_t *p_group;		/**< pointer to groups */
	
} effect_manage_t;

typedef struct
{			
	enum
	{
		SYNCHRONOUS,		/**< synchronize mode will be control by master*/
		ASYNCHRONOUS
	} mode;
	
	uint8_t brChannel[MAX_NUM_CHANNEL];
	
	uint8_t proup_number;		/**< group number want to run */
	uint32_t number_of_run;		/**< number of times to run effect*/
	
	union
	{
		
		struct
		{
			
			
		} synchronous;
		struct
		{
			
			
		} asynchronous;
	};

	
} effect_request_t;

#endif
