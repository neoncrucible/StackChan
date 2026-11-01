// SPDX-License-Identifier: MIT
#include <phosphor/face.h>
#include <phosphor/audio_meter.h>
#include <cassert>
#include <iostream>
#include <limits>

int main() {
    using namespace phosphor;
    // Render every state at the extremes that modifiers and audio can produce.
    for (auto expression : {Expression::Neutral, Expression::Happy, Expression::Sad,
                            Expression::Angry, Expression::Curious, Expression::Sleepy}) {
    for (auto state : {State::Ready, State::Listening, State::Speaking, State::Connecting, State::Notice}) {
        for (int level : {0, 50, 100}) {
            Face f{level, 100 - level, -100, 100, level, state};
            f.expression = expression;
            int count = 0;
            draw_face(f, [&](int x, int y, int w, int h, uint32_t) {
                assert(x >= 0 && y >= 0 && w > 0 && h > 0);
                assert(x + w <= 256 && y + h <= 144);
                ++count;
            });
            assert(count < 64);
        }
    }
    }
    int16_t silence[128]{};
    int16_t extremes[]{-32768, 32767, -32768, 32767};
    assert(pcm_level(nullptr, 100) == 0);
    assert(pcm_level(silence, 128) == 0);
    assert(pcm_level(extremes, 4) == 100);
    int envelope = 0;
    for (int i = 0; i < 10; ++i) {
        const int next = envelope_step(envelope, 100);
        assert(next >= envelope && next <= 100);
        envelope = next;
    }
    for (int i = 0; i < 20; ++i) envelope = envelope_step(envelope, 0);
    assert(envelope == 0);
    publish_audio(extremes, 4, std::numeric_limits<uint32_t>::max() - 20);
    assert(read_audio(10) == 100); // Millisecond clock wraps after 49 days.
    assert(read_audio(300) == 0);  // Stopped/failed audio never leaves the mouth open.
    std::cout << "PHOSPHOR geometry, envelope and stale audio checks passed\n";
}
