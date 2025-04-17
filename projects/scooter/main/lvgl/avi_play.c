#include <os/os.h>
#if CONFIG_LVGL
#include "lvgl.h"
#include "lv_vendor.h"
#endif
#include "modules/avilib.h"
#include "modules/jpeg_decode_sw.h"
#include "media_evt.h"
#include "lv_jpeg_hw_decode.h"
#include "avi_play.h"

#define TAG "AVI"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)


#define AVI_VIDEO_USE_HW_DECODE    1
#define AVI_VIDEO_MAX_FRAME_LEN    (30 * 1024)

static bk_avi_play_t bk_avi_play = {0};
static lv_obj_t *img = NULL;
static lv_timer_t *timer = NULL;
static lv_img_dsc_t img_dsc =
{
    .header.cf = LV_IMG_CF_TRUE_COLOR,
    .header.always_zero = 0,
    .header.w = 0,
    .header.h = 0,
    .data_size = 0,
    .data = NULL,
};
extern void lv_example_meter(void);

static void bk_avi_play_config_free(bk_avi_play_t *avi_play)
{
    if (avi_play->avi)
    {
        AVI_close(avi_play->avi);
        avi_play->avi = NULL;
    }

    if (avi_play->video_frame)
    {
        psram_free(avi_play->video_frame);
        avi_play->video_frame = NULL;
    }

    if (avi_play->framebuffer)
    {
        psram_free(avi_play->framebuffer);
        avi_play->framebuffer = NULL;
    }

    avi_play->pos = 0;
}

static bk_err_t bk_avi_play_open(bk_avi_play_t *avi_play, const char *filename)
{
    LOGI("%s [%d]\r\n", __func__, __LINE__);

    os_memset(avi_play, 0x00, sizeof(bk_avi_play_t));

    lv_vendor_fs_init();

    avi_play->avi = AVI_open_input_file(filename, 1);
    if (avi_play->avi == NULL)
    {
        LOGE("%s open avi file failed\r\n", __func__);
        return BK_FAIL;
    }
    else
    {
        avi_play->video_num = AVI_video_frames(avi_play->avi);
        avi_play->frame_size = avi_play->avi->width * avi_play->avi->height * 2;
        LOGI("avi video_num: %d, width: %d, height: %d, frame_size: %d, fps: %d\r\n", avi_play->video_num, avi_play->avi->width, avi_play->avi->height, avi_play->frame_size, (uint32_t)avi_play->avi->fps);
    }

    avi_play->pos = 0;

    avi_play->video_frame = psram_malloc(AVI_VIDEO_MAX_FRAME_LEN);
    if (avi_play->video_frame == NULL)
    {
        LOGE("%s video_frame malloc fail\r\n", __func__);
        AVI_close(avi_play->avi);
        return BK_FAIL;
    }

    avi_play->framebuffer = psram_malloc(avi_play->frame_size);
    if (avi_play->framebuffer == NULL)
    {
        LOGE("%s framebuffer malloc fail\r\n", __func__);
        goto out;
    }

#if AVI_VIDEO_USE_HW_DECODE
    bk_jpeg_hw_decode_to_mem_init();
#else
    jd_output_format format = {0};

    bk_jpeg_dec_sw_init(NULL, 0);
    format.format = JD_FORMAT_RGB565;
    format.scale = 0;
    format.byte_order = JD_BIG_ENDIAN;
    jd_set_output_format(&format);
#endif
    LOGI("%s complete\r\n", __func__);

    return BK_OK;
out:
    bk_avi_play_config_free(avi_play);

    return BK_FAIL;
}

static void bk_avi_play_close(bk_avi_play_t *avi_play)
{
#if AVI_VIDEO_USE_HW_DECODE
    bk_jpeg_hw_decode_to_mem_deinit();
#else
    bk_jpeg_dec_sw_deinit();
#endif

    bk_avi_play_config_free(avi_play);

    lv_vendor_fs_deinit();

    LOGI("%s complete\r\n", __func__);
}

