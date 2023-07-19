/*
 *
 * Copyright (c) 2023 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 *
 */
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/services/bas.h>
#include <zephyr/random/rand32.h>
#include <zmk/display.h>
#include "peripheral_status.h"
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/split/bluetooth/peripheral.h>
#include <zmk/events/split_peripheral_status_changed.h>
#include <zmk/usb.h>
#include <zmk/ble.h>
struct Animation {
    const int frameCount;
    const int timeGap;
    const int repetition;
    const void *images[];
};

LV_IMG_DECLARE(kronii_001)
LV_IMG_DECLARE(kronii_002)
LV_IMG_DECLARE(kronii_003)
LV_IMG_DECLARE(kronii_004)
LV_IMG_DECLARE(kronii_005)
LV_IMG_DECLARE(kronii_006)
LV_IMG_DECLARE(kronii_007)
LV_IMG_DECLARE(kronii_008)
LV_IMG_DECLARE(kronii_009)
LV_IMG_DECLARE(kronii_010)

LV_IMG_DECLARE(marine_001)
LV_IMG_DECLARE(marine_002)
LV_IMG_DECLARE(marine_003)
LV_IMG_DECLARE(marine_004)
LV_IMG_DECLARE(marine_005)
LV_IMG_DECLARE(marine_006)
LV_IMG_DECLARE(marine_007)
LV_IMG_DECLARE(marine_008)
LV_IMG_DECLARE(marine_009)
LV_IMG_DECLARE(marine_010)

const struct Animation kronii_anim = {.frameCount = 10,
                                      .timeGap = 100,
                                      .repetition = 10,
                                      .images = {&kronii_001, &kronii_002, &kronii_003, &kronii_004,
                                                 &kronii_005, &kronii_006, &kronii_007, &kronii_008,
                                                 &kronii_009, &kronii_010}};

const struct Animation marine_anim = {.frameCount = 10,
                                      .timeGap = 100,
                                      .repetition = 10,
                                      .images = {&marine_001, &marine_002, &marine_003, &marine_004,
                                                 &marine_005, &marine_006, &marine_007, &marine_008,
                                                 &marine_009, &marine_010}};

int currentAnimIdx = 0;
int animsLength = 2;
const struct Animation *anims[] = {&kronii_anim, &marine_anim};

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);
struct peripheral_status_state {
    bool connected;
};
static void draw_top(lv_obj_t *widget, lv_color_t cbuf[], struct status_state state) {
    lv_obj_t *canvas = lv_obj_get_child(widget, 0);
    lv_draw_label_dsc_t label_dsc;
    init_label_dsc(&label_dsc, LVGL_FOREGROUND, &lv_font_montserrat_16, LV_TEXT_ALIGN_RIGHT);
    lv_draw_rect_dsc_t rect_black_dsc;
    init_rect_dsc(&rect_black_dsc, LVGL_BACKGROUND);
    // Fill background
    lv_canvas_draw_rect(canvas, 0, 0, DISP_WIDTH, 20, &rect_black_dsc);
    // Draw battery
    draw_battery(canvas, state);
    // Draw output status
    lv_canvas_draw_text(canvas, 0, 0, CANVAS_SIZE, &label_dsc,
                        state.connected ? LV_SYMBOL_WIFI : LV_SYMBOL_CLOSE);
    // Rotate canvas
    rotate_canvas(canvas, cbuf);
}
static void set_battery_status(struct zmk_widget_status *widget,
                               struct battery_status_state state) {
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
    widget->state.charging = state.usb_present;
#endif /* IS_ENABLED(CONFIG_USB_DEVICE_STACK) */
    widget->state.battery = state.level;
    draw_top(widget->obj, widget->cbuf, widget->state);
}
static void battery_status_update_cb(struct battery_status_state state) {
    struct zmk_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_battery_status(widget, state); }
}
static struct battery_status_state battery_status_get_state(const zmk_event_t *eh) {
    return (struct battery_status_state) {
        .level = bt_bas_get_battery_level(),
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
        .usb_present = zmk_usb_is_powered(),
#endif /* IS_ENABLED(CONFIG_USB_DEVICE_STACK) */
    };
}
ZMK_DISPLAY_WIDGET_LISTENER(widget_battery_status, struct battery_status_state,
                            battery_status_update_cb, battery_status_get_state)
ZMK_SUBSCRIPTION(widget_battery_status, zmk_battery_state_changed);
#if IS_ENABLED(CONFIG_USB_DEVICE_STACK)
ZMK_SUBSCRIPTION(widget_battery_status, zmk_usb_conn_state_changed);
#endif /* IS_ENABLED(CONFIG_USB_DEVICE_STACK) */
static struct peripheral_status_state get_state(const zmk_event_t *_eh) {
    return (struct peripheral_status_state){.connected = zmk_split_bt_peripheral_is_connected()};
}
static void set_connection_status(struct zmk_widget_status *widget,
                                  struct peripheral_status_state state) {
    widget->state.connected = state.connected;
    draw_top(widget->obj, widget->cbuf, widget->state);
}
static void output_status_update_cb(struct peripheral_status_state state) {
    struct zmk_widget_status *widget;
    SYS_SLIST_FOR_EACH_CONTAINER(&widgets, widget, node) { set_connection_status(widget, state); }
}
ZMK_DISPLAY_WIDGET_LISTENER(widget_peripheral_status, struct peripheral_status_state,
                            output_status_update_cb, get_state)
ZMK_SUBSCRIPTION(widget_peripheral_status, zmk_split_peripheral_status_changed);
const void **images;
uint8_t images_len;
struct zmk_widget_status *lastWidget;
void set_img_src(void *var, int32_t val);
void set_anim() {
    // Params
    int anim_len = anims[currentAnimIdx]->frameCount;
    int per_frame_time_in_ms = anims[currentAnimIdx]->timeGap;
    // Init animations
    lv_anim_init(&lastWidget->anim);
    lv_anim_set_var(&lastWidget->anim, lastWidget->obj);
    lv_anim_set_time(&lastWidget->anim, anim_len * per_frame_time_in_ms);
    lv_anim_set_values(&lastWidget->anim, 0, anim_len - 1);
    lv_anim_set_exec_cb(&lastWidget->anim, (lv_anim_exec_xcb_t)set_img_src);
    lv_anim_set_repeat_count(&lastWidget->anim, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_delay(&lastWidget->anim, 0);
    lv_anim_start(&lastWidget->anim);
}
long long int framesPlayed = 0;
void set_img_src(void *var, int32_t val) {
    lv_obj_t *img = (lv_obj_t *)var;

    lv_img_set_src(img, anims[currentAnimIdx]->images[val]);
    if (anims[currentAnimIdx]->repetition != -1) {
        framesPlayed++;
        if (anims[currentAnimIdx]->repetition * anims[currentAnimIdx]->frameCount == framesPlayed) {
            currentAnimIdx++;
            framesPlayed = 0;
            if (currentAnimIdx >= animsLength) {
                currentAnimIdx = 0;
            }
            set_anim();
        }
    }
}
int zmk_widget_status_init(struct zmk_widget_status *widget, lv_obj_t *parent) {
    lastWidget = widget;
    widget->obj = lv_img_create(parent);

    lv_obj_set_size(widget->obj, 160, DISP_WIDTH);
    lv_obj_t *top = lv_canvas_create(widget->obj);
    lv_obj_align(top, LV_ALIGN_TOP_RIGHT, DISP_WIDTH, 0);
    lv_canvas_set_buffer(top, widget->cbuf, DISP_WIDTH, 20, LV_IMG_CF_TRUE_COLOR);
    set_anim();
    sys_slist_append(&widgets, &widget->node);
    widget_battery_status_init();
    widget_peripheral_status_init();
    return 0;
}
lv_obj_t *zmk_widget_status_obj(struct zmk_widget_status *widget) { return widget->obj; }
