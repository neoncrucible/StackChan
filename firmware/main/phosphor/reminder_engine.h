// SPDX-License-Identifier: MIT
#pragma once
#include <array>
#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>

namespace phosphor {
constexpr int64_t clock_epoch = 1704067200; // 2024-01-01; reject unset clocks.
struct Reminder {
    int id = 0;
    int64_t due = 0;
    uint32_t interval = 0;
    std::string message;
};
inline std::string short_text(std::string_view text) {
    size_t n = std::min<size_t>(text.size(), 120);
    while (n < text.size() && n && (static_cast<unsigned char>(text[n]) & 0xc0) == 0x80) --n;
    return std::string(text.substr(0, n));
}
class Reminders {
public:
    std::array<Reminder, 8> items{};
    int next_id = 1;
    int add(int64_t now, uint32_t seconds, std::string_view message, bool repeat, int64_t at = 0) {
        if (now < clock_epoch) return -2;
        if (seconds < 1 || seconds > 86400 || message.empty()) return -1;
        if (repeat && seconds < 60) return -1; // Avoid rapid alarms and flash writes.
        if (at && (at <= now || at - now > 366LL * 86400)) return -1;
        for (auto& r : items) if (!r.id) {
            // IDs remain distinct even when the counter eventually wraps.
            while (std::any_of(items.begin(), items.end(), [&](auto& e) { return e.id == next_id; }))
                advance_id();
            int id = next_id; advance_id();
            r = {id, at ? at : now + seconds, repeat ? seconds : 0, short_text(message)};
            return id;
        }
        return -3;
    }
    bool remove(int id) {
        for (auto& r : items) if (r.id == id && id > 0) { r = {}; return true; }
        return false;
    }
    size_t take_due(int64_t now, std::array<Reminder, 8>& events) {
        if (now < clock_epoch) return 0;
        size_t count = 0;
        for (auto& r : items) if (r.id && now >= r.due) {
            events[count++] = r;
            if (r.interval) r.due += ((now - r.due) / r.interval + 1) * r.interval;
            else r = {};
        }
        return count;
    }
private:
    void advance_id() { next_id = next_id == INT32_MAX ? 1 : next_id + 1; }
};
} // namespace phosphor
