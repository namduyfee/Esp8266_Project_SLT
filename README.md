# ESP8266 PWM Sync - Tai Lieu Protocol

Tai lieu nay mo ta format lenh TCP va ESP-NOW trong project ESP8266 dieu khien PWM GPIO. Master nhan file effect qua TCP, sau do tach data cho tung node va dong bo effect sang slave bang ESP-NOW.

## 1. Tong Quan

- TCP dung de app/PC gui lenh va file effect den ESP master qua port `80`.
- ESP-NOW dung de master broadcast, add peer, gui file effect cho slave va gui state sync/asyn.
- Cac ESP cung project duoc loc bang `gw_code` trong header ESP-NOW.
- Tat ca so nhieu byte trong payload hien tai duoc ghi theo little-endian, tru khi ghi chu khac.
- CRC dung CRC16 Modbus, init `0xffff`.

## 2. TCP Frame

TCP la stream, nen mot lan `send()` co the chua nua frame, mot frame, hoac nhieu frame. Firmware tach frame theo header:

```text
Byte 0..2 : 'T' 'C' 'P'
Byte 3    : CMD(u8)
Byte 4..7 : LEN(u32 little-endian)
Byte 8..  : PAYLOAD[LEN]
```

Gioi han:

- `TCP_FRAME_HEADER_LEN = 8`.
- `TCP_RX_BUF_SIZE = 4096`.
- `TCP_MAX_PAYLOAD_LEN = 4088`.
- `TCP_WRF` payload = `offset(u32) + chunk_data`, nen `chunk_data` toi da ly thuyet la `4084` byte.
- Nen gui chunk khoang `1024..1400` byte de nhe RAM va de TCP/ESP8266 on dinh hon.

## 3. TCP Command

Gia tri command theo `main/my_tcpip.h`.

| CMD | Value | Huong | Muc dich |
| --- | ---: | --- | --- |
| `TCP_NONE` | 0 | - | Khong dung |
| `TCP_OPF` | 1 | App -> ESP | Open file tong `/spiffs/tcp.bin` |
| `TCP_CLSF` | 2 | App -> ESP | Close file tong |
| `TCP_DLTF` | 3 | App -> ESP | Delete file tong |
| `TCP_ST_WRF` | 4 | App -> ESP | Start write file |
| `TCP_WRF` | 5 | App -> ESP | Write chunk file |
| `TCP_END_WRF` | 6 | App -> ESP | End write, check checksum |
| `TCP_EFF_SYNCH` | 7 | App -> Master | Chuyen effect sang synchronous |
| `TCP_EFF_ASYNCH` | 8 | App -> Master | Chuyen effect sang asynchronous |
| `TCP_GET_INF_ESP_MODE` | 9 | App -> ESP | Doc mode/id/project code |
| `TCP_SET_INF_ESP_MODE` | 10 | App -> ESP | Ghi mode/id/project code |
| `TCP_RETURN_ID_RECEIVED` | 11 | Master -> App | Bitmask node da nhan file effect |
| `TCP_ACK` | 12 | ESP -> App | Lenh thanh cong |
| `TCP_NACK` | 13 | ESP -> App | Lenh loi |

## 4. TCP ACK/NACK

ACK/NACK cung la TCP frame binh thuong:

```text
TCP ACK frame:
'T' 'C' 'P' TCP_ACK LEN PAYLOAD

TCP NACK frame:
'T' 'C' 'P' TCP_NACK LEN PAYLOAD
```

Payload chung:

```text
response_to_cmd(u8)
optional_data...
```

Vi du:

```text
ACK cho TCP_OPF:
CMD = TCP_ACK
PAYLOAD = TCP_OPF

NACK cho TCP_END_WRF:
CMD = TCP_NACK
PAYLOAD = TCP_END_WRF
```

## 5. TCP Payload Tung Lenh

### `TCP_OPF`

Open file tong tren master.

```text
Request payload:
<empty>

Response:
TCP_ACK  payload = TCP_OPF
TCP_NACK payload = TCP_OPF
```

### `TCP_CLSF`

Close file tong.

```text
Request payload:
<empty>

Response:
TCP_ACK  payload = TCP_CLSF
TCP_NACK payload = TCP_CLSF
```

