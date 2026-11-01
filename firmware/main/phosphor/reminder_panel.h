// SPDX-License-Identifier: MIT
#pragma once
#include <lvgl.h>
#include <string>
#include <cstdint>
namespace phosphor {
class ReminderPanel {
public:
    explicit ReminderPanel(lv_obj_t* parent);
    ~ReminderPanel();
    bool done() const { return done_; }
    void update();
private:
    void refresh();
    lv_obj_t *panel_ = nullptr, *list_ = nullptr, *status_ = nullptr;
    bool done_ = false, dirty_ = true;
    int cancel_ = 0, minutes_ = 0;
    uint32_t revision_ = 0;
    std::string feedback_;
};
}
