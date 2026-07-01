
MASTER SELECT:
- master hoạt động ở mode ap, slave thì mode sta.
- sử dụng nút nhấn để chọn esp là master và phát broadcast
- các esp sau khi nhấn nút thì nó sẽ phát broadcast ở mức ưu tiên cao trong 5s, đưa tất cả esp khác về slave
sau đó phát định kì ở mức ưu tiên thâp.

DISPLAY ID:
- id của esp sẽ hiện thì dựa trên tổng của channel, dải channel là 0 -> 7.
ví dụ id là 14 thì = 7 + 6 + 1 => đèn trên channel 7, 6, 1 sẽ cùng sáng.



ESPNOW:

 *	frame				 : KEY + CMD + DATA_1, DATA_2, ... DATA_N + CHECKSUM 
 	
 -		BROADCAST		 : KEY + NOW_BRC  (CMD) + ID MASTER + CHECKSUM
 -		ADD_PEER		 : KEY + NOW_ADD_PEER (CMD) + ID SENDER + CHECKSUM
 		
 -		OPF, CLF, DLF	 : KEY + CMD (NOW_OPF, NOW_CLSF, NOW_DLTF) + CHECKSUM
 		
 -		START_WWRITE	 : KEY + NOW_ST_WRF (CMD) + TOTAL_PACKET + OFFSET_START + TOTAL_BYTE + CHECKSUM
 -		WRITE			 : KEY + NOW_WRF (CMD) + INDEX PACKET + OFFSET_WR + DATA1, .. DATAN + CHECKSUM
 -		NOW_END_WRF		 : KEY + NOW_END_WRF (CMD) + CHECKSUM_DATA_SENT + CHECKSUM
 		
 -		EFF_SYNC, EFF_ASYNC:	KEY + CMD (NOW_EFF_SYNC or NOW_EFF_ASYNC) + TOTAL_GROUP + ...(list index group) + ...(list state of group) + CHECKSUM
 -		
 -		ACK, NACK		 : KEY + CMD (ACK or NACK) + CMD_RETURN (NOW_OPF, NOW_CLSF, ...) + CHECKSUM

 * NOTE:
	+ KEY bao gồm 3 byte, những esp cùng KEY thì mới truyền được tin cho nhau
	

TCP:

* Frame
- delete, open, close:
 'TCP' + CMD (D O C). 
 return: 'TCP' + CMD (TCP_ACK | TCP_NACK)

- start write
 'TCP' + CMD (TCP_ST_WRITE) + offset_start + total byte write
 return: TCP + CMD (TCP_ACK | TCP_NACK) + max segment size (max size of segment tcp)

- wirte
 'TCP' + CMD (WRITE) + offset + data
...

- end write
 'TCP' + CMD (END WRITE) + CHECKSUM
 return: 'TCP' + CMD (TCP_ACK | TCP_NACK)

- effect synch , asynch 
 'TCP' + CMD (TCP_EFF_SYNCH | TCP_EFF_ASYNCH) + số lượng group update + danh sách group update + danh sách trạng thái cần update tương ứng theo thứ tự danh sách group.

* NOTE: 
	+ mỗi thời điểm chỉ có một thiết bị kết nối và truyên file từ app xuống esp master. 

- set infor esp mode
 'TCP' + CMD (TCP_SET_INF_ESP_MODE) + mode (ESP_NOW_MASTER : 0 or ESP_NOW_SLAVE : 1) + my_id + gw_code

- get infor esp mode
 'TCP' + CMD (TCP_GET_INF_ESP_MODE)
 return 'TCP' + CMD (TCP_GET_INF_ESP_MODE) +  mode (ESP_NOW_MASTER : 0 or ESP_NOW_SLAVE : 1) + my_id + my_code + gw_code