### `TCP_DLTF`

Xoa file tong `/spiffs/tcp.bin`.

```text
Request payload:
<empty>

Response:
TCP_ACK  payload = TCP_DLTF
TCP_NACK payload = TCP_DLTF
```

### `TCP_ST_WRF`

Bat dau ghi mot vung file. Firmware mo file tam `/spiffs/tcp_tmp.bin`.

```text
Request payload:
offset_start(u32)
total_byte(u32)

Response OK:
TCP_ACK payload = TCP_ST_WRF + max_segment_size(u32)

Response fail:
TCP_NACK payload = TCP_ST_WRF
```

Ghi chu:

- `offset_start = 0` va `total_byte > 2`: sau `TCP_END_WRF`, file tam thay the toan bo `/spiffs/tcp.bin`.
- `offset_start != 0`: sau `TCP_END_WRF`, firmware ghi file tam vao vi tri offset trong file hien co.
- `max_segment_size` hien tra ve `TCP_MSS`.

### `TCP_WRF`

Ghi mot chunk vao file tam.

```text
Request payload:
offset(u32)
chunk_data[0..N]

Response:
TCP_ACK payload = TCP_WRF    ; chi gui khi remaining == 0
TCP_NACK payload = TCP_WRF   ; neu loi queue/file/data
```

Ghi chu:

- Mot frame `TCP_WRF` chi nen chua mot chunk: `offset + chunk_data`.
- TCP co the gop nhieu frame vao cung mot callback, nhung moi frame van phai co header `TCP CMD LEN`.
- Checksum duoc tinh tren so byte firmware thuc su ghi.

### `TCP_END_WRF`

Ket thuc ghi file va check checksum.

```text
Request payload:
checksum(u16)

Response:
TCP_ACK  payload = TCP_END_WRF
TCP_NACK payload = TCP_END_WRF
```

Neu checksum dung:

- File tam duoc thay the hoac merge vao `/spiffs/tcp.bin`.
- Master trigger `task_update_effect_node()` de gui effect cho cac node.

### `TCP_EFF_SYNCH`

Chuyen master sang synchronous mode.

```text
Request payload:
<empty>

Response:
TCP_ACK  payload = TCP_EFF_SYNCH
TCP_NACK payload = TCP_EFF_SYNCH
```

Ghi chu:

- Master tinh state cua group va gui snapshot `NOW_EFF_SYNC` cho slave.
- Slave trong sync mode chi doi state khi nhan frame sync moi.

### `TCP_EFF_ASYNCH`

Chuyen master/slave sang asynchronous mode.

```text
Request payload:
<empty>

Response:
TCP_ACK  payload = TCP_EFF_ASYNCH
TCP_NACK payload = TCP_EFF_ASYNCH
```

Ghi chu:

- Master gui `NOW_EFF_ASYNC` de dua cac group ve state 0.
- Sau do moi ESP tu chay effect theo file local.

### `TCP_GET_INF_ESP_MODE`

Doc cau hinh ESP-NOW hien tai.

```text
Request payload:
<empty>

Response:
CMD = TCP_GET_INF_ESP_MODE
PAYLOAD:
mode(u8)
my_id(u8)
my_code_ascii_hex[8]
gw_code_ascii_hex[8]
```

Gia tri `mode`:

```text
0 = ESP_NOW_MASTER
1 = ESP_NOW_SLAVE
```

### `TCP_SET_INF_ESP_MODE`

Ghi cau hinh ESP-NOW va luu vao NVS.

```text
Request payload:
mode(u8)
my_id(u8)
gw_code_ascii_hex[8]

Response:
TCP_ACK  payload = TCP_SET_INF_ESP_MODE
TCP_NACK payload = TCP_SET_INF_ESP_MODE
```

Ghi chu:

- Payload toi thieu 10 byte.
- `gw_code_ascii_hex` la chuoi 8 ky tu hex, vi du `1234ABCD`.
- Sau khi set, firmware deinit/init lai ESP-NOW va clear peer cu.

### `TCP_RETURN_ID_RECEIVED`

Master tra ve sau khi gui file effect cho node.

