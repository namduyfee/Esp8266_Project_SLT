#ifndef SPIFFS_CONFIG
#define SPIFFS_CONFIG

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

#include "nvs_flash.h"
#include "esp_spiffs.h"


#define PATH_EFFECT "/spiffs/effect.bin"
#define PATH_EFFECT_TMP "/spiffs/effect_tmp.bin"

#define PATH_TCP_FILE "/spiffs/tcp.bin"
#define PATH_TCP_FILE_TMP "/spiffs/tcp_tmp.bin"


#define POS_CONTINUE -1
#define REMAINING -1

typedef enum
{
	F_NONE_SOURCE = 0,
	F_TCP_SOURCE,
	F_NOW_SOURCE
} file_source_req;

typedef enum
{
	F_NONE    = 0,
	F_OP,
	F_CLS,
	F_DLT, 
	
	F_ST_WR,
	F_WR,
	F_END_WR,
		
} file_command_t;

typedef struct
{
	void* data;
	uint32_t tot_byte;
	
} file_buf_t;

typedef struct 
{
	file_command_t cmd;			/**< command with file */				
	file_source_req source;
	union
	{
		struct
		{
			off_t offset;
			uint32_t tot_byte;				/**< total byte to write */
		} write_start;
		struct
		{
			off_t offset;				/**< offset of request */
			file_buf_t buf;				/**< content and len of segment */
			
		} write;
		struct
		{
			uint16_t checksum;
		} write_end;
	};

} file_request_t;

typedef struct
{

	struct
	{
		off_t offset_start;				/**< the fisrt offset is written after recv message */
		int tot_byte;
		int remaining;					/**< remaining bytes to write */
		uint16_t checksum;
		
	} write;
	
	struct
	{

	} read;
	
} file_mana_t;


void spiffs_init(void);
void spiffs_read_file(const char *path, char *data, uint32_t data_len);


#endif
