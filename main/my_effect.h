
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

typedef struct 
{
	uint16_t totByteDes;	/**< total byte to descriptor this object */
	uint8_t* p_pin;			/**< pin list of object */
	uint8_t numPin;			/**< number of pins of object */
	
} object_t;

typedef struct
{
	object_t* p_object;		
	uint8_t numObject;		/**< number of objects of this group */
	uint16_t numState;		/**< number of states of this group */
	uint8_t* p_timOfSta;	/**< time of each state */
	uint8_t* p_brNesOfSta;	/**< bright ness of each state */
	uint16_t totByteDes;	/**< total byte of descriptor this group */
	uint16_t adManage;		/**< address manage this group */
	
} group_t;

typedef struct
{
	uint8_t speedEf;
	uint8_t brNess;			/**< */
	group_t* p_group;		/**< list group */
	uint8_t numGroup;		/**< number of Group*/
	
} effect_mana_t;




#endif