```text
Response frame:
CMD = TCP_RETURN_ID_RECEIVED
PAYLOAD:
bit_mask_id_esp(u32)
```

Y nghia:

```text
bit n = 1: node id n da nhan effect thanh cong
bit n = 0: node id n chua nhan / gui loi / timeout
```

## 6. ESP-NOW Frame

ESP-NOW frame do project tao:

```text
Byte 0..3       : gw_code(u32)
Byte 4          : CMD(u8)
Byte 5..N-3     : PAYLOAD
Byte N-2..N-1   : CRC16(u16 little-endian)
```

CRC:

```text
crc = crc16_modbus(0xffff, frame_without_crc)
crc_low  = crc & 0xff
crc_high = crc >> 8
```

Gioi han:

- `CONFIG_ESPNOW_CHANNEL = 1`.
- `MAX_PEER = 20`.
- `NOW_ID_BRC = 0xff`.
- `ESP_NOW_MAX_DATA_LEN` la gioi han tong frame ESP-NOW.
- Max data cua `NOW_WRF`:

```text
ESP_NOW_MAX_DATA_LEN
- NOW_SZOF_HEADER(4)
- NOW_SZOF_CMD(1)
- NOW_SZOF_CRC(2)
- NOW_SZOF_PACKET_NUM(4)
- NOW_SZOF_OFFSET(4)
```

## 7. ESP-NOW Command

Gia tri command theo `main/esp_now_config.h`.

| CMD | Value | Huong | Muc dich |
| --- | ---: | --- | --- |
| `NOW_NONE` | 0 | - | Khong dung |
| `NOW_BRC` | 1 | Master -> Broadcast | Bao master/id gateway |
| `NOW_ADD_PEER` | 2 | Slave -> Master | Slave xin add peer |
| `NOW_OPF` | 3 | Master -> Slave | Open effect file |
| `NOW_CLSF` | 4 | Master -> Slave | Close effect file |
| `NOW_DLTF` | 5 | Master -> Slave | Delete effect file |
| `NOW_ST_WRF` | 6 | Master -> Slave | Start write effect |
| `NOW_WRF` | 7 | Master -> Slave | Write packet effect |
| `NOW_END_WRF` | 8 | Master -> Slave | End write/checksum |
| `NOW_EFF_SYNC` | 9 | Master -> Slave | Snapshot state sync |
| `NOW_EFF_ASYNC` | 10 | Master -> Slave | Chuyen async/state 0 |
| `NOW_ACK` | 11 | Slave -> Master | ACK cho lenh ESP-NOW |
| `NOW_NACK` | 12 | Slave -> Master | NACK cho lenh ESP-NOW |

## 8. ESP-NOW Payload Tung Lenh

### `NOW_BRC`

Master broadcast moi khoang 1 giay de slave tim gateway.

```text
Frame:
gw_code(u32)
NOW_BRC(u8)
master_id(u8)
crc(u16)
```

Slave xu ly:

- Luu MAC/id cua master vao `gw_peer`.
- Add master lam peer.
- Gui lai `NOW_ADD_PEER`.

### `NOW_ADD_PEER`

Slave gui id cua minh cho master.

```text
Frame:
gw_code(u32)
NOW_ADD_PEER(u8)
slave_id(u8)
crc(u16)
```

Master xu ly:

- Lay MAC tu callback receive.
- Add peer voi `slave_id`.

### `NOW_OPF`

Master yeu cau slave open `/spiffs/effect.bin`.

```text
Frame:
gw_code(u32)
NOW_OPF(u8)
crc(u16)

Response:
NOW_ACK  payload = NOW_OPF
NOW_NACK payload = NOW_OPF
```

### `NOW_CLSF`

Master yeu cau slave close file effect.

```text
Frame:
gw_code(u32)
NOW_CLSF(u8)
crc(u16)

Response:
NOW_ACK  payload = NOW_CLSF
NOW_NACK payload = NOW_CLSF
```

### `NOW_DLTF`

Master yeu cau slave delete file effect.

```text
Frame:
gw_code(u32)
NOW_DLTF(u8)
crc(u16)

Response:
NOW_ACK  payload = NOW_DLTF
NOW_NACK payload = NOW_DLTF
```

