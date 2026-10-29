// SPDX-License-Identifier: MIT
#pragma once
#include "face.h"
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
private:
    static void draw(lv_event_t* event);
    std::unique_ptr<uitk::lvgl_cpp::Container> panel_;
    lv_obj_t* face_ = nullptr;
    lv_obj_t* status_ = nullptr;
    Face rendered_;
    State state_ = State::Ready;
    uint32_t last_frame_ = 0;
    int envelope_ = 0;
};
} // namespace phosphor
