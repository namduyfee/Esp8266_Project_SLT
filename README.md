

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

TCP:

* delete, open, close:
- 'TCP' + CMD (D O C). 
-> return: 'TCP' + CMD (TCP_ACK | TCP_NACK)

* start write
- 'TCP' + CMD (TCP_ST_WRITE) + offset_start + total byte write
-> return: TCP + CMD (TCP_ACK | TCP_NACK) + max segment size (max size of segment tcp)

* wirte
- 'TCP' + CMD (WRITE) + offset + data
...

* end write
- 'TCP' + CMD (END WRITE) + checksum 
-> return: 'TCP' + CMD (TCP_ACK | TCP_NACK)


