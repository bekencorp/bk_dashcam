#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include "components/bluetooth/bk_dm_bluetooth.h"
#include <components/ate.h>

#include "cli.h"
#include "lcd_act.h"
#include "media_app.h"
#include "media_service.h"
#include "bt_manager.h"

extern void user_app_main(void);
extern void rtos_set_user_app_entry(beken_thread_function_t entry);


#if (CONFIG_SYS_CPU0)

const lcd_open_t lcd_open =
{
    .device_ppi = PPI_480X272,
    .device_name = "st7282",
};

void lvgl_app_init(void)
{
    bk_err_t ret;

    os_printf("!!!LVGL APP INIT!!!\r\n");

    ret = media_app_lvgl_open((lcd_open_t *)&lcd_open);
    if (ret != BK_OK)
    {
        os_printf("[%s] media_app_lvgl_open failed\r\n", __func__);
        return;
    }
}
#endif

void user_app_main(void)
{
}

int main(void)
{
#if (CONFIG_SYS_CPU0)
    rtos_set_user_app_entry((beken_thread_function_t)user_app_main);
    //bk_set_printf_sync(true);
    //shell_set_log_level(BK_LOG_INFO);
#endif

    bk_init();

    media_service_init();

#if CONFIG_SYS_CPU0
    if (!ate_is_enabled())
    {
        bt_manager_init();

#if CONFIG_A2DP_SINK_DEMO
        extern int a2dp_sink_demo_init(uint8_t aac_supported);
        a2dp_sink_demo_init(0);
#endif

#if CONFIG_HFP_HF_DEMO
        extern int hfp_hf_demo_init(uint8_t msbc_supported);
        hfp_hf_demo_init(0);
#endif

#if CONFIG_BT
        extern int cli_headset_demo_init(void);
        cli_headset_demo_init();
#endif

#if CONFIG_BLE
        extern int cli_ble_gatt_demo_init(void);
        cli_ble_gatt_demo_init();
        extern int cli_ble_hogpd_demo_init(void);
        cli_ble_hogpd_demo_init();
#endif

        lvgl_app_init();
    }
#endif
    return 0;
}
