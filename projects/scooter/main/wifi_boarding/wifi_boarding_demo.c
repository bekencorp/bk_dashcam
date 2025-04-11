#include <common/sys_config.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <components/log.h>
#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>

#include "wifi_boarding_demo.h"
#include "wifi_boarding_network.h"

#include "components/bluetooth/bk_dm_bluetooth_types.h"
#include "components/bluetooth/bk_dm_gap_ble_types.h"
#include "components/bluetooth/bk_dm_gap_ble.h"
#include "components/bluetooth/bk_dm_gatt_types.h"
#include "components/bluetooth/bk_dm_gatts.h"
#include "dm_gatts.h"
#include "components/bluetooth/bk_dm_bluetooth.h"

#include <modules/wifi.h>
#include <components/event.h>
#include <components/netif.h>

#if WIFI_BOARDING_DEMO_ENABLE

typedef struct
{
    uint8_t status; //0 idle 1 connected
} wifi_boarding_app_env_t;

#define MIN_VALUE(x, y) (((x) < (y)) ? (x): (y))

static bk_boarding_info_t *bk_boarding_info = NULL;
static ble_boarding_info_t *s_ble_boarding_info = NULL;


static uint16_t s_prop_cli_config;
static uint8_t s_ssid[64];
static uint8_t s_password[64];
static uint8_t s_wifi_boarding_is_init;
static uint16_t s_bd_conn_ind = ~0;

static beken_thread_t s_boarding_thd = NULL;
static beken_queue_t s_boarding_queue = NULL;

static bk_gatt_if_t s_bd_gatts_if = 0;

enum
{
    BOARDING_IDX_SVC,
    BOARDING_IDX_CHAR1,
    BOARDING_IDX_CHAR1_DESC,
    BOARDING_IDX_CHAR_OPERATION,
    BOARDING_IDX_CHAR_SSID,
    BOARDING_IDX_CHAR_PASSWORD,
    BOARDING_IDX_NB,
};

static const bk_gatts_attr_db_t s_gatts_attr_db_service_boarding[] =
{
    {
        BK_GATT_PRIMARY_SERVICE_DECL(0xfa00),
    },

    {
        BK_GATT_CHAR_DECL(0xea01,
                          0, NULL,
                          BK_GATT_CHAR_PROP_BIT_NOTIFY,
                          BK_GATT_PERM_READ,
                          BK_GATT_RSP_BY_APP),
    },
    {
        BK_GATT_CHAR_DESC_DECL(BK_GATT_UUID_CHAR_CLIENT_CONFIG,
                               sizeof(s_prop_cli_config), (uint8_t *)&s_prop_cli_config,
                               BK_GATT_PERM_READ | BK_GATT_PERM_WRITE,
                               BK_GATT_RSP_BY_APP),
    },

    //operation
    {
        BK_GATT_CHAR_DECL(0xea02,
                          0, NULL,
                          BK_GATT_CHAR_PROP_BIT_WRITE,
                          BK_GATT_PERM_WRITE,
                          BK_GATT_RSP_BY_APP),
    },

    //ssid
    {
        BK_GATT_CHAR_DECL(0xea05,
                          sizeof(s_password), (uint8_t *)s_password,
                          BK_GATT_CHAR_PROP_BIT_READ | BK_GATT_CHAR_PROP_BIT_WRITE,
                          BK_GATT_PERM_READ | BK_GATT_PERM_WRITE,
                          BK_GATT_AUTO_RSP),
    },

    //password
    {
        BK_GATT_CHAR_DECL(0xea06,
                          sizeof(s_ssid), (uint8_t *)s_ssid,
                          BK_GATT_CHAR_PROP_BIT_READ | BK_GATT_CHAR_PROP_BIT_WRITE,
                          BK_GATT_PERM_READ | BK_GATT_PERM_WRITE,
                          BK_GATT_AUTO_RSP),
    },
};

