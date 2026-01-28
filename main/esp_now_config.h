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

#define PATH_GWAY_PEERS "/spiffs/gway_peers.bin"				/**< store gateway addr, info peers*/

#define TIME_BRC	5000
#define MAX_BRC_CNT 100
#define MAC_ADDR_LEN 6
#define CONFIG_ESPNOW_CHANNEL 1
#define NOW_SEND_CYCLE_MS 100

#define NOW_INDEX_HEADER 0
#define NOW_LEN_HEADER 3

#define NOW_INDEX_CMD (NOW_INDEX_HEADER + NOW_LEN_HEADER)
#define NOW_LEN_CMD 1

#define NOW_INDEX_DATA (NOW_INDEX_CMD + NOW_LEN_CMD)

#define NOW_INDEX_POS (NOW_INDEX_CMD + NOW_LEN_CMD)
#define NOW_LEN_POS 1

#define NOW_INDEX_ADDR (NOW_INDEX_POS + NOW_LEN_POS)
#define NOW_LEN_ADDR 6

#define NOW_LEN_CRC 2

typedef enum
{
	NOW_NONE				= 0,	
	NOW_ADD_PEER			= 1,
	NOW_GET_PEER			= 2,
	NOW_BRC					= 3,				/**< broadcast */
	NOW_FB_BRC				= 4,				/**< feedback broadcast */
	
	NOW_OPF					= 5,				/**< open file   */
	NOW_CLSF				= 6,				/**< close file  */
	NOW_DLTF				= 7,				/**< delete file */
	NOW_RDF					= 8,				/**< read file	 */
	NOW_WRF					= 9,				/**< write file  */
	
	NOW_RET_OPF				= 10,				/**< return open file	*/
	NOW_RET_CLSF			= 11,				/**< return close file	*/
	NOW_RET_DLTF			= 12,				/**< return delete file	*/
	NOW_RET_RDF				= 13,				/**< return read file	*/
	NOW_RET_WRF				= 14,				/**< return write file	*/
	
	NOW_ACK					= 15,
	NOW_NACK				= 16
} command_espnow_t; 

typedef struct
{
	void* data;
	uint32_t len;		/**< total byte */
} buf_espnow_t;

typedef struct
{
	uint8_t position;
	uint8_t addr[6];
	buf_espnow_t buf;
	uint32_t retry_cnt;			/**< number of times the sent callback is called */
	
} espnow_send_queue_t;

typedef struct espnow_send_buf
{
	buf_espnow_t buf;
	uint32_t retry_cnt;			/**< number of times the sent callback is called */
	struct espnow_send_buf* next;
	
} espnow_send_node_t;

typedef struct Peer
{
	esp_now_peer_info_t info;
	struct
	{
		espnow_send_node_t* p_hnode;
		
	} send;
	
	struct
	{
		buf_espnow_t* buf;
		uint8_t tot_buf;
	} recv;
	
	uint8_t position;				/**< position of peer in system */
	
} Peer_Typedef;

typedef struct My_Esp_Now
{	
	bool gateway_added; 
	Peer_Typedef* p_peer; 
	
	bool can_send;
	
	uint32_t tot_pos_added;				/**< total position added */ 
	
	uint8_t my_pos;
	
	bool is_broadcast;
	
	struct
	{
		buf_espnow_t buf;
		
	} recv;
	
} My_Esp_Now_Typedef;


void init_espnow(void); 
uint8_t espnow_add_peer(uint8_t* peer_addr, uint8_t position, bool save, const char* path);    
bool is_same_macadrr(const uint8_t *mac1, const uint8_t *mac2);  
void clear_all_peer(void);
uint16_t crc16_modbus(uint8_t *buf, uint32_t len); 
buf_espnow_t espnow_make_seg_cmd(command_espnow_t cmd, void* buf, uint32_t len);  
uint8_t espnow_make_node_send(Peer_Typedef* p_peer, espnow_send_queue_t q_send);  
void espnow_swt_node_send(Peer_Typedef* p_peer); 
void espnow_free_all_node(Peer_Typedef* p_peer);
#endif