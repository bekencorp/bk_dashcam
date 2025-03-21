#include <os/os.h>
#include "lcd_act.h"
#include "media_evt.h"
#if (CONFIG_TP)
#include "driver/drv_tp.h"
#endif
#include "frame_buffer.h"
#include "yuv_encode.h"
#include "lv_vendor.h"

extern uint8_t lvgl_video_switch;

extern void lv_example_meter(void);
extern void lv_example_meter_exit(void);

static lv_vnd_config_t lv_vnd_config = {0};
void lvgl_event_open_handle(media_mailbox_msg_t *msg)
{
    os_printf("[%s] lvgl open start\r\n", __func__);

    lvgl_video_switch = 1;

    lcd_open_t *lcd_open = (lcd_open_t *)msg->param;

#ifdef CONFIG_LVGL_USE_PSRAM
#if 0
#define PSRAM_DRAW_BUFFER ((0x60000000UL) + 5 * 1024 * 1024)
    lv_vnd_config.draw_pixel_size = ppi_to_pixel_x(lcd_open->device_ppi) * ppi_to_pixel_y(lcd_open->device_ppi);
    lv_vnd_config.draw_buf_2_1 = (lv_color_t *)PSRAM_DRAW_BUFFER;
    lv_vnd_config.draw_buf_2_2 = (lv_color_t *)(PSRAM_DRAW_BUFFER + lv_vnd_config.draw_pixel_size * sizeof(lv_color_t));
#else
    lv_vnd_config.draw_pixel_size = ppi_to_pixel_x(lcd_open->device_ppi) * ppi_to_pixel_y(lcd_open->device_ppi);
    lv_vnd_config.draw_buf_2_1 = (lv_color_t *)frame_buffer_display_malloc(lv_vnd_config.draw_pixel_size * sizeof(lv_color_t));
    lv_vnd_config.draw_buf_2_2 = (lv_color_t *)frame_buffer_display_malloc(lv_vnd_config.draw_pixel_size * sizeof(lv_color_t));
#endif
#else
#define PSRAM_FRAME_BUFFER ((0x60000000UL) + 5 * 1024 * 1024)
    lv_vnd_config.draw_pixel_size = ppi_to_pixel_x(lcd_open->device_ppi) * ppi_to_pixel_y(lcd_open->device_ppi) / 10;
    lv_vnd_config.draw_buf_2_1 = LV_MEM_CUSTOM_ALLOC(lv_vnd_config.draw_pixel_size * sizeof(lv_color_t));
    lv_vnd_config.draw_buf_2_2 = NULL;
    lv_vnd_config.frame_buf_1 = (lv_color_t *)PSRAM_FRAME_BUFFER;
    lv_vnd_config.frame_buf_2 = (lv_color_t *)(PSRAM_FRAME_BUFFER + ppi_to_pixel_x(lcd_open->device_ppi) * ppi_to_pixel_y(lcd_open->device_ppi) * sizeof(lv_color_t));
#endif
    lv_vnd_config.lcd_hor_res = ppi_to_pixel_x(lcd_open->device_ppi);
    lv_vnd_config.lcd_ver_res = ppi_to_pixel_y(lcd_open->device_ppi);
    lv_vnd_config.rotation = ROTATE_NONE;

    lv_vendor_init(&lv_vnd_config);

    lcd_display_open(lcd_open);

#if (CONFIG_TP)
    drv_tp_open(ppi_to_pixel_x(lcd_open->device_ppi), ppi_to_pixel_y(lcd_open->device_ppi), TP_MIRROR_NONE);
#endif

    lv_example_meter();
    lv_vendor_start();
}

void lvgl_event_close_handle(media_mailbox_msg_t *msg)
{
//    lcd_display_close();

#if (CONFIG_TP)
    drv_tp_close();
#endif

    lv_example_meter_exit();

    lv_vendor_stop();

    lvgl_video_switch = 0;

    lv_vendor_deinit();
#ifdef CONFIG_LVGL_USE_PSRAM
    if (lv_vnd_config.draw_buf_2_1)
    {
        frame_buffer_display_free((frame_buffer_t *)lv_vnd_config.draw_buf_2_1);
        lv_vnd_config.draw_buf_2_1 = NULL;
    }
    if (lv_vnd_config.draw_buf_2_2)
    {
        frame_buffer_display_free((frame_buffer_t *)lv_vnd_config.draw_buf_2_2);
        lv_vnd_config.draw_buf_2_2 = NULL;
    }
#endif
    os_memset(&lv_vnd_config, 0, sizeof(lv_vnd_config_t));
}

void lvgl_event_handle(media_mailbox_msg_t *msg)
{
    switch (msg->event)
    {
        case EVENT_LVGL_OPEN_IND:
            lvgl_event_open_handle(msg);
            break;

        case EVENT_LVGL_CLOSE_IND:
            lvgl_event_close_handle(msg);
            break;

        default:
            break;
    }

    msg_send_rsp_to_media_major_mailbox(msg, BK_OK, APP_MODULE);
}


