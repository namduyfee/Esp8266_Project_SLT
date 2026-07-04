# ESP8266 PWM Sync Protocol

Tai lieu nay mo ta format lenh TCP va ESP-NOW dang duoc dung trong project.

Quy uoc chung:

- Cac so nhieu byte dung little-endian, byte thap truoc.
- `u8` = 1 byte, `u16` = 2 byte, `u32` = 4 byte.
- `data...` la day byte co do dai tuy lenh.
- TCP la stream, mot lan `send()` co the bi tach/gop. ESP dung header + length de tach frame.

## TCP

### Frame chung

Moi frame TCP co dang:

```text
'T' 'C' 'P' CMD LEN0 LEN1 LEN2 LEN3 PAYLOAD...
```

Trong do:

- Magic: 3 byte ASCII `TCP`.
- `CMD`: u8, ma lenh TCP.
- `LEN`: u32 little-endian, do dai payload.
- `PAYLOAD`: du lieu cua lenh, dai dung bang `LEN`.

Gioi han hien tai:

- `TCP_RX_BUF_SIZE = 4096`.
- `TCP_FRAME_HEADER_LEN = 8`.
- Payload toi da cua mot frame: `4088` byte.
- Rieng `TCP_WRF`: payload gom `offset(4) + chunk_data`, nen `chunk_data` toi da la `4084` byte.
- Nen gui chunk file khoang `1024` hoac `1400` byte de nhe RAM hon.

### Ma lenh TCP

| Ten | Gia tri | Huong | Ghi chu |
| --- | ---: | --- | --- |
| `TCP_NONE` | 0 | - | Khong dung |
| `TCP_OPF` | 1 | App -> ESP | Open file `/spiffs/tcp.bin` |
| `TCP_CLSF` | 2 | App -> ESP | Close file |
| `TCP_DLTF` | 3 | App -> ESP | Delete file |
| `4` | 4 | - | Da bo chuc nang read file, de trong |
| `TCP_ST_WRF` | 5 | App -> ESP | Start write file |
| `TCP_WRF` | 6 | App -> ESP | Write mot chunk file |
| `TCP_END_WRF` | 7 | App -> ESP | Ket thuc write va check checksum |
| `8`, `9` | 8, 9 | - | Da bo read response, de trong |
| `TCP_EFF_SYNCH` | 10 | App -> ESP | Chuyen effect sang synchronous |
| `TCP_EFF_ASYNCH` | 11 | App -> ESP | Chuyen effect sang asynchronous |
| `TCP_GET_INF_ESP_MODE` | 12 | App -> ESP, ESP -> App | Lay thong tin ESP-NOW mode |
| `TCP_SET_INF_ESP_MODE` | 13 | App -> ESP | Set ESP-NOW mode/id/project |
| `TCP_RETURN_ID_RECEIVED` | 14 | ESP -> App | Tra bitmask cac node da nhan effect |
| `TCP_ACK` | 15 | ESP -> App | Lenh thanh cong |
| `TCP_NACK` | 16 | ESP -> App | Lenh loi/khong hop le |

### ACK/NACK TCP

ACK/NACK la frame TCP binh thuong, nhung `CMD` la `TCP_ACK` hoac `TCP_NACK`.

Payload ACK/NACK luon bat dau bang lenh goc:

```text
PAYLOAD = response_to_cmd(u8) + optional_data...
```

Vi du:

```text
Open file OK:
CMD = TCP_ACK
PAYLOAD = TCP_OPF

Open file fail:
CMD = TCP_NACK
PAYLOAD = TCP_OPF

Start write OK:
CMD = TCP_ACK
PAYLOAD = TCP_ST_WRF + max_segment_size(u32)
```

### Lenh file TCP

#### `TCP_OPF`

Open file TCP.

```text
PAYLOAD: rong
Response: TCP_ACK/TCP_NACK + TCP_OPF
```

#### `TCP_CLSF`

Close file TCP.

```text
PAYLOAD: rong
Response: TCP_ACK/TCP_NACK + TCP_CLSF
```

#### `TCP_DLTF`

Delete file TCP.

```text
PAYLOAD: rong
Response: TCP_ACK/TCP_NACK + TCP_DLTF
```

#### `TCP_ST_WRF`

Bat dau ghi file.

```text
PAYLOAD:
offset_start(u32)
total_byte(u32)

Response OK:
TCP_ACK + TCP_ST_WRF + max_segment_size(u32)

Response fail:
TCP_NACK + TCP_ST_WRF
```