static uint16_t s_boarding_attr_handle_list[sizeof(s_gatts_attr_db_service_boarding) / sizeof(s_gatts_attr_db_service_boarding[0])];

int wifi_boarding_notify(uint8_t *data, uint16_t length)
{
    if (s_bd_conn_ind == 0xFF)
    {
        wboard_loge("BLE is disconnected, can not send data !!!");
        return BK_FAIL;
    }
    else
    {
        wboard_logi("len %d", length);
        bk_ble_gatts_send_indicate(s_bd_gatts_if, s_bd_conn_ind, s_boarding_attr_handle_list[BOARDING_IDX_CHAR1], length, data, 0);
        return BK_OK;
    }
}

static int32_t wifi_boarding_gatts_cb(bk_gatts_cb_event_t event, bk_gatt_if_t gatts_if, bk_ble_gatts_cb_param_t *comm_param)
{
    ble_err_t ret = 0;
    dm_gatt_app_env_t *common_env_tmp = NULL;
    wifi_boarding_app_env_t *app_env_tmp = NULL;

    switch (event)
    {
    case BK_GATTS_CONNECT_EVT:
    {
        struct gatts_connect_evt_param *param = (typeof(param))comm_param;

        wboard_logi("BK_GATTS_CONNECT_EVT %d role %d %02X:%02X:%02X:%02X:%02X:%02X", param->conn_id, param->link_role,
                    param->remote_bda[5],
                    param->remote_bda[4],
                    param->remote_bda[3],
                    param->remote_bda[2],
                    param->remote_bda[1],
                    param->remote_bda[0]);

        s_bd_gatts_if = gatts_if;
        s_bd_conn_ind = param->conn_id;

        common_env_tmp = dm_ble_alloc_addition_data_by_addr(param->remote_bda, sizeof(*app_env_tmp));

        if (!common_env_tmp)
        {
            wboard_loge("alloc addition data err !!!!");
            break;
        }

        app_env_tmp = (typeof(app_env_tmp))common_env_tmp->addition_data;
        app_env_tmp->status = 1;
    }
    break;

    case BK_GATTS_DISCONNECT_EVT:
    {
        struct gatts_disconnect_evt_param *param = (typeof(param))comm_param;

        s_bd_gatts_if = 0;
        s_bd_conn_ind = ~0;


        wboard_logi("BK_GATTS_DISCONNECT_EVT %02X:%02X:%02X:%02X:%02X:%02X",
                    param->remote_bda[5],
                    param->remote_bda[4],
                    param->remote_bda[3],
                    param->remote_bda[2],
                    param->remote_bda[1],
                    param->remote_bda[0]);

        common_env_tmp = dm_ble_find_app_env_by_addr(param->remote_bda);

        if (!common_env_tmp)
        {
            wboard_loge("cant find app env");
            break;
        }

        if (common_env_tmp->addition_data)
        {
            os_free(common_env_tmp->addition_data);
            common_env_tmp->addition_data = NULL;
        }

        common_env_tmp->addition_data_len = 0;
    }
    break;

    case BK_GATTS_CONF_EVT:
    {
        wboard_logi("BK_GATTS_CONF_EVT");
    }
    break;

    case BK_GATTS_RESPONSE_EVT:
    {
        wboard_logi("BK_GATTS_RESPONSE_EVT");
    }
    break;

    case BK_GATTS_READ_EVT:
    {
        struct gatts_read_evt_param *param = (typeof(param))comm_param;
        bk_gatt_rsp_t rsp;
        uint16_t final_len = 0;

        memset(&rsp, 0, sizeof(rsp));
        wboard_logi("read attr handle %d need rsp %d", param->handle, param->need_rsp);

        uint8_t *tmp_buff = NULL;
        uint16_t buff_size = 0;
        uint8_t valid = 1;

        if (s_boarding_attr_handle_list[BOARDING_IDX_CHAR1_DESC] == param->handle)
        {
            bk_ble_gatts_get_attr_value(param->handle, &buff_size, &tmp_buff);
        }
        else if (s_boarding_attr_handle_list[BOARDING_IDX_CHAR_SSID] == param->handle)
        {
            bk_ble_gatts_get_attr_value(param->handle, &buff_size, &tmp_buff);
        }
        else if (s_boarding_attr_handle_list[BOARDING_IDX_CHAR_PASSWORD] == param->handle)
        {
            bk_ble_gatts_get_attr_value(param->handle, &buff_size, &tmp_buff);
        }
        else
        {
            wboard_loge("invalid read handle %d", param->handle);
            valid = 0;
        }

        if (param->need_rsp)
        {
            final_len = buff_size - param->offset;

            rsp.attr_value.auth_req = BK_GATT_AUTH_REQ_NONE;
            rsp.attr_value.handle = param->handle;
            rsp.attr_value.offset = param->offset;

            if (tmp_buff && valid)
            {
                rsp.attr_value.len = final_len;
                rsp.attr_value.value = tmp_buff + param->offset;
            }
            else
            {
                rsp.attr_value.len = 0;
                rsp.attr_value.value = NULL;
            }

            ret = bk_ble_gatts_send_response(gatts_if, param->conn_id, param->trans_id,
                                             (tmp_buff && valid ? BK_GATT_OK : BK_GATT_INSUF_RESOURCE), &rsp);
        }
    }
    break;

    case BK_GATTS_WRITE_EVT:
    {
        struct gatts_write_evt_param *param = (typeof(param))comm_param;
        bk_gatt_rsp_t rsp;
        uint16_t final_len = 0;

        memset(&rsp, 0, sizeof(rsp));

        wboard_logi("write attr handle %d len %d offset %d need rsp %d", param->handle, param->len, param->offset, param->need_rsp);

        uint8_t *tmp_buff = NULL;
        uint16_t buff_size = 0;
        uint8_t valid = 1;

        if (s_boarding_attr_handle_list[BOARDING_IDX_CHAR1_DESC] == param->handle)
        {
            bk_ble_gatts_get_attr_value(param->handle, &buff_size, &tmp_buff);
        }
        else if (s_boarding_attr_handle_list[BOARDING_IDX_CHAR_OPERATION] == param->handle)
        {
            bk_ble_gatts_get_attr_value(param->handle, &buff_size, &tmp_buff);
            wboard_logi("write boarding op char");
        }
        else if (s_boarding_attr_handle_list[BOARDING_IDX_CHAR_SSID] == param->handle)
        {
            bk_ble_gatts_get_attr_value(param->handle, &buff_size, &tmp_buff);

            if (s_ble_boarding_info->ssid_value)
            {
                os_free(s_ble_boarding_info->ssid_value);
                s_ble_boarding_info->ssid_value = NULL;
                s_ble_boarding_info->ssid_length = 0;
            }

            s_ble_boarding_info->ssid_length = param->len;
            s_ble_boarding_info->ssid_value = os_malloc(param->len + 1);

            if (!s_ble_boarding_info->ssid_value)
            {
                wboard_loge("alloc ssid err");
                valid = 0;
            }
            else
            {
                os_memset(s_ble_boarding_info->ssid_value, 0, param->len + 1);
                os_memcpy((uint8_t *)s_ble_boarding_info->ssid_value, param->value, param->len);

                wboard_logi("ssid: %s", s_ble_boarding_info->ssid_value);
            }
        }
        else if (s_boarding_attr_handle_list[BOARDING_IDX_CHAR_PASSWORD] == param->handle)
        {
            bk_ble_gatts_get_attr_value(param->handle, &buff_size, &tmp_buff);

            if (s_ble_boarding_info->password_value)
            {
                os_free(s_ble_boarding_info->password_value);
                s_ble_boarding_info->password_value = NULL;
                s_ble_boarding_info->password_length = 0;
            }

            s_ble_boarding_info->password_length = param->len;
            s_ble_boarding_info->password_value = os_malloc(param->len + 1);

            if (!s_ble_boarding_info->password_value)
            {
                wboard_loge("alloc password err");
                valid = 0;
            }
            else
            {
                os_memset(s_ble_boarding_info->password_value, 0, param->len + 1);
                os_memcpy((uint8_t *)s_ble_boarding_info->password_value, param->value, param->len);
                wboard_logi("password: %s", s_ble_boarding_info->password_value);
            }
        }
        else
        {
            wboard_loge("invalid write handle %d", param->handle);
            valid = 0;
        }

        if (param->need_rsp)
        {
            final_len = (param->len < buff_size - param->offset ? param->len :  buff_size - param->offset);

            if (tmp_buff)
            {
                os_memcpy(tmp_buff + param->offset, param->value, final_len);
            }

            rsp.attr_value.auth_req = BK_GATT_AUTH_REQ_NONE;
            rsp.attr_value.handle = param->handle;
            rsp.attr_value.offset = param->offset;

            if (tmp_buff && valid)
            {
                rsp.attr_value.len = final_len;
                rsp.attr_value.value = tmp_buff + param->offset;
            }

            ret = bk_ble_gatts_send_response(gatts_if, param->conn_id, param->trans_id, valid ? BK_GATT_OK : BK_GATT_INSUF_RESOURCE, &rsp);
        }

        if (s_boarding_attr_handle_list[BOARDING_IDX_CHAR_OPERATION] == param->handle)
        {
            uint16_t opcode = 0;
            uint16_t length = 0;
            uint8_t *data = NULL;

            if (param->len < 2)
            {
                wboard_loge("len invalid %d", param->len);
                break;
            }

            opcode = param->value[0] | param->value[1] << 8;

            if (param->len >= 4)
            {
                length = param->value[2] | param->value[3] << 8;
            }

            if (param->len > 4)
            {
                data = &param->value[4];
            }

            if (s_ble_boarding_info && s_ble_boarding_info->cb)
            {
                s_ble_boarding_info->cb(opcode, length, data);
            }
            else
            {
                wboard_loge("invalid s_ble_boarding_info");
                break;
            }

#if 0
            uint8_t test_data[20] = {0};
            uint16_t test_data_len = sizeof(test_data) - 2 - 1 - 2;
            os_memcpy(test_data, &opcode, sizeof(opcode));
            test_data[2] = 0;
            os_memcpy(test_data + 3, &test_data_len, sizeof(test_data_len));

            wifi_boarding_notify(test_data, sizeof(test_data));
#endif
        }
    }
    break;

    case BK_GATTS_EXEC_WRITE_EVT:
    {
        struct gatts_exec_write_evt_param *param = (typeof(param))comm_param;
        wboard_logi("exec write");
    }
    break;

    default:
        break;
    }

    return 0;
}

