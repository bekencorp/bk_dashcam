#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include "components/bluetooth/bk_dm_bluetooth.h"
#include <components/ate.h>

#include "cli.h"
//#include "lcd_act.h"
#include "media_app.h"
#include "media_service.h"
#include "bt_manager.h"
#include "gatt/dm_gatt.h"
#include "gatt/dm_gatts.h"
#include "hogpd/hogpd_demo.h"
#include "wifi_boarding/wifi_boarding_demo.h"

#if CONFIG_MEDIA_RECEIVE_DEMO
#include "media_tcp_service.h"
#include "media_udp_service.h"
#endif

#define AUTO_ENABLE_BLUETOOTH_DEMO 1


extern void rtos_set_user_app_entry(beken_thread_function_t entry);


#if (CONFIG_SYS_CPU0)

typedef struct {
    uint8_t enable : 1;
    uint32_t seqence;
    beken_semaphore_t sem;
    beken_thread_t thread;
} test_thread_t;

test_thread_t *s_app_test = NULL;
beken_thread_t media_demo_thread;

#define DEFAULT_WIFI_SSID "bicycle"
#define DEFAULT_WIFI_KEY "12345678"
static char wifi_ssid[50] = DEFAULT_WIFI_SSID;
static char wifi_key[50] = DEFAULT_WIFI_KEY;


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

void media_receive_sta_demo_init(void)
{
    os_printf("+++start connect\n");
    demo_sta_app_init(wifi_ssid, wifi_key);
    os_printf("---connect ssid:%s, key:%s\n", wifi_ssid, wifi_key);

#if CONFIG_MEDIA_DEMO_MODE_TCP
    av_server_tcp_service_init((void *)&lcd_open, ROTATE_NONE);
#else
    av_server_udp_service_init((void *)&lcd_open, ROTATE_NONE);
#endif
}

void media_receive_softap_demo_init(void)
{
    os_printf("---create ssid:%s, key:%s\n", wifi_ssid, wifi_key);
    demo_softap_app_init(wifi_ssid, wifi_key, "13");
    os_printf("---connected\n");

#if CONFIG_MEDIA_DEMO_MODE_TCP
    av_server_tcp_service_init((void *)&lcd_open, ROTATE_NONE);
#else
    av_server_udp_service_init((void *)&lcd_open, ROTATE_NONE);
#endif
}

void media_receive_demo_entry(beken_thread_arg_t param)
{
    if ((uint32_t)param == BK_SOFT_AP)
    {
        media_receive_softap_demo_init();
    }
    else if ((uint32_t)param == BK_STATION)
    {
        media_receive_sta_demo_init();
    }
    media_demo_thread = NULL;
    rtos_delete_thread(NULL);
}

void cli_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    os_printf("%s\r\n", __func__);

    if (os_strcmp(argv[1], "open") == 0)
    {
        media_camera_device_t device = {0};

        device.type = NET_CAMERA;
        device.fmt =PIXEL_FMT_JPEG;
        device.info.fps = 30;
        device.info.resolution.width = 480;
        device.info.resolution.height = 272;
        device.mode = JPEG_MODE;

        media_app_camera_open(&device);

        media_app_lcd_open((void *)&lcd_open);

        media_app_lvgl_close();

        if (s_app_test->sem)
        {
            rtos_set_semaphore(&s_app_test->sem);
            os_printf("start receive jpeg data...\n");
        }
    }

    if (os_strcmp(argv[1], "close") == 0)
    {
        //
    }

    if (os_strcmp(argv[1], "sta") == 0)
    {
        if (argc >= 3)
        {
            int wifi_ssid_len = os_strlen(argv[2]);
            if (wifi_ssid_len > 0)
            {
                if (wifi_ssid_len >= 50)
                {
                    wifi_ssid_len = 49;
                }
                os_memcpy(wifi_ssid, argv[2], wifi_ssid_len);
                wifi_ssid[wifi_ssid_len] = '\0';
            }
        }
        else
        {
            os_memcpy(wifi_ssid, DEFAULT_WIFI_SSID, os_strlen(DEFAULT_WIFI_SSID));
            wifi_ssid[os_strlen(DEFAULT_WIFI_SSID)] = '\0';
        }
        if (argc >= 4)
        {
            int wifi_key_len = os_strlen(argv[3]);
            if (wifi_key_len > 0)
            {
                if (wifi_key_len >= 50)
                {
                    wifi_key_len = 49;
                }
                os_memcpy(wifi_key, argv[3], wifi_key_len);
                wifi_key[wifi_key_len] = '\0';
            }
        }
        else
        {
            os_memcpy(wifi_key, DEFAULT_WIFI_KEY, os_strlen(DEFAULT_WIFI_KEY));
            wifi_key[os_strlen(DEFAULT_WIFI_KEY)] = '\0';
        }
        if (media_demo_thread == NULL)
        {
            rtos_create_thread(&media_demo_thread,
                                BEKEN_APPLICATION_PRIORITY,
                                "media_sta",
                                (beken_thread_function_t)media_receive_demo_entry,
                                1024 * 2,
                                (beken_thread_arg_t)BK_STATION);
        }
        else
        {
            os_printf("already open wifi\n");
        }
    }
    if (os_strcmp(argv[1], "ap") == 0)
    {
        if (argc >= 3)
        {
            int wifi_ssid_len = os_strlen(argv[2]);
            if (wifi_ssid_len > 0)
            {
                if (wifi_ssid_len >= 50)
                {
                    wifi_ssid_len = 49;
                }
                os_memcpy(wifi_ssid, argv[2], wifi_ssid_len);
                wifi_ssid[wifi_ssid_len] = '\0';
            }
        }
        else
        {
            os_memcpy(wifi_ssid, DEFAULT_WIFI_SSID, os_strlen(DEFAULT_WIFI_SSID));
            wifi_ssid[os_strlen(DEFAULT_WIFI_SSID)] = '\0';
        }
        if (argc >= 4)
        {
            int wifi_key_len = os_strlen(argv[3]);
            if (wifi_key_len > 0)
            {
                if (wifi_key_len >= 50)
                {
                    wifi_key_len = 49;
                }
                os_memcpy(wifi_key, argv[3], wifi_key_len);
                wifi_key[wifi_key_len] = '\0';
            }
        }
        else
        {
            os_memcpy(wifi_key, DEFAULT_WIFI_KEY, os_strlen(DEFAULT_WIFI_KEY));
            wifi_key[os_strlen(DEFAULT_WIFI_KEY)] = '\0';
        }
        if (media_demo_thread == NULL)
        {
            rtos_create_thread(&media_demo_thread,
                                BEKEN_APPLICATION_PRIORITY,
                                "media_ap",
                                (beken_thread_function_t)media_receive_demo_entry,
                                1024 * 2,
                                (beken_thread_arg_t)BK_SOFT_AP);
        }
        else
        {
            os_printf("already open wifi\n");
        }
    }
}

