#pragma once

#include <stdint.h>

#define WIFI_BOARDING_DEMO_ENABLE 1

enum
{
    BOARDING_DEBUG_LEVEL_ERROR,
    BOARDING_DEBUG_LEVEL_WARNING,
    BOARDING_DEBUG_LEVEL_INFO,
    BOARDING_DEBUG_LEVEL_DEBUG,
    BOARDING_DEBUG_LEVEL_VERBOSE,
};

#define BOARDING_DEBUG_LEVEL_INFO BOARDING_DEBUG_LEVEL_INFO

#define wboard_loge(format, ...) do{if(BOARDING_DEBUG_LEVEL_INFO >= BOARDING_DEBUG_LEVEL_ERROR)   BK_LOGE("app_board", "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define wboard_logw(format, ...) do{if(BOARDING_DEBUG_LEVEL_INFO >= BOARDING_DEBUG_LEVEL_WARNING) BK_LOGW("app_board", "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define wboard_logi(format, ...) do{if(BOARDING_DEBUG_LEVEL_INFO >= BOARDING_DEBUG_LEVEL_INFO)    BK_LOGI("app_board", "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define wboard_logd(format, ...) do{if(BOARDING_DEBUG_LEVEL_INFO >= BOARDING_DEBUG_LEVEL_DEBUG)   BK_LOGI("app_board", "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)
#define wboard_logv(format, ...) do{if(BOARDING_DEBUG_LEVEL_INFO >= BOARDING_DEBUG_LEVEL_VERBOSE) BK_LOGI("app_board", "%s:" format "\n", __func__, ##__VA_ARGS__);} while(0)

#define STREAM_TO_UINT16(u16, p) {u16 = ((uint16_t)(*(p)) + (((uint16_t)(*((p) + 1))) << 8)); (p) += 2;}

typedef enum
{
    BOARDING_OP_UNKNOWN = 0,
    BOARDING_OP_STATION_START = 1,
    BOARDING_OP_SOFT_AP_START = 2,
    BOARDING_OP_SERVICE_UDP_START = 3,
    BOARDING_OP_SERVICE_TCP_START = 4,
    BOARDING_OP_SET_CS2_DID = 5,
    BOARDING_OP_SET_CS2_APILICENSE = 6,
    BOARDING_OP_SET_CS2_KEY = 7,
    BOARDING_OP_SET_CS2_INIT_STRING = 8,
    BOARDING_OP_SRRVICE_CS2_START = 9,
    BOARDING_OP_BLE_DISABLE = 10,
    BOARDING_OP_SET_WIFI_CHANNEL = 11,
    BOARDING_OP_AGORA_AGENT_RSP = 12,
    BOARDING_OP_SET_AGORA_AGENT_INFO = 13,
    BOARDING_OP_NET_PAN_START = 14,
    BOARDING_OP_NETWORK_PROVISIONING_FIRST_TIME = 15,
    BOARDING_OP_START_AGENT_FROM_DEV = 16,
} boarding_opcode_t;

typedef void (*ble_boarding_op_cb_t)(uint16_t opcode, uint16_t length, uint8_t *data);
typedef struct
{
    char *ssid_value;
    char *password_value;
    ble_boarding_op_cb_t cb;
    uint8_t boarding_notify[2];
    uint16_t ssid_length;
    uint16_t password_length;
} ble_boarding_info_t;

typedef struct
{
    ble_boarding_info_t boarding_info;
    uint16_t channel;
} bk_boarding_info_t;


#define EVT_STATUS_OK               (0)
#define EVT_STATUS_ERROR            (1)

typedef enum
{
    DBEVT_WIFI_STATION_CONNECT,
    DBEVT_WIFI_STATION_CONNECTED,
    DBEVT_WIFI_STATION_DISCONNECTED,

    DBEVT_WIFI_SOFT_AP_TURNING_ON,

    DBEVT_BLE_DISABLE,
    DBEVT_EXIT,
} dbevt_t;

typedef struct
{
    uint32_t event;
    uint32_t param;
} boarding_msg_t;

bk_err_t boarding_send_msg(boarding_msg_t *msg);
int32_t wifi_boarding_demo_main(void);