static int32_t wifi_boarding_demo_reg_db(void)
{
    int32_t ret = dm_gatts_reg_db((bk_gatts_attr_db_t *)s_gatts_attr_db_service_boarding,
                                  sizeof(s_gatts_attr_db_service_boarding) / sizeof(s_gatts_attr_db_service_boarding[0]),
                                  s_boarding_attr_handle_list,
                                  wifi_boarding_gatts_cb);

    if (ret)
    {
        wboard_loge("reg db err");
        return ret;
    }

    for (int i = 0; i < sizeof(s_gatts_attr_db_service_boarding) / sizeof(s_gatts_attr_db_service_boarding[0]); ++i)
    {
        wboard_logi("attr handle %d", s_boarding_attr_handle_list[i]);
    }

    return ret;
}

bk_err_t boarding_send_msg(boarding_msg_t *msg)
{
    bk_err_t ret = BK_OK;

    if (s_boarding_queue)
    {
        ret = rtos_push_to_queue(&s_boarding_queue, msg, BEKEN_NO_WAIT);

        if (BK_OK != ret)
        {
            wboard_loge("%s failed\n", __func__);
            return BK_FAIL;
        }

        return ret;
    }

    return ret;
}

void bk_boarding_event_notify(uint16_t opcode, int status)
{
    uint8_t data[] =
    {
        opcode & 0xFF, opcode >> 8,     /* opcode           */
                              status & 0xFF,                                                          /* status           */
                              0, 0,                                                                   /* payload length   */
    };

    wboard_logi("%s: %d, %d\n", __func__, opcode, status);
    wifi_boarding_notify(data, sizeof(data));
}

