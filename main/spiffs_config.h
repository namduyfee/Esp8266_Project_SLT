#ifndef SPIFFS_CONFIG
#define SPIFFS_CONFIG

#include "my_lib.h"

void spiffs_init(void);
void spiffs_write_file(const char *path, const char *data, uint32_t data_len);
void spiffs_read_file(const char *path, char *data, uint32_t data_len);


#endif