`offset_start` la vi tri bat dau ghi. Neu ghi file moi tu dau thi de `0`.

#### `TCP_WRF`

Ghi mot chunk du lieu.

```text
PAYLOAD:
offset(u32)
chunk_data...
```

Ghi chu:

- `offset` la vi tri ghi tinh theo file dich.
- ESP gom cac chunk theo `total_byte` da khai bao o `TCP_ST_WRF`.
- Hien tai ESP khong ACK tung chunk. ACK `TCP_WRF` duoc gui khi so byte con lai ve `0`.
- Neu file chua open hoac du lieu khong hop le, ESP co the gui `TCP_NACK + TCP_WRF`.

#### `TCP_END_WRF`

Ket thuc ghi file va so checksum.

```text
PAYLOAD:
checksum(u16)

Response:
TCP_ACK/TCP_NACK + TCP_END_WRF
```

Checksum la CRC16 Modbus tinh tren toan bo data da gui trong cac chunk `TCP_WRF`, init `0xffff`.

Sau khi `TCP_END_WRF` thanh cong, ESP master se phat file effect xuong cac node bang ESP-NOW.

### Lenh cau hinh ESP-NOW qua TCP

#### `TCP_GET_INF_ESP_MODE`

Lay thong tin mode/id/project hien tai.

```text
Request payload: rong

Response CMD: TCP_GET_INF_ESP_MODE
Response payload:
mode(u8)
my_id(u8)
my_code_ascii(8 byte)
gw_code_ascii(8 byte)
```

`mode`: `0 = ESP_NOW_MASTER`, `1 = ESP_NOW_SLAVE`.

#### `TCP_SET_INF_ESP_MODE`

Set mode/id/project.

```text
Request payload:
mode(u8)
my_id(u8)
gw_code_ascii(8 byte)

Response:
TCP_ACK/TCP_NACK + TCP_SET_INF_ESP_MODE
```

`gw_code_ascii` la chuoi hex 8 ky tu, vi du `12AB34CD`.

### Lenh mode effect qua TCP

#### `TCP_EFF_SYNCH`

Chuyen effect sang synchronous.

```text
Request payload: rong
Response: TCP_ACK + TCP_EFF_SYNCH
```

#### `TCP_EFF_ASYNCH`

Chuyen effect sang asynchronous.

```text
Request payload: rong
Response: TCP_ACK + TCP_EFF_ASYNCH
```

### `TCP_RETURN_ID_RECEIVED`

Sau khi master gui effect xuong cac node, ESP tra ve bitmask node da nhan thanh cong.

```text
CMD: TCP_RETURN_ID_RECEIVED
PAYLOAD:
bit_mask_id_esp(u32)
```

Bit `n = 1` nghia la node co id `n` da nhan file/effect thanh cong.

## ESP-NOW

### Frame chung

Frame ESP-NOW tren duong truyen:

```text
GW_CODE CMD PAYLOAD... CRC16
```

Trong do:

- `GW_CODE`: u32 little-endian, project id/gateway code. Chi ESP cung `gw_code` moi nhan frame.
- `CMD`: u8, ma lenh ESP-NOW.
- `PAYLOAD`: du lieu lenh.
- `CRC16`: u16 little-endian, CRC16 Modbus init `0xffff`, tinh tren `GW_CODE + CMD + PAYLOAD`.

Khi frame vao queue noi bo, CRC da duoc kiem tra va cat bo.

### Ma lenh ESP-NOW

| Ten | Gia tri | Huong | Ghi chu |
| --- | ---: | --- | --- |
| `NOW_NONE` | 0 | - | Khong dung |
| `NOW_BRC` | 1 | Master -> broadcast | Broadcast master |
| `NOW_ADD_PEER` | 2 | Slave -> master | Xin add peer |
| `NOW_OPF` | 3 | Master -> slave | Open effect file |
| `NOW_CLSF` | 4 | Master -> slave | Close effect file |
| `NOW_DLTF` | 5 | Master -> slave | Delete effect file |
| `NOW_ST_WRF` | 6 | Master -> slave | Start write effect |
| `NOW_WRF` | 7 | Master -> slave | Write packet effect |
| `NOW_END_WRF` | 8 | Master -> slave | End write/checksum |
| `NOW_EFF_SYNC` | 9 | Master -> slave | Dong bo state effect |
| `NOW_EFF_ASYNC` | 10 | Master -> slave | Chay effect async |
| `NOW_ACK` | 11 | Slave -> master | Lenh thanh cong |
| `NOW_NACK` | 12 | Slave -> master | Lenh loi/can gui lai |