void bk_boarding_event_notify_with_data(uint16_t opcode, int status, char *payload, uint16_t length)
{
    uint8_t data[1024] =
    {
        opcode & 0xFF, opcode >> 8,     /* opcode           */
                              status & 0xFF,                  /* status           */
                              length & 0xFF, length >> 8,     /* payload length   */
                              0,
    };

    if (length > 1024 - 5)
    {
        wboard_loge("size %d over flow\n", length);
        return;
    }

    os_memcpy(&data[5], payload, length);

    wboard_logi("%s: %d, %d\n", __func__, opcode, status);
    wifi_boarding_notify(data, length + 5);
}

static void bk_boarding_operation_handle(uint16_t opcode, uint16_t length, uint8_t *data)
{
    wboard_logw("%s, opcode: %04X, length: %u\n", __func__, opcode, length);

    switch (opcode)
    {
        case BOARDING_OP_STATION_START:
        {
            boarding_msg_t msg;

            msg.event = DBEVT_WIFI_STATION_CONNECT;
            msg.param = (uint32_t)bk_boarding_info;
            boarding_send_msg(&msg);
        }
        break;

        case BOARDING_OP_SOFT_AP_START:
        {
            boarding_msg_t msg;

            msg.event = DBEVT_WIFI_SOFT_AP_TURNING_ON;
            msg.param = (uint32_t)bk_boarding_info;
            boarding_send_msg(&msg);
        }
        break;

        case BOARDING_OP_BLE_DISABLE:
        {
            boarding_msg_t msg;

            msg.event = DBEVT_BLE_DISABLE;
            msg.param = 0;
            boarding_send_msg(&msg);
        }
        break;

        case BOARDING_OP_SET_WIFI_CHANNEL:
        {
            STREAM_TO_UINT16(bk_boarding_info->channel, data);

            wboard_logi("%s, BOARDING_OP_SET_WIFI_CHANNEL: %u\n", __func__, bk_boarding_info->channel);

        }
        break;

        default:
        {
            wboard_loge("%s, unsupported opcode: 0x%04X !!!\n", __func__, opcode);
        }
        break;

    }
}

