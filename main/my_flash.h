#ifndef _MY_FLASH_H
#define _MY_FLASH_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nvs.h"
#include "nvs_flash.h"

#define NAME_SPACE "storage"

void write_blob(nvs_handle_t handle, int *data, size_t length);
void read_blob(nvs_handle_t handle, int *data_rec, size_t length);

#endif /* _MY_FLASH_H */
