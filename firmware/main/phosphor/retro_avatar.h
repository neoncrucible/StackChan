// SPDX-License-Identifier: MIT
#pragma once
#include "face.h"
#include "gestures.h"
#include "reminder_panel.h"
#include <stackchan/avatar/avatar/avatar.h>
#include <smooth_lvgl.hpp>
#include <memory>

namespace phosphor {
class RetroAvatar final : public stackchan::avatar::Avatar {
public:
    ~RetroAvatar() override;
    void init(lv_obj_t* parent);
    uitk::lvgl_cpp::Container* getPanel() const { return panel_.get(); }
    void update() override;
    void setEmotion(const stackchan::avatar::Emotion& emotion) override;
    void setState(State state);
    bool gesture(std::string_view name);
    void pauseGestures(uint32_t duration = 15000);
    void setAlertActive(bool active) { if (active) cancelGesture(); alert_active_ = active; }
    void setIdleMotionLevel(uint8_t level) { idle_level_ = level; }
    void showReminders();
    bool panelOpen() const { return reminder_panel_ != nullptr; }
private:
    static void draw(lv_event_t* event);
    std::unique_ptr<uitk::lvgl_cpp::Container> panel_;
    lv_obj_t* face_ = nullptr;
    lv_obj_t* status_ = nullptr;
    Face rendered_;
    State state_ = State::Ready;
    uint32_t last_frame_ = 0;
    int envelope_ = 0;
    lv_obj_t *idle_text_ = nullptr, *reminder_button_ = nullptr;
    std::unique_ptr<ReminderPanel> reminder_panel_;
    Gesture gesture_;
    uint8_t idle_level_ = 1;
    uint32_t state_since_ = 0, info_tick_ = 0, gesture_tick_ = 0, pause_since_ = 0, pause_ms_ = 0;
    uint32_t owned_since_ = 0;
    bool motion_owned_ = false;
    bool alert_active_ = false;
    unsigned idle_cycle_ = 0;
    std::string idle_line_;
    void updateCompanion(uint32_t now);
    void cancelGesture();
};
} // namespace phosphor