static void boarding_message_handle(void)
{
    bk_err_t ret = BK_OK;
    boarding_msg_t msg;

    while (1)
    {

        ret = rtos_pop_from_queue(&s_boarding_queue, &msg, BEKEN_WAIT_FOREVER);

        if (kNoErr == ret)
        {
            switch (msg.event)
            {
                case DBEVT_WIFI_STATION_CONNECT:
                {
                    wboard_logi("DBEVT_WIFI_STATION_CONNECT\n");

                    bk_boarding_info_t *wifi_info = (bk_boarding_info_t *) msg.param;
                    boarding_wifi_sta_connect(wifi_info->boarding_info.ssid_value,
                                              wifi_info->boarding_info.password_value);
                }
                break;

                case DBEVT_WIFI_STATION_CONNECTED:
                {
                    wboard_logi("DBEVT_WIFI_STATION_CONNECTED\n");

                    netif_ip4_config_t ip4_config;
                    extern uint32_t uap_ip_is_start(void);

                    os_memset(&ip4_config, 0x0, sizeof(netif_ip4_config_t));
                    bk_netif_get_ip4_config(NETIF_IF_AP, &ip4_config);
                    if (uap_ip_is_start())
                    {
                        bk_netif_get_ip4_config(NETIF_IF_AP, &ip4_config);
                    }
                    else
                    {
                        bk_netif_get_ip4_config(NETIF_IF_STA, &ip4_config);
                    }

                    wboard_logi("ip: %s\n", ip4_config.ip);

                    bk_boarding_event_notify_with_data(BOARDING_OP_STATION_START, BK_OK, ip4_config.ip, strlen(ip4_config.ip));
                }
                break;

                case DBEVT_WIFI_STATION_DISCONNECTED:
                {
                    wboard_logi("DBEVT_WIFI_STATION_DISCONNECTED\n");
                }
                break;

                case DBEVT_WIFI_SOFT_AP_TURNING_ON:
                {
                    wboard_logi("DBEVT_WIFI_SOFT_AP_TURNING_ON\n");
                    bk_boarding_info_t *wifi_info = (bk_boarding_info_t *) msg.param;
                    int ret = boarding_wifi_soft_ap_start(wifi_info->boarding_info.ssid_value,
                                                          wifi_info->boarding_info.password_value,
                                                          wifi_info->channel);

                    if (ret == BK_OK)
                    {
                        bk_boarding_event_notify(BOARDING_OP_SOFT_AP_START, EVT_STATUS_OK);
                    }
                    else
                    {
                        bk_boarding_event_notify(BOARDING_OP_SOFT_AP_START, EVT_STATUS_ERROR);
                    }
                }
                break;


                case DBEVT_BLE_DISABLE:
                {
#if CONFIG_BLUETOOTH
                    bk_bluetooth_deinit();
                    wboard_logi("close bluetooth finish!\r\n");
#endif
                }
                break;

                case DBEVT_EXIT:
                    goto exit;
                    break;

                default:
                    break;
            }
        }
    }

exit:

    /* delate msg queue */
    ret = rtos_deinit_queue(&s_boarding_queue);

    if (ret != kNoErr)
    {
        wboard_loge("delete message queue fail\n");
    }

    s_boarding_queue = NULL;

    wboard_loge("delete message queue complete\n");

    /* delate task */
    rtos_delete_thread(NULL);

    s_boarding_thd = NULL;

    wboard_loge("delete task complete\n");
}

