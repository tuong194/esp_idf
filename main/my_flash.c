#include "my_flash.h"

void write_blob(nvs_handle_t handle, int *data, size_t length)
{
    // int data[] = {1,2,3,4,5,6};
    // size_t length = sizeof(data);
    nvs_open(NAME_SPACE, NVS_READWRITE, &handle);
    nvs_set_blob(handle, "datablob", (const void *)data, length);
    nvs_commit(handle);
    printf("write data\n");
    nvs_close(handle);
}

void read_blob(nvs_handle_t handle, int *data_rec, size_t length)
{
    // int data[6];
    // size_t length = sizeof(data);
    nvs_open(NAME_SPACE, NVS_READWRITE, &handle);
    nvs_get_blob(handle, "datablob", (const void *)data_rec, &length);
    nvs_close(handle);
    printf("read data\n");
} 