static const struct cli_command s_test_cmd_commands[] =
{
    {"test", "open", cli_test_cmd},
};


#define CMDS_COUNT  (sizeof(s_test_cmd_commands) / sizeof(struct cli_command))

int cli_test_cmd_init(void)
{
    return cli_register_commands(s_test_cmd_commands, CMDS_COUNT);
}

extern unsigned char jpeg14[2489];
extern unsigned char jpeg28[2829];
extern unsigned char jpeg42[2751];

void app_main_test_entry(beken_thread_arg_t param)
{
    test_thread_t *config = (test_thread_t *)param;

    rtos_get_semaphore(&config->sem, BEKEN_NEVER_TIMEOUT);

    frame_buffer_t * new_frame = NULL;

    config->enable = true;
    config->seqence = 0;

    media_app_frame_buffer_init(FB_INDEX_SMALL_JPEG);

    while (config->enable)
    {
        new_frame = media_app_frame_buffer_small_jpeg_malloc();
        if (new_frame->frame == NULL)
        {
            rtos_delay_milliseconds(100);
            continue;
        }

        uint8_t index = config->seqence & 0x3;

        if (index == 0)
        {
            os_memcpy(new_frame->frame, jpeg14, 2489);
        }
        else if (index == 1)
        {
            os_memcpy(new_frame->frame, jpeg28, 2829);
        }
        else if (index == 2)
        {
            os_memcpy(new_frame->frame, jpeg42, 2751);
        }
        else
        {
            os_memcpy(new_frame->frame, jpeg14, 2489);
        }

        new_frame->sequence = config->seqence++;
        new_frame->width = 480;
        new_frame->height = 272;
        new_frame->fmt = PIXEL_FMT_SMALL_JPEG;

        media_app_frame_buffer_push(new_frame);

        rtos_delay_milliseconds(30);
    }

}

static void user_app_main(void)
{
    s_app_test = (test_thread_t *)os_malloc(sizeof(test_thread_t));

    os_memset(s_app_test, 0, sizeof(test_thread_t));

    rtos_init_semaphore(&s_app_test->sem, 1);

    rtos_create_thread(&s_app_test->thread,
                        BEKEN_APPLICATION_PRIORITY,
                        "app_test",
                        (beken_thread_function_t)app_main_test_entry,
                        1024 * 2,
                        (beken_thread_arg_t)s_app_test);

#if CONFIG_MP3_PLAY_TEST
    extern int cli_audio_play_sdcard_mp3_music_init(void);
    cli_audio_play_sdcard_mp3_music_init();
#endif

}


#endif

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

#if AUTO_ENABLE_BLUETOOTH_DEMO
#if CONFIG_A2DP_SINK_DEMO
        extern int a2dp_sink_demo_init(uint8_t aac_supported);
        a2dp_sink_demo_init(0);
#endif

#if CONFIG_HFP_HF_DEMO
        extern int hfp_hf_demo_init(uint8_t msbc_supported);
        hfp_hf_demo_init(0);
#endif

#if CONFIG_BLE
        cli_gatt_param_t param = {.rpa = 0, .p_rpa = &param.rpa, .pa = 0, .p_pa = &param.pa};

        dm_gatt_main(&param);
        dm_gatts_main(&param);
        hogpd_demo_init();
        wifi_boarding_demo_main();
#endif
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
		extern int cli_ble_wboarding_demo_init(void);
        cli_ble_wboarding_demo_init();
#endif

        lvgl_app_init();

#if CONFIG_MEDIA_RECEIVE_DEMO
//        media_receive_demo_init();
#endif

        cli_test_cmd_init();

        bk_wifi_set_wifi_media_mode(1);
    }
#endif
    return 0;
}
