
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

#define PATH_EFFECT "/spiffs/effect.bin"
#define PATH_EFFECT_TMP "/spiffs/effect_tmp.bin"

typedef struct 
{
	uint8_t numPin;				/**< number of pins of object */
	uint8_t* p_pin;				/**< pin list of object, total number of elements = numPin */
	uint8_t* brNess;			/**< brightness of object at each state, total number of elements = numState */
} object_t;	

typedef struct
{
	uint16_t numState;		/**< number of states of this group */
	uint8_t* p_timOfSta;	/**< time of each state, total number of elements = numState */
	uint8_t numObject;		/**< number of objects of this group */
	uint16_t* totByDesOb;	/**< total byte to descrip each object, total number of elements = numObject */
	object_t* p_object;		/**< total number of elements = numObject */
} group_t;

typedef struct
{
	uint8_t brNess;			/**< synchronization brightness for groups, devices */
	uint8_t speedEf;		/**< synchronization speed for groups, devices */
	uint8_t numGroup;		/**< number of Group */
	uint32_t* p_adMaGroup;	/**< list of group management address, total number of elements = numGroup */
	group_t* p_group;		/**< list group */
	
} effect_manage_t;

#endif
