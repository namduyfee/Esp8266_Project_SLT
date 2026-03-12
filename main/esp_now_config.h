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

#define TIME_BRC	5000
#define MAX_PEER 20

#define NOW_ID_BRC (0xFF)

#define PATH_PEERS_ADDED "/spiffs/peers_added.bin"				/**< info peers is added */
#define PATH_GWAY_ADDR	 "/spiffs/gway_addr.bin"				/**< gateway address */

#define MAC_ADDR_LEN 6
#define CONFIG_ESPNOW_CHANNEL 1
#define NOW_SEND_CYCLE_MS 100

#define NOW_INDEX_HEADER 0
#define NOW_LEN_HEADER 3

#define NOW_INDEX_CMD (NOW_INDEX_HEADER + NOW_LEN_HEADER)
#define NOW_LEN_CMD 1

#define NOW_INDEX_PAYLOAD (NOW_INDEX_CMD + NOW_LEN_CMD)

#define NOW_LEN_CRC 2


typedef enum
{
	NOW_NONE				= 0,	
	NOW_ADD_PEER,
	NOW_GET_PEER,
	NOW_BRC,					/**< broadcast */
	NOW_FB_BRC,					/**< feedback broadcast */
	
	NOW_OPF,					/**< open file   */
	NOW_CLSF,					/**< close file  */
	NOW_DLTF,					/**< delete file */
	NOW_RDF,					/**< read file	 */
	
	NOW_ST_WRF,
	NOW_WRF,					/**< write file  */
	NOW_END_WRF,
	
	NOW_RET_OPF,				/**< return open file	*/
	NOW_RET_CLSF,				/**< return close file	*/
	NOW_RET_DLTF,				/**< return delete file	*/
	
	NOW_ST_RET_RDF,
	NOW_RET_RDF,				/**< return read file	*/
	NOW_END_RET_RDF,
	
	NOW_RET_WRF,				/**< return write file	*/
	
	NOW_ACK,
	NOW_NACK
} command_espnow_t; 

typedef struct
{
	uint8_t mac[6];
	uint8_t id;
} peer_info_t;

typedef struct
{
	uint8_t* data;
	uint32_t len;		/**< total byte */
	
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

typedef struct My_Esp_Now
{	
	peer_info_t gw_peer;
	peer_info_t peer_list[MAX_PEER]; 
	uint8_t cnt_id_added;				/**< total position added */
	
	uint8_t my_id;
	
} My_Esp_Now_Typedef;


void init_espnow(void); 
void espnow_add_peer(uint8_t* peer_addr, uint8_t id);     
bool is_same_macadrr(const uint8_t *mac1, const uint8_t *mac2);  
void clear_all_peer(void);
uint16_t crc16_modbus(uint16_t crc, uint8_t *buf, uint32_t len);  

buf_espnow_t espnow_make_frame_send(void* payload, uint32_t len_payload, command_espnow_t cmd);  

#endif