### `NOW_ST_WRF`

Master bat dau gui block effect cho slave.

```text
Frame:
gw_code(u32)
NOW_ST_WRF(u8)
total_packet(u32)
offset_start(u32)
total_byte(u32)
crc(u16)

Response:
NOW_ACK  payload = NOW_ST_WRF
NOW_NACK payload = NOW_ST_WRF
```

Ghi chu:

- Slave reset buffer nhan file cu khi co `NOW_ST_WRF` moi.
- `total_packet` phai > 0 va <= `ESPNOW_RECV_WRF_MAX_PACKETS`.
- Neu phien nhan file bi treo qua `ESPNOW_RECV_WRF_TIMEOUT_MS`, slave cleanup RAM.

### `NOW_WRF`

Master gui mot packet data effect.

```text
Frame:
gw_code(u32)
NOW_WRF(u8)
packet_number(u32)
offset(u32)
data[0..N]
crc(u16)
```

Ghi chu:

- `packet_number` la index trong phien `NOW_ST_WRF`.
- `offset` la offset ghi vao file effect cua slave.
- Slave chi nhan packet neu da co `NOW_ST_WRF` hop le.
- Lenh nay khong ACK tung packet; ACK/NACK tong hop o `NOW_END_WRF`.

### `NOW_END_WRF`

Master bao ket thuc gui file va gui checksum tong.

```text
Frame:
gw_code(u32)
NOW_END_WRF(u8)
checksum(u16)
crc(u16)
```

Response thanh cong:

```text
gw_code(u32)
NOW_ACK(u8)
NOW_END_WRF(u8)
crc(u16)
```

Response thieu packet:

```text
gw_code(u32)
NOW_NACK(u8)
NOW_END_WRF(u8)
packet_number_0(u32)
packet_number_1(u32)
...
crc(u16)
```

Response loi checksum hoac can gui lai tat ca:

```text
gw_code(u32)
NOW_NACK(u8)
NOW_END_WRF(u8)
0xffffffff(u32)
crc(u16)
```

Ghi chu:

- Checksum la CRC16 Modbus init `0xffff`, tinh lien tiep tren tat ca data packet theo thu tu packet.
- Neu danh sach packet thieu qua dai, slave tra `0xffffffff` de master gui lai tu dau.

### `NOW_EFF_SYNC`

Master gui snapshot state cho sync mode.

```text
Frame:
gw_code(u32)
NOW_EFF_SYNC(u8)
number_of_group(u8)
group_id[0](u8)
...
group_id[number_of_group-1](u8)
state[0](u16)
...
state[number_of_group-1](u16)
crc(u16)
```

Vi du 3 group:

```text
number_of_group = 3
group_id = [1, 2, 3]
state    = [0, 4, 7]
```

Ghi chu:

- Slave validate payload length truoc khi copy.
- Frame sync duoc dua vao queue realtime rieng `xNowRecvEffect`.
- Neu queue realtime day, firmware bo frame effect cu nhat va giu frame moi nhat.

### `NOW_EFF_ASYNC`

Master dua node ve async mode, thuong voi state 0.

```text
Frame:
gw_code(u32)
NOW_EFF_ASYNC(u8)
number_of_group(u8)
group_id[0](u8)
...
group_id[number_of_group-1](u8)
state[0](u16)
...
state[number_of_group-1](u16)
crc(u16)
```

Hien tai master tao `state` bang 0 cho cac group khi chuyen async.

### `NOW_ACK`

ACK cho mot lenh ESP-NOW.

```text
Frame:
gw_code(u32)
NOW_ACK(u8)
ack_of_cmd(u8)
optional_data...
crc(u16)
```

Dang dung:

| `ack_of_cmd` | Optional data |
| --- | --- |
| `NOW_OPF` | none |
| `NOW_CLSF` | none |
| `NOW_DLTF` | none |
| `NOW_ST_WRF` | none |
| `NOW_END_WRF` | none |

### `NOW_NACK`

NACK cho mot lenh ESP-NOW.

```text
Frame:
gw_code(u32)
NOW_NACK(u8)
nack_of_cmd(u8)
optional_data...
crc(u16)
```