#endif

int32_t wifi_boarding_demo_main(void)
{
#if WIFI_BOARDING_DEMO_ENABLE

    if (!dm_gatts_is_init())
    {
        wboard_loge("gatts is not init");
        return -1;
    }

    if (s_wifi_boarding_is_init)
    {
        wboard_loge("already init");
        return -1;
    }

    bk_err_t ret = BK_OK;

    ret = rtos_init_queue(&s_boarding_queue,
                      "boarding_queue",
                      sizeof(boarding_msg_t),
                      10);

    if (ret != BK_OK)
    {
        wboard_loge("%s, create boarding message queue failed\n");
        return -1;
    }

    ret = rtos_create_thread(&s_boarding_thd,
                         BEKEN_DEFAULT_WORKER_PRIORITY,
                         "boarding_thd",
                         (beken_thread_function_t)boarding_message_handle,
                         2560,
                         NULL);

    if (ret != BK_OK)
    {
        wboard_loge("create boarding major thread fail\n");
        return -1;
    }

    s_wifi_boarding_is_init = 1;

    if (bk_boarding_info == NULL)
    {
        bk_boarding_info = os_malloc(sizeof(bk_boarding_info_t));

        if (bk_boarding_info == NULL)
        {
            wboard_loge("bk_boarding_info malloc failed\n");

            return -1;
        }

        os_memset(bk_boarding_info, 0, sizeof(bk_boarding_info_t));
    }

    bk_boarding_info->boarding_info.cb = bk_boarding_operation_handle;
    s_ble_boarding_info = &bk_boarding_info->boarding_info;

    wifi_boarding_demo_reg_db();

    wboard_logi("done");
#else
    wboard_loge("wifi boarding demo not enable");
#endif
    return 0;
}