### Pairing/broadcast

#### `NOW_BRC`

Master broadcast id cua no.

```text
PAYLOAD:
master_id(u8)
```

Slave nhan frame nay se luu gateway peer va gui `NOW_ADD_PEER`.

#### `NOW_ADD_PEER`

Slave gui id cua no ve master.

```text
PAYLOAD:
sender_id(u8)
```

Master them MAC/id vao danh sach peer.

### File/effect transfer bang ESP-NOW

Trinh tu chuan:

```text
NOW_OPF
NOW_ST_WRF
NOW_WRF ... NOW_WRF
NOW_END_WRF
NOW_CLSF
```

#### `NOW_OPF`

Mo file effect tren slave.

```text
PAYLOAD: rong
Response: NOW_ACK/NOW_NACK + NOW_OPF
```

#### `NOW_CLSF`

Dong file effect tren slave.

```text
PAYLOAD: rong
Response: NOW_ACK/NOW_NACK + NOW_CLSF
```

#### `NOW_DLTF`

Xoa file effect tren slave.

```text
PAYLOAD: rong
Response: NOW_ACK/NOW_NACK + NOW_DLTF
```

#### `NOW_ST_WRF`

Bat dau ghi file effect tren slave.

```text
PAYLOAD:
total_packet(u32)
offset_start(u32)
total_byte(u32)

Response:
NOW_ACK/NOW_NACK + NOW_ST_WRF
```

`total_packet` la so packet `NOW_WRF` se gui tiep theo.

#### `NOW_WRF`

Gui mot packet data.

```text
PAYLOAD:
packet_number(u32)
offset(u32)
data...
```

Ghi chu:

- `packet_number` dung de slave phat hien packet nao bi thieu.
- `offset` la vi tri ghi trong file effect tam.
- Master tinh kich thuoc data toi da theo gioi han ESP-NOW:

```text
max_data = ESP_NOW_MAX_DATA_LEN
         - NOW_SZOF_HEADER
         - NOW_SZOF_CMD
         - NOW_SZOF_CRC
         - NOW_SZOF_OFFSET
         - NOW_SZOF_PACKET_NUM
```

#### `NOW_END_WRF`

Bao da gui xong file effect.

```text
PAYLOAD:
checksum(u16)
```

Neu du packet va checksum dung:

```text
Response: NOW_ACK + NOW_END_WRF
```

Neu thieu packet:

```text
Response: NOW_NACK + NOW_END_WRF + packet_number_1(u32) + packet_number_2(u32) + ...
```

Neu loi nang hoac can gui lai toan bo:

```text
Response: NOW_NACK + NOW_END_WRF + 0xffffffff(u32)
```

### Effect sync/async bang ESP-NOW

#### `NOW_EFF_SYNC`

Master gui danh sach group can update va state moi cua tung group.

```text
PAYLOAD:
total_group(u8)
group_id[total_group]
state[total_group]      // moi state la u16
```

Thu tu `state[]` ung voi thu tu `group_id[]`.

#### `NOW_EFF_ASYNC`

Master yeu cau cac slave chay effect theo async mode.

```text
PAYLOAD:
total_group(u8)
group_id[total_group]
state[total_group]      // moi state la u16, thuong la 0 khi bat dau
```

### ACK/NACK ESP-NOW

ACK/NACK ESP-NOW cung co payload bat dau bang lenh goc:

```text
NOW_ACK + original_cmd(u8)
NOW_NACK + original_cmd(u8) + optional_data...
```

Vi du:

```text
NOW_ACK + NOW_OPF
NOW_NACK + NOW_END_WRF + packet_number_list...
```

## Ghi chu van hanh

- TCP server hien chi cho 1 client ket noi cung luc.
- TCP auto disconnect sau `TCP_AUTO_DIS_MS = 40000 ms` neu khong co du lieu.
- ESP-NOW khong ma hoa (`encrypt = false`).
- ESP-NOW dung channel `CONFIG_ESPNOW_CHANNEL = 1`.
- `gw_code` la project id: ESP khac `gw_code` se bo qua frame ESP-NOW.
- CRC dung trong ESP-NOW la CRC16 Modbus, init `0xffff`.
