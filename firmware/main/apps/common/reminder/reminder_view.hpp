/* SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 * SPDX-License-Identifier: MIT */
#pragma once
#include <stackchan/avatar/avatar/decorator.h>
#include <phosphor/ui.h>
#include <string>
#include <string_view>
namespace tools { int create_reminder(uint32_t, std::string_view, bool); const char* reminder_error(int); }
namespace view {
class ReminderView : public stackchan::avatar::Decorator {
public:
    ReminderView(lv_obj_t* parent, std::string_view message) : message_(message) {
        panel_ = phosphor::panel(parent, 0, 0, 320, 240);
        phosphor::text(panel_, "PHOSPHOR / REMINDER", 20, 25, 280, &lv_font_unscii_8);
        label_ = phosphor::text(panel_, message_.c_str(), 20, 58, 280, &lv_font_montserrat_20);
        lv_obj_set_height(label_, 108);
        phosphor::button(panel_, "SNOOZE 5m", 16, 185, 140, 40, [](lv_event_t* e) {
            static_cast<ReminderView*>(lv_event_get_user_data(e))->snooze_ = true;
        }, this);
        phosphor::button(panel_, "DONE", 164, 185, 140, 40, [](lv_event_t* e) {
            static_cast<ReminderView*>(lv_event_get_user_data(e))->requestDestroy();
        }, this);
    }
    ~ReminderView() override { lv_obj_delete(panel_); }
    void _update() override {
        if (!snooze_) return;
        snooze_ = false;
        int id = tools::create_reminder(300000, message_, false);
        if (id > 0) requestDestroy();
        else lv_label_set_text(label_, tools::reminder_error(id));
    }
private:
    lv_obj_t *panel_, *label_;
    std::string message_;
    bool snooze_ = false;
};
}
