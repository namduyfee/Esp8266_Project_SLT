
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

#define MAX_OJECT_FOR_A_NODE MAX_NUM_CHANNEL
#define MAX_NUM_OF_GROUP 8

#define UNIT_OF_TIME_EXIST	100 // 1 time exist = 100 ms

typedef enum
{
	EFF_SYNCHRONOUS = 0, /**< synchronize mode will be control by master*/
	EFF_ASYNCHRONOUS	
		
} effect_mode_t;
typedef struct 
{
	uint8_t numPin;				/**< number of pins of object */
	uint8_t p_pin[MAX_NUM_CHANNEL];				/**< pin list of object, total number of elements = numPin */
	uint8_t* brNessofState;			/**< brightness of object at each state, total number of elements = numState */
} object_t;	

typedef struct
{
	uint16_t numState;		/**< number of states of this group */
	uint8_t* p_timExistOfSta;	/**< time of each state, total number of elements = numState */
	uint8_t numObject;		/**< number of objects of this group */
	object_t p_object[MAX_OJECT_FOR_A_NODE];		/**< total number of elements = numObject */
} group_t;

typedef struct
{
	uint8_t brNess;			/**< synchronization brightness for groups, devices */
	uint8_t speedEf;		/**< synchronization speed for groups, devices */
	uint8_t numGroup;		/**< number of Group */
	uint32_t offStartGr[MAX_NUM_OF_GROUP];	/**< list of offset start of groups, total number of elements = numGroup */
	uint32_t totByteGr[MAX_NUM_OF_GROUP];	/**< list total byte of groups */
	group_t p_group[MAX_NUM_OF_GROUP];		/**< pointer to groups */
	
	effect_mode_t mode;
	
} effect_manage_t;

typedef struct
{			
	effect_mode_t mode;
	
	uint8_t number_of_group;	/** number of group want to request */
	uint8_t *gproup_number;		/**< group number list want to run */
	uint16_t *state_number;		/** state number of group list want to run */

} effect_request_t;

#endif
