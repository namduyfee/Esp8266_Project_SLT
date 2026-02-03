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

#define POS_CONTINUE -1
#define REMAINING -1

typedef enum
{
	F_TCP_SOURCE = 0,
	F_NOW_SOURCE
} file_source_req;

typedef enum
{
	F_NONE    = 0,
	F_OP,
	F_CLS,
	F_DLT, 
	F_RD,
	F_WR,
	
	F_RET_OP,
	F_RET_CLS,
	F_RET_DLT, 
	F_RET_RD,
	F_RET_WRT
		
} file_command_t;

typedef struct
{
	void* data;
	uint32_t len;			/**< total bytes of memory that is pointed by data */
	
} file_buf_t;

typedef struct 
{
	file_command_t cmd;			/**< command with file */
	off_t offset;				/**< offset of request */
	file_source_req source;
	union
	{
		struct
		{
			file_buf_t buf;				/**< content and len of segment */
			int tot_len;				/**< total length message */
		} write;
		struct
		{
			int len;				/**< total length message */
		} read;
	};
	
} file_request_t;

typedef struct
{
	file_command_t cmd_cur;				/**< command current */
	
	struct
	{
		off_t offset_last;					/**< lastest offset after write */
		off_t offset_start;				/**< the fisrt offset is written after recv message */
		int tot_len;					/**< total length message */
		int remaining;					/**< remaining bytes to write */
	} write;
	struct
	{
		off_t off_last;					/**< lastest offset after read */
	} read;
	
} file_mana_t;


void spiffs_init(void);
void spiffs_read_file(const char *path, char *data, uint32_t data_len);


#endif