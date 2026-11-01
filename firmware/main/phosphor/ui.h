// SPDX-License-Identifier: MIT
#pragma once
#include "face.h"
#include <lvgl.h>
namespace phosphor {
inline lv_obj_t* text(lv_obj_t* parent, const char* value, int x, int y, int width,
                      const lv_font_t* font = &lv_font_montserrat_16) {
    auto* label = lv_label_create(parent);
    lv_obj_set_pos(label, x, y); lv_obj_set_width(label, width);
    lv_label_set_long_mode(label, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(green), 0);
    lv_label_set_text(label, value);
    lv_obj_remove_flag(label, LV_OBJ_FLAG_CLICKABLE);
    return label;
}
inline lv_obj_t* panel(lv_obj_t* parent, int x, int y, int w, int h) {
    auto* p = lv_obj_create(parent);
    lv_obj_set_pos(p, x, y); lv_obj_set_size(p, w, h);
    lv_obj_set_style_bg_color(p, lv_color_hex(background), 0);
    lv_obj_set_style_border_width(p, 0, 0); lv_obj_set_style_radius(p, 0, 0);
    lv_obj_set_style_pad_all(p, 0, 0); lv_obj_remove_flag(p, LV_OBJ_FLAG_SCROLLABLE);
    return p;
}
inline lv_obj_t* button(lv_obj_t* parent, const char* value, int x, int y, int w, int h,
                        lv_event_cb_t fn, void* user) {
    auto* b = lv_button_create(parent);
    lv_obj_set_pos(b, x, y); lv_obj_set_size(b, w, h);
    lv_obj_set_style_bg_color(b, lv_color_hex(dim), 0);
    lv_obj_set_style_border_color(b, lv_color_hex(green), 0);
    lv_obj_set_style_border_width(b, 1, 0); lv_obj_set_style_radius(b, 3, 0);
    lv_obj_set_style_shadow_width(b, 0, 0); lv_obj_set_style_pad_all(b, 0, 0);
    auto* t = text(b, value, 0, 0, w - 4);
    lv_obj_set_style_text_align(t, LV_TEXT_ALIGN_CENTER, 0); lv_obj_center(t);
    lv_obj_remove_flag(b, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_event_cb(b, fn, LV_EVENT_CLICKED, user);
    return b;
}
}
