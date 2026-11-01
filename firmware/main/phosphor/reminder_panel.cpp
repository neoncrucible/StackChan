// SPDX-License-Identifier: MIT
#include "reminder_panel.h"
#include "ui.h"
#include <apps/common/reminder/reminder.h>
#include <cstdio>
#include <ctime>
namespace phosphor {
ReminderPanel::ReminderPanel(lv_obj_t* parent) {
    panel_ = panel(parent, 0, 0, 320, 240);
    text(panel_, "PHOSPHOR / REMINDERS", 12, 8, 260, &lv_font_unscii_8);
    button(panel_, "X", 274, 4, 38, 30, [](lv_event_t* e) {
        static_cast<ReminderPanel*>(lv_event_get_user_data(e))->done_ = true;
    }, this);
    status_ = text(panel_, "Local timers / tap X to cancel a row", 12, 36, 296);
    lv_obj_set_height(status_, 38);
    list_ = panel(panel_, 8, 76, 304, 108);
    lv_obj_add_flag(list_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(list_, LV_DIR_VER);
    for (int i = 0; i < 3; ++i) {
        const int minutes[] = {5, 15, 60};
        const char* labels[] = {"+5 min", "+15 min", "+1 hour"};
        auto* b = button(panel_, labels[i], 8 + i * 103, 193, 98, 38, [](lv_event_t* e) {
            auto* self = static_cast<ReminderPanel*>(lv_event_get_user_data(e));
            self->minutes_ = static_cast<int>(reinterpret_cast<intptr_t>(lv_obj_get_user_data(lv_event_get_target_obj(e))));
        }, this);
        lv_obj_set_user_data(b, reinterpret_cast<void*>(static_cast<intptr_t>(minutes[i])));
    }
    refresh();
}
ReminderPanel::~ReminderPanel() { lv_obj_delete(panel_); }
void ReminderPanel::refresh() {
    revision_ = tools::reminder_revision();
    lv_obj_clean(list_);
    auto rows = tools::get_active_reminders();
    int y = 0;
    for (const auto& r : rows) {
        struct tm tm{}; time_t due = r.dueUnix; localtime_r(&due, &tm);
        char stamp[32]; strftime(stamp, sizeof(stamp), "%d %b %H:%M", &tm);
        std::string line = std::string(stamp) + (r.repeat ? " [R]\n" : "\n") + r.message;
        auto* label = text(list_, line.c_str(), 4, y, 246); lv_obj_set_height(label, 44);
        auto* b = button(list_, "X", 254, y + 3, 38, 38, [](lv_event_t* e) {
            auto* self = static_cast<ReminderPanel*>(lv_event_get_user_data(e));
            self->cancel_ = static_cast<int>(reinterpret_cast<intptr_t>(lv_obj_get_user_data(lv_event_get_target_obj(e))));
        }, this);
        lv_obj_set_user_data(b, reinterpret_cast<void*>(static_cast<intptr_t>(r.id)));
        y += 50;
    }
    if (rows.empty()) text(list_, "Nothing pending. A rare luxury.", 4, 10, 290);
    lv_label_set_text(status_, feedback_.empty() ? "Saved on this robot / up to eight" : feedback_.c_str());
    dirty_ = false;
}
void ReminderPanel::update() {
    if (cancel_) { feedback_ = tools::stop_reminder(cancel_) ? "Cancelled." : "Cancel failed. Please try again."; cancel_ = 0; dirty_ = true; }
    if (minutes_) {
        int id = tools::create_reminder(minutes_ * 60000, "Timer complete.", false);
        feedback_ = id > 0 ? "Saved. I'll keep an eye on the clock." : tools::reminder_error(id);
        minutes_ = 0; dirty_ = true;
    }
    if (dirty_ || revision_ != tools::reminder_revision()) refresh();
}
}
