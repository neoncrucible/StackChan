// SPDX-License-Identifier: MIT
#pragma once
#include "face.h"
#include <atomic>

namespace phosphor {
// Audio publishes two scalars; it never calls LVGL or waits for the display.
inline std::atomic<int> output_level{0};
inline std::atomic<uint32_t> output_tick{0};
inline void publish_audio(const int16_t* pcm, int count, uint32_t now) {
    output_level.store(pcm_level(pcm, count), std::memory_order_relaxed);
    output_tick.store(now, std::memory_order_release);
}
inline int read_audio(uint32_t now) {
    const uint32_t tick = output_tick.load(std::memory_order_acquire);
    return now - tick <= 160 ? output_level.load(std::memory_order_relaxed) : 0;
}
} // namespace phosphor
