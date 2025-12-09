#ifndef MQTT_DATA_H
#define MQTT_DATA_H

#define MAX_DEVICES 20
#define MAX_NAME_LEN 32
#define MAX_ID_LEN   32

typedef struct {
    char device_id[MAX_ID_LEN];
    char name[MAX_NAME_LEN];
} device_info_t;

extern device_info_t device_list[MAX_DEVICES];
extern int device_count;

#endif
