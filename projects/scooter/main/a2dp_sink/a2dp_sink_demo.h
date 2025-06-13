/**
 * @file a2dp_sink_demo.h
 *
 */

#ifndef A2DP_SINK_DEMO_H
#define A2DP_SINK_DEMO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

int a2dp_sink_demo_init(uint8_t aac_supported);
int a2dp_sink_demo_deinit(void);
int32_t bk_bt_app_avrcp_ct_get_attr(uint32_t attr);
void bk_bt_a2dp_sink_demo_debug_info(void);
void bk_bt_a2dp_sink_demo_set_auto_accept_connect_req(uint8_t accept);
int32_t bk_bt_a2dp_sink_demo_try_connect();
int32_t bk_bt_a2dp_sink_demo_try_disconnect_current();

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*A2DP_SINK_DEMO_H*/