static bk_err_t bk_avi_video_prase_to_rgb565(bk_avi_play_t *avi_play)
{
    bk_err_t ret = BK_OK;

    ret = AVI_set_video_position(avi_play->avi, avi_play->pos, (long *)&avi_play->video_len);
    if (ret != BK_OK)
    {
        LOGE("%s %d AVI_set_video_position failed\r\n", __func__, __LINE__);
        return ret;
    }

    AVI_read_frame(avi_play->avi, (char *)avi_play->video_frame, avi_play->video_len);

    if (avi_play->video_len == 0)
    {
        avi_play->pos = avi_play->pos + 1;
        ret = AVI_set_video_position(avi_play->avi, avi_play->pos, (long *)&avi_play->video_len);
        if (ret != BK_OK)
        {
            LOGE("%s %d AVI_set_video_position failed\r\n", __func__, __LINE__);
            return ret;
        }

        AVI_read_frame(avi_play->avi, (char *)avi_play->video_frame, avi_play->video_len);
    }

#if AVI_VIDEO_USE_HW_DECODE
    ret = bk_jpeg_hw_decode_to_mem((uint8_t *)avi_play->video_frame, (uint8_t *)avi_play->framebuffer, avi_play->video_len, avi_play->avi->width, avi_play->avi->height);
    if (ret != BK_OK)
    {
        LOGE("%s %d bk_jpeg_hw_decode_to_mem failed\r\n", __func__, __LINE__);
        return ret;
    }
#else
    sw_jpeg_dec_res_t result;
    bk_jpeg_dec_sw_start(JPEGDEC_BY_FRAME, (uint8_t *)avi_play->video_frame, (uint8_t *)avi_play->framebuffer, avi_play->video_len, avi_play->frame_size, (sw_jpeg_dec_res_t *)&result);
#endif

    return ret;
}

void bk_avi_play_stop(void)
{
    lv_timer_del(timer);
    lv_obj_del(img);

    LOGI("%s complete\r\n", __func__);
}

static void lv_timer_cb(lv_timer_t *timer)
{
    bk_err_t ret = BK_OK;

    bk_avi_play.pos++;

    if (bk_avi_play.pos < bk_avi_play.video_num)
    {
        ret = bk_avi_video_prase_to_rgb565(&bk_avi_play);
        if (ret != BK_OK)
        {
            LOGE("%s %d bk_avi_video_prase_to_rgb565 failed\r\n", __func__, __LINE__);
            return;
        }

        lv_img_set_src(img, &img_dsc);
    }
    else
    {
        bk_avi_play_stop();
        bk_avi_play_close(&bk_avi_play);

        lv_example_meter();
    }
}

void bk_avi_play_start(void)
{
    bk_err_t ret = BK_OK;

    ret = bk_avi_play_open(&bk_avi_play, "/animation.avi");
    if (ret != BK_OK)
    {
        LOGE("%s bk_avi_play_open failed\r\n", __func__);
        return;
    }

    ret = bk_avi_video_prase_to_rgb565(&bk_avi_play);
    if (ret != BK_OK)
    {
        LOGE("%s %d bk_avi_video_prase_to_rgb565 failed\r\n", __func__, __LINE__);
        return;
    }

    img_dsc.header.w = bk_avi_play.avi->width;
    img_dsc.header.h = bk_avi_play.avi->height;
    img_dsc.data_size = img_dsc.header.w * img_dsc.header.h * 2;
    img_dsc.data = (const uint8_t *)bk_avi_play.framebuffer;

    lv_vendor_disp_lock();
    img = lv_img_create(lv_scr_act());
    lv_img_set_src(img, &img_dsc);
    lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);

    timer = lv_timer_create(lv_timer_cb, 1000 / bk_avi_play.avi->fps, NULL);
    lv_vendor_disp_unlock();

    LOGI("%s complete\r\n", __func__);
}

