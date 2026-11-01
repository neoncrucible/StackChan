// SPDX-License-Identifier: MIT
#pragma once
#include <array>
#include <cstdint>
#include <string_view>
namespace phosphor {
struct Pose { int yaw, pitch; uint32_t after_ms; };
class Gesture {
public:
    bool start(std::string_view name, uint32_t now, int yaw, int pitch) {
        index_ = 0; started_ = now;
        if (name == "nod") poses_ = {{{yaw, pitch + 6, 0}, {yaw, pitch - 3, 450}, {yaw, pitch, 900}}};
        else if (name == "shake") poses_ = {{{yaw - 7, pitch, 0}, {yaw + 7, pitch, 450}, {yaw, pitch, 900}}};
        else if (name == "curious") poses_ = {{{yaw + 6, pitch + 4, 0}, {yaw + 6, pitch + 4, 700}, {yaw, pitch, 1400}}};
        else { cancel(); return false; }
        active_ = true; return true;
    }
    bool next(uint32_t now, Pose& pose) {
        if (!active_ || uint32_t(now - started_) < poses_[index_].after_ms) return false;
        pose = poses_[index_++];
        if (index_ == poses_.size()) active_ = false;
        return true;
    }
    void cancel() { active_ = false; }
    bool active() const { return active_; }
private:
    std::array<Pose, 3> poses_{};
    size_t index_ = 0;
    uint32_t started_ = 0;
    bool active_ = false;
};
}
