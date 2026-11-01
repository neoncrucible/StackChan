// SPDX-License-Identifier: MIT
#include "retro_avatar.h"
#include "audio_meter.h"
#include "ui.h"
#include "reminder_engine.h"
#include <apps/common/reminder/reminder.h>
#include <hal/board/hal_bridge.h>
#include <stackchan/stackchan.h>
#include <ctime>
#include <cstdio>
#include <hal/hal.h>
#include <algorithm>
#include <string>

namespace phosphor {
using namespace stackchan::avatar;

class TerminalSpeech final : public SpeechBubble {
public:
    explicit TerminalSpeech(lv_obj_t* parent) {
        label_ = lv_label_create(parent);
        lv_obj_set_pos(label_, 20, 188);
        lv_obj_set_size(label_, 280, 36);
        lv_label_set_long_mode(label_, LV_LABEL_LONG_CLIP);
        lv_obj_set_style_text_font(label_, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(label_, lv_color_hex(green), 0);
        lv_obj_set_style_text_align(label_, LV_TEXT_ALIGN_CENTER, 0);
        lv_label_set_text(label_, "");
        lv_obj_remove_flag(label_, LV_OBJ_FLAG_CLICKABLE);
    }
    ~TerminalSpeech() override { lv_obj_delete(label_); }
    void setSpeech(std::string_view text) override {
        size_t n = std::min<size_t>(text.size(), 256);
        // Do not split a UTF-8 code point at the caption limit.
        while (n < text.size() && n && (static_cast<unsigned char>(text[n]) & 0xc0) == 0x80) --n;
        const std::string next(text.substr(0, n));
        if (next != text_) { text_ = next; lv_label_set_text(label_, text_.c_str()); }
    }
    void clearSpeech() override { setSpeech(""); }
    void setVisible(bool visible) override {
        if (visible) lv_obj_remove_flag(label_, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(label_, LV_OBJ_FLAG_HIDDEN);
    }
    void setTextFont(void* font) override {
        if (font) lv_obj_set_style_text_font(label_, static_cast<const lv_font_t*>(font), 0);
    }
private:
    lv_obj_t* label_;
    std::string text_;
};

RetroAvatar::~RetroAvatar() {
    // Elements own children of panel_: destroy them before their parent.
    reminder_panel_.reset();
    clearDecorators();
    _key_elements = {};
    panel_.reset();
}

void RetroAvatar::init(lv_obj_t* parent) {
    panel_ = std::make_unique<uitk::lvgl_cpp::Container>(parent);
    panel_->setSize(320, 240);
    panel_->align(LV_ALIGN_CENTER, 0, 0);
    panel_->setRadius(0);
    panel_->setBorderWidth(0);
    panel_->setPadding(0, 0, 0, 0);
    panel_->setBgColor(lv_color_hex(background));
    panel_->removeFlag(LV_OBJ_FLAG_SCROLLABLE);

    auto title = lv_label_create(panel_->get());
    lv_obj_set_pos(title, 20, 18);
    lv_obj_set_style_text_font(title, &lv_font_unscii_8, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(green), 0);
    lv_label_set_text(title, "PHOSPHOR // 02");

    status_ = lv_label_create(panel_->get());
    lv_obj_set_pos(status_, 216, 18);
    lv_obj_set_style_text_font(status_, &lv_font_unscii_8, 0);
    lv_obj_set_style_text_color(status_, lv_color_hex(green), 0);
    lv_label_set_text(status_, "READY");

    face_ = lv_obj_create(panel_->get());
    lv_obj_remove_style_all(face_);
    lv_obj_set_pos(face_, 32, 38);
    lv_obj_set_size(face_, 256, 144);
    lv_obj_remove_flag(face_, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(face_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(face_, draw, LV_EVENT_DRAW_MAIN, this);

    // Model-only features let the existing local blink and emotion controls
    // operate without creating per-feature LVGL objects or image decoders.
    _key_elements.leftEye = std::make_unique<Feature>();
    _key_elements.rightEye = std::make_unique<Feature>();
    _key_elements.mouth = std::make_unique<Feature>();
    _key_elements.leftEye->setWeight(100);
    _key_elements.rightEye->setWeight(100);
    _key_elements.speechBubble = std::make_unique<TerminalSpeech>(panel_->get());
    idle_text_ = text(panel_->get(), "", 20, 184, 280);
    lv_obj_set_height(idle_text_, 37);
    lv_obj_add_flag(idle_text_, LV_OBJ_FLAG_HIDDEN);
    reminder_button_ = button(panel_->get(), "REMINDERS", 20, 218, 112, 20, [](lv_event_t* e) {
        static_cast<RetroAvatar*>(lv_event_get_user_data(e))->showReminders();
    }, this);
    // A separate small button preserves tap-to-talk on the face.
    auto* label = lv_obj_get_child(reminder_button_, 0);
    lv_obj_set_style_text_font(label, &lv_font_unscii_8, 0);
}

void RetroAvatar::setState(State state) {
    if (state_ == state) return;
    const State previous = state_;
    state_ = state;
    state_since_ = GetHAL().millis();
    info_tick_ = state_since_ - 1000;
    if (previous == State::Speaking) {
        nod_pending_ = idle_level_ > 0;
        reply_ended_ = state_since_;
    }
    if (state != State::Ready) {
        cancelGesture();
        leftEye().setPosition({0, 0}); rightEye().setPosition({0, 0});
    }
    const char* label = "READY";
    switch (state) {
        case State::Listening: label = "LISTEN"; break;
        case State::Speaking: label = "VOICE"; break;
        case State::Connecting: label = "LINKING"; break;
        case State::Notice: label = "STATUS"; break;
        default: break;
    }
    lv_label_set_text(status_, label);
    lv_obj_set_style_text_color(status_, lv_color_hex(
        state == State::Notice || state == State::Connecting ? amber : green), 0);
}

void RetroAvatar::setEmotion(const Emotion& emotion) {
    Avatar::setEmotion(emotion);
    // Small changes to the terminal eyes, without adding decorators or assets.
    int left = 100, right = 100;
    if (emotion == Emotion::Sleepy) left = right = 24;
    else if (emotion == Emotion::Happy) left = right = 65;
    else if (emotion == Emotion::Sad) left = right = 45;
    else if (emotion == Emotion::Angry) left = right = 35;
    else if (emotion == Emotion::Doubt) left = 55;
    leftEye().setWeight(left);
    rightEye().setWeight(right);
}

void RetroAvatar::update() {
    Avatar::update();
    const uint32_t now = GetHAL().millis();
    updateCompanion(now);
    if (now - last_frame_ < 40) return; // Face at 25 Hz, motor loop at 50 Hz.
    last_frame_ = now;
    envelope_ = envelope_step(envelope_, state_ == State::Speaking ? read_audio(now) : 0);
    Face next;
    next.left_open = leftEye().getWeight();
    next.right_open = rightEye().getWeight();
    next.look_x = leftEye().getPosition().x;
    next.look_y = leftEye().getPosition().y;
    next.mouth = envelope_;
    next.state = state_;
    switch (getEmotion()) {
        case Emotion::Happy: next.expression = Expression::Happy; break;
        case Emotion::Sad: next.expression = Expression::Sad; break;
        case Emotion::Angry: next.expression = Expression::Angry; break;
        case Emotion::Doubt: next.expression = Expression::Curious; break;
        case Emotion::Sleepy: next.expression = Expression::Sleepy; break;
        default: next.expression = Expression::Neutral; break;
    }
    if (state_ == State::Connecting) next.expression = Expression::Curious;
    if (!(next == rendered_)) {
        rendered_ = next;
        lv_obj_invalidate(face_); // Only the face area, and only when it changes.
    }
}

void RetroAvatar::showReminders() {
    if (state_ == State::Listening || state_ == State::Speaking) return;
    pauseGestures();
    if (!reminder_panel_) reminder_panel_ = std::make_unique<ReminderPanel>(panel_->get());
}
void RetroAvatar::pauseGestures(uint32_t duration) {
    cancelGesture();
    pause_since_ = GetHAL().millis(); pause_ms_ = duration;
}
void RetroAvatar::cancelGesture() {
    gesture_.cancel();
    if (motion_owned_) {
        auto& motion = GetStackChan().motion();
        motion.yawServo().pauseAnimation(); motion.pitchServo().pauseAnimation();
    }
    motion_owned_ = false;
}
bool RetroAvatar::gesture(std::string_view name) {
    const auto now = GetHAL().millis();
    if (state_ != State::Ready || reminder_panel_ || alert_active_ || (now - pause_since_ < pause_ms_)) return false;
    if (name != "nod" && name != "shake" && name != "curious") return false;
    auto& motion = GetStackChan().motion();
    const int yaw = std::clamp(motion.yawServo().getCurrentAngle() / 10, -40, 40);
    const int pitch = std::clamp(motion.pitchServo().getCurrentAngle() / 10, 14, 60);
    gesture_.start(name, now, yaw, pitch);
    owned_since_ = now; gesture_tick_ = now; motion_owned_ = true;
    return true;
}
void RetroAvatar::updateCompanion(uint32_t now) {
    if (reminder_panel_) {
        reminder_panel_->update();
        if (reminder_panel_->done() || state_ == State::Listening || state_ == State::Speaking)
            reminder_panel_.reset();
    }
    if (motion_owned_ && now - owned_since_ > 3500) motion_owned_ = false;
    if (nod_pending_ && now - reply_ended_ > 15000) nod_pending_ = false;
    if (state_ == State::Ready && !reminder_panel_ && !alert_active_ && getEmotion() != Emotion::Sleepy) {
        if (nod_pending_) {
            nod_pending_ = false;
            if (idle_level_ > 0) gesture("nod");
        }
        Pose pose;
        if (gesture_.next(now, pose)) {
            auto& motion = GetStackChan().motion();
            motion.yawServo().moveWithSpeed(std::clamp(pose.yaw, -45, 45) * 10, 180);
            motion.pitchServo().moveWithSpeed(std::clamp(pose.pitch, 8, 75) * 10, 180);
        }
        const uint32_t interval = idle_level_ == 1 ? 45000 : idle_level_ == 2 ? 30000 : 20000;
        if (idle_level_ && now - gesture_tick_ > interval && now - pause_since_ >= pause_ms_) {
            gesture("curious");
            const int look = (++idle_cycle_ % 2) ? 25 : -25;
            leftEye().setPosition({look, 0}); rightEye().setPosition({look, 0});
        }
    }
    if (now - info_tick_ < 1000) return;
    info_tick_ = now;
    const bool idle = state_ == State::Ready && now - state_since_ >= 8000;
    if (idle) {
        time_t now_time = time(nullptr); struct tm local{}; localtime_r(&now_time, &local);
        char clock[16] = "--:--";
        if (now_time >= clock_epoch) strftime(clock, sizeof(clock), "%H:%M", &local);
        int battery = -1; bool charging = false;
        const bool battery_valid = hal_bridge::board_get_battery_status(battery, charging);
        std::string next = std::string(clock) + "   BAT " +
            (battery_valid && battery >= 0 && battery <= 100 ? std::to_string(battery) + "%" : "--") +
            (battery_valid && charging ? " +" : "");
        auto reminders = tools::get_active_reminders();
        if (reminders.empty()) next += "\nReady when you are.";
        else if (now_time < clock_epoch) next += "\nClock unset / reminders waiting";
        else {
            const auto seconds = reminders.front().remainingSeconds;
            next += "\nNext " + (seconds < 60 ? std::to_string(seconds) + "s" :
                std::to_string(seconds / 3600) + "h " + std::to_string((seconds / 60) % 60) + "m");
            next += ": " + reminders.front().message;
        }
        if (next != idle_line_) { idle_line_ = next; lv_label_set_text(idle_text_, next.c_str()); }
        _key_elements.speechBubble->setVisible(false);
        lv_obj_remove_flag(idle_text_, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(idle_text_, LV_OBJ_FLAG_HIDDEN);
        _key_elements.speechBubble->setVisible(true);
    }
    if (state_ != State::Listening && state_ != State::Speaking)
        lv_obj_remove_flag(reminder_button_, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(reminder_button_, LV_OBJ_FLAG_HIDDEN);
}

void RetroAvatar::draw(lv_event_t* event) {
    auto self = static_cast<RetroAvatar*>(lv_event_get_user_data(event));
    lv_area_t origin;
    lv_obj_get_coords(self->face_, &origin);
    auto layer = lv_event_get_layer(event);
    draw_face(self->rendered_, [&](int x, int y, int w, int h, uint32_t color) {
        lv_draw_rect_dsc_t dsc;
        lv_draw_rect_dsc_init(&dsc);
        dsc.bg_color = lv_color_hex(color);
        dsc.bg_opa = LV_OPA_COVER;
        const lv_area_t area = {origin.x1 + x, origin.y1 + y, origin.x1 + x + w - 1, origin.y1 + y + h - 1};
        lv_draw_rect(layer, &dsc, &area);
    });
}
} // namespace phosphor
