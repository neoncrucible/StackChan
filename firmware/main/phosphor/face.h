// SPDX-License-Identifier: MIT
// Original PHOSPHOR face. Shared by the device and the desktop SVG preview.
#pragma once
#include <algorithm>
#include <cstdint>
#include <cstdlib>

namespace phosphor {
enum class State { Ready, Listening, Speaking, Connecting, Notice };
enum class Expression { Neutral, Happy, Sad, Angry, Curious, Sleepy };
constexpr uint32_t background = 0x07120e;
constexpr uint32_t dim = 0x183e2a;
constexpr uint32_t green = 0xa4ff7c;
constexpr uint32_t amber = 0xffbf69;
struct Face {
    int left_open = 100, right_open = 100;
    int look_x = 0, look_y = 0;
    int mouth = 0;
    State state = State::Ready;
    Expression expression = Expression::Neutral;
    bool operator==(const Face&) const = default;
};

// Coordinates are local to a 256 x 144 object. No bitmap, framebuffer or heap
// allocation is needed. The renderer emits only filled integer rectangles.
template<class Rect> void draw_face(const Face& f, Rect rect) {
    const auto ink = (f.state == State::Notice || f.state == State::Connecting) ? amber : green;
    rect(0, 0, 256, 144, background);
    // Broken corners and a low contrast CRT grille.
    for (int x : {8, 230}) {
        rect(x, 7, 18, 2, dim); rect(x, 135, 18, 2, dim);
    }
    for (int x : {8, 246}) {
        rect(x, 7, 2, 18, dim); rect(x, 119, 2, 18, dim);
    }
    for (int y = 18; y < 134; y += 8) rect(18, y, 220, 1, 0x0b1b13);
    const int dx = std::clamp(f.look_x, -100, 100) / 10;
    const int dy = std::clamp(f.look_y, -100, 100) / 14;
    for (int side = 0; side < 2; ++side) {
        const int x = (side == 0 ? 42 : 164) + dx;
        const int h = 2 + std::clamp(side == 0 ? f.left_open : f.right_open, 0, 100) * 30 / 100;
        const int y = 63 - h / 2 + dy;
        rect(x - 6, 38 + dy, 56, 2, dim);
        rect(x, y, 44, h, ink);
        // Cutout gives each eye a squared, old terminal cursor shape.
        if (h > 8) rect(x + 8, y + 4, 28, h - 8, background);
        rect(x + (side ? 2 : 34), y + h - 4, 8, 4, ink);
        // Block brows keep expressions readable without sprite assets.
        if (f.expression == Expression::Angry) {
            rect(x, side ? 35 : 31, 20, 3, ink);
            rect(x + 24, side ? 31 : 35, 20, 3, ink);
        } else if (f.expression == Expression::Curious && side == 0) {
            rect(x, 29, 44, 3, ink);
        } else if (f.expression == Expression::Sad) {
            rect(x, side ? 30 : 34, 20, 2, ink);
            rect(x + 24, side ? 34 : 30, 20, 2, ink);
        }
    }
    constexpr int shape[] = {2, 3, 5, 7, 9, 7, 5, 3, 2};
    const int level = std::clamp(f.mouth, 0, 100);
    for (int i = 0; i < 9; ++i) {
        const int h = 2 + shape[i] * level / 30;
        int curve = 0;
        if (f.state != State::Speaking && f.expression == Expression::Happy)
            curve = 5 - std::abs(4 - i) * 2;
        else if (f.state != State::Speaking && f.expression == Expression::Sad)
            curve = std::abs(4 - i) * 2 - 5;
        rect(76 + i * 12, 113 + curve - h / 2, 8, h, ink);
    }
    if (f.state == State::Listening) {
        rect(116, 22, 4, 4, ink); rect(126, 18, 4, 8, ink); rect(136, 22, 4, 4, ink);
    }
}

inline int pcm_level(const int16_t* pcm, int count) {
    if (!pcm || count <= 0) return 0;
    uint32_t total = 0, n = 0;
    // Sparse envelope sampling is enough for a face, at bounded CPU cost.
    const int stride = std::max(1, count / 64);
    for (int i = 0; i < count; i += stride) {
        const int v = pcm[i];
        total += v < 0 ? -v : v;
        ++n;
    }
    return static_cast<int>(std::min<uint32_t>(100, total / n * 100U / 6000U));
}
inline int envelope_step(int current, int target) {
    const int delta = target - current;
    return current + (delta > 0 ? (delta * 3 + 3) / 4 : (delta - 2) / 3);
}
} // namespace phosphor
