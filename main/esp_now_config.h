#ifndef ESP_NOW_CONFIG
#define ESP_NOW_CONFIG

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

#include "esp_wifi.h"
#include "esp_now.h"
#include "my_tcpip.h"

#define MAX_PEER 20

#define NOW_ID_BRC (0xFF)

#define PATH_PEERS_ADDED "/spiffs/peers_added.bin"				/**< info peers is added */
#define PATH_GWAY_ADDR	 "/spiffs/gway_addr.bin"				/**< gateway address */

#define MAC_ADDR_LEN 6
#define CONFIG_ESPNOW_CHANNEL 1
#define NOW_SEND_CYCLE_MS 100

#define NOW_INDEX_HEADER 0
#define NOW_SZOF_HEADER 4

#define NOW_INDEX_CMD (NOW_INDEX_HEADER + NOW_SZOF_HEADER)
#define NOW_SZOF_CMD 1

#define NOW_INDEX_PAYLOAD (NOW_INDEX_CMD + NOW_SZOF_CMD)

#define NOW_SZOF_CRC 2

#define NOW_SZOF_OFFSET 4
#define NOW_SZOF_PACKET_NUM 4

/** CMD size is use with 1byte not 4byte that default of enum */
typedef enum
{
	NOW_NONE	= 0,
	NOW_BRC,			/**< broadcast */
	NOW_ADD_PEER,
	
	NOW_OPF,			/**< open file   */
	NOW_CLSF,			/**< close file  */
	NOW_DLTF,			/**< delete file */
	
	NOW_ST_WRF,			/**< start write request */
	NOW_WRF,			/**< write file  */
	NOW_END_WRF,		/**< end write file */
	
	NOW_EFF_SYNC,		/**< EFFECT SYNCH REQUEST */
	NOW_EFF_ASYNC,		/**< EFFECT ASYNCH REQUEST */
	
	
	NOW_ACK,
	NOW_NACK
} command_espnow_t; 

typedef enum
{
	ESP_NOW_MASTER = 0,
	ESP_NOW_SLAVE
} espnow_mode_t;
typedef struct
{
	uint8_t mac[6];
	uint8_t id;
} peer_info_t;

typedef struct
{
	uint8_t* data;
	uint32_t tot_byte;		/**< total byte */
	
} buf_espnow_t;

typedef struct
{
	buf_espnow_t buf;
	uint8_t addr[6];			/**< addrest source */
} espnow_recv_queue_t;

typedef struct
{
	uint8_t dest_id;			/**< destination id */
	buf_espnow_t buf;
	uint32_t tmout_ms;			/**< timeout if can not send */
	
} espnow_send_queue_t;
typedef struct
{
	uint32_t offset;
	uint8_t* data;
	uint32_t tot_byte;
	
} espnow_wrf_packet_t;
typedef struct
{
	uint32_t tot_req_pack;
	uint32_t* request;
		
} espnow_wrf_resend;

typedef struct My_Esp_Now
{	
	peer_info_t gw_peer;
	uint32_t gw_code;
	
	peer_info_t peer_list[MAX_PEER];	/**< info peers added */
	uint8_t cnt_id_added;				/**< total id added */
	
	uint8_t my_id;
	uint32_t my_code;
	
	int8_t mode;
	
	bool send_success;
	
	uint8_t state_return;
	struct
	{
		espnow_wrf_packet_t* p_packet;
		uint32_t tot_packet;
		uint32_t offset_st; 
		uint32_t tot_byte;
		
	} mana_recv_wrf_mess;
	
	bool brc_new;
	
} My_Esp_Now_Typedef;


void init_espnow(void); 
void espnow_add_peer(uint8_t* peer_addr, uint8_t id);     
bool is_same_macadrr(const uint8_t *mac1, const uint8_t *mac2);  
void clear_all_peer(void);
uint16_t crc16_modbus(uint16_t crc, uint8_t *buf, uint32_t len);  

buf_espnow_t espnow_make_frame_send(void* payload, uint32_t len_payload, command_espnow_t cmd);  

#endif