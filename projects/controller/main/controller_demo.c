#include <components/system.h>
#include <os/os.h>
#include <components/log.h>
#include "components/bluetooth/bk_ble.h"
#include <os/mem.h>
#include <os/str.h>

#define TAG  "cp_test"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

static ble_err_t ble_hci_to_host_evt_cb(uint8_t *buf, uint16_t len)
{
    LOGI("recv hci evt: type(0x%02X), len(%d)\r\n",buf[0], len);

    char *log = NULL;
    uint32 log_len = len * 3 + 1;

    log = (char *)os_malloc(log_len);

    if (log)
    {
        os_memset(log, 0, log_len);

        for (int i = 0; i < len; ++i)
        {
            os_snprintf(log + os_strlen(log), log_len - os_strlen(log), "%02X ", buf[i]);
        }

        LOGI("%s \r\n", log);
        os_free(log);
    }
    return 0;
}

static ble_err_t ble_hci_to_host_acl_cb(uint8_t *buf, uint16_t len)
{
    return 0;
}

void controller_demo_init(void)
{
    bk_ble_reg_hci_recv_callback(ble_hci_to_host_evt_cb, ble_hci_to_host_acl_cb);

    rtos_delay_milliseconds(1000);

    uint8_t hci_reset_cmd[] = {0x01, 0x03, 0x0c, 0x00};

    LOGI("Send hci reset cmd\r\n");
    bk_ble_hci_to_controller(hci_reset_cmd[0], &hci_reset_cmd[1], sizeof(hci_reset_cmd) - 1);
}
