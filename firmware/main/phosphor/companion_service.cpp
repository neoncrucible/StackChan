// SPDX-License-Identifier: MIT
#include "companion_service.h"
#include "retro_avatar.h"
#include <apps/common/reminder/reminder.h>
#include <apps/common/reminder/reminder_view.hpp>
#include <assets/assets.h>
#include <hal/hal.h>
#include <hal/board/hal_bridge.h>
#include <stackchan/stackchan.h>
#include <array>
#include <algorithm>
#include <memory>

namespace phosphor {
namespace {
struct Alert { int id = 0; std::string message; };
std::array<Alert, 8> pending;
std::unique_ptr<view::ReminderView> visible;
int visible_id = 0;
bool chime_pending = false;
}
void init_companion() {
    tools::init_reminders();
    tools::on_reminder_triggered().connect([](int id, std::string_view message) {
        // Repeating reminders coalesce while their alert is still visible/queued.
        if (visible_id == id) return;
        for (const auto& a : pending) if (a.id == id) return;
        for (auto& a : pending) if (!a.id) { a = {id, std::string(message)}; return; }
        // Bounded even if new reminders are created before old alerts are dismissed.
        std::move(pending.begin() + 1, pending.end(), pending.begin());
        pending.back() = {id, std::string(message)};
    });
}
void update_companion() {
    // Called by the home/settings loop, then by the robot task after handover.
    tools::update_reminders();
    if (!visible && pending.front().id) hal_bridge::board_wake_display();
    {
        LvglLockGuard lock;
        if (visible) {
            visible->_update();
            if (visible->isDestroyRequested()) {
                visible.reset(); visible_id = 0; chime_pending = false;
            }
        }
        if (!visible && pending.front().id) {
            visible_id = pending.front().id;
            // The top layer survives launcher/setup screen changes.
            visible = std::make_unique<view::ReminderView>(lv_layer_top(), pending.front().message);
            std::move(pending.begin() + 1, pending.end(), pending.begin());
            pending.back() = {};
            chime_pending = true;
        }
        if (GetStackChan().hasAvatar()) {
            auto& avatar = static_cast<RetroAvatar&>(GetStackChan().avatar());
            avatar.setAlertActive(visible != nullptr);
        }
    }
    // The audio service exists only in conversation mode. Do not mix a chime
    // with speech or microphone capture, or play a dismissed alert later.
    if (chime_pending && hal_bridge::is_xiaozhi_ready() && hal_bridge::is_xiaozhi_idle()) {
        chime_pending = false;
        hal_bridge::app_play_sound(OGG_NEW_NOTIFICATION);
    }
}
}
