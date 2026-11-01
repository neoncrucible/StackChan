#include <phosphor/reminder_engine.h>
#include <phosphor/gestures.h>
#include <cassert>
#include <iostream>
using namespace phosphor;
int main() {
    constexpr int64_t now = clock_epoch + 86400;
    Reminders r;
    assert(r.add(0, 60, "clock invalid", false) == -2);
    assert(r.add(now, 0, "bad", false) == -1);
    assert(r.add(now, 60, "bad", false, now - 1) == -1);
    const int one = r.add(now, 60, "one shot", false);
    assert(r.add(now, 30, "too frequent", true) == -1);
    const int repeat = r.add(now, 60, "repeat", true);
    std::array<Reminder, 8> events;
    assert(r.take_due(now + 29, events) == 0);
    auto restored = r; // Same epoch deadlines survive a device reboot.
    assert(restored.take_due(now + 301, events) == 2);
    assert(restored.take_due(now + 302, events) == 0); // no catch-up storm
    assert(restored.items[1].due == now + 360);
    assert(!restored.remove(one));
    assert(restored.remove(repeat));
    for (int i = 0; i < 8; ++i) assert(restored.add(now, 10, "bounded", false) > 0);
    assert(restored.add(now, 10, "full", false) == -3);
    auto longtext = std::string(119, 'a') + "\xe2\x82\xac";
    assert(short_text(longtext).size() == 119);
    Gesture g; Pose p;
    assert(g.start("nod", UINT32_MAX - 100, 0, 20));
    assert(g.next(UINT32_MAX - 100, p) && p.pitch == 26);
    assert(!g.next(100, p));
    assert(g.next(349, p) && p.pitch == 17); // unsigned clock wrap
    g.cancel(); assert(!g.next(900, p));
    assert(!g.start("unknown", 0, 0, 0));
    std::cout << "Persistent deadlines, bounded capacity, UTF-8 and cancellable gestures passed\n";
}