Dang dung:

| `nack_of_cmd` | Optional data |
| --- | --- |
| `NOW_OPF` | none |
| `NOW_CLSF` | none |
| `NOW_DLTF` | none |
| `NOW_ST_WRF` | none |
| `NOW_END_WRF` | list packet_number(u32) hoac `0xffffffff` |

## 9. Luong Gui File Tu App Den He Thong

1. App ket noi TCP den master port `80`.
2. App gui `TCP_OPF`.
3. App gui `TCP_ST_WRF(offset_start=0, total_byte=file_size)`.
4. App gui nhieu frame `TCP_WRF(offset, chunk_data)`.
5. App gui `TCP_END_WRF(checksum)`.
6. Master ghi `/spiffs/tcp.bin`.
7. Master tach file tong theo node.
8. Master ghi block cua chinh no vao `/spiffs/effect.bin`.
9. Master gui block cua slave bang `NOW_OPF`, `NOW_ST_WRF`, nhieu `NOW_WRF`, `NOW_END_WRF`.
10. Master tra `TCP_RETURN_ID_RECEIVED(bit_mask_id_esp)` ve app.
11. App gui `TCP_EFF_SYNCH` hoac `TCP_EFF_ASYNCH`.

## 10. Cau Truc File Effect

File effect local `/spiffs/effect.bin` duoc `task_make_effect()` doc.

Header:

```text
brNess(u8)
speedEf(u8)
numGroup(u8)
offStartGr[numGroup](u32)
totByteGr[numGroup](u32)
```

Moi group:

```text
id_group(i8)
numState(u16)
timeExistOfSta[numState](u8)
numObject(u8)
object...
```

Moi object:

```text
id_object(u8)
numPin(u8)
p_pin[numPin](u8)
brNessofState[numState](u8)
```

Thoi gian state:

```text
time_ms = (TIME_EXIST_BASE + (timeExist - 1) * UNIT_OF_TIME_EXIST) * speedEf / 100
TIME_EXIST_BASE = 220ms
UNIT_OF_TIME_EXIST = 20ms
```

## 11. Timeout Va Queue ESP-NOW

| Nhom | Timeout send | Queue wait | Ghi chu |
| --- | ---: | ---: | --- |
| Broadcast | `80ms` | `5ms` | Tim peer nhanh |
| Add peer | `300ms` | `300ms` | Ket noi slave/master |
| Control ACK/NACK | `300ms` | `300ms` | Phan hoi lenh |
| File control | `500ms` | `500ms` | OPF/ST_WRF/END_WRF |
| File data | `150ms` | `500ms` | WRF packet |
| Effect sync realtime | `40ms` | `0ms` | Frame cu co the drop |
| Effect mode async | `150ms` | `5ms` | Chuyen mode |

Nhan realtime:

- `xNowRecv`: queue frame ESP-NOW thuong.
- `xNowRecvEffect`: queue rieng cho `NOW_EFF_SYNC/NOW_EFF_ASYNC`.
- `xEffRequest`: queue request effect cho `task_make_effect()`, firmware giu request moi nhat.

## 12. Duong Dan SPIFFS

```text
/spiffs/tcp.bin        File tong master nhan tu TCP
/spiffs/tcp_tmp.bin    File tam khi ghi TCP
/spiffs/effect.bin     File effect local cua tung ESP
/spiffs/effect_tmp.bin File tam khi ghi effect qua ESP-NOW
```

## 13. Ghi Chu Debug

- TCP phai dung magic `TCP`, `CMD`, `LEN little-endian`.
- Mot TCP callback co the chua nhieu frame; parser se xu ly lan luot.
- ESP-NOW frame sai `gw_code` hoac CRC se bi bo qua.
- Neu slave dung effect trong sync mode, kiem tra `NOW_EFF_SYNC`, peer list, queue `xNowRecvEffect`, va `gw_code`.
- Neu truyen file lon, chia TCP chunk nho va tinh checksum tren dung thu tu byte cua file.
- Neu `TCP_RETURN_ID_RECEIVED` thieu bit cua slave, xem ACK/NACK `NOW_END_WRF` va packet resend.
