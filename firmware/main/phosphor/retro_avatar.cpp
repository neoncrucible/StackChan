// SPDX-License-Identifier: MIT
#include "retro_avatar.h"
#include "audio_meter.h"
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
    lv_label_set_text(title, "PHOSPHOR // 01");

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
}

void RetroAvatar::setState(State state) {
    if (state_ == state) return;
    state_ = state;
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
    if (!(next == rendered_)) {
        rendered_ = next;
        lv_obj_invalidate(face_); // Only the face area, and only when it changes.
    }
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
