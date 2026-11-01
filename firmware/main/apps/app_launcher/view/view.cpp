/* SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 * SPDX-License-Identifier: MIT */
#include "view.h"
#include <phosphor/ui.h>
#include <hal/hal.h>
namespace view {
LauncherView::~LauncherView() { if (panel_) lv_obj_delete(panel_); }
void LauncherView::init(std::vector<mooncake::AppProps_t> apps) {
    panel_ = phosphor::panel(lv_screen_active(), 0, 0, 320, 240);
    phosphor::text(panel_, "P H O S P H O R", 26, 28, 288, &lv_font_montserrat_24);
    phosphor::text(panel_, "PERSONAL DESK UNIT // 02", 28, 67, 280, &lv_font_unscii_8);
    phosphor::text(panel_, "Ready when you are.", 28, 90, 270);
    int y = 128;
    for (const auto& app : apps) {
        const bool voice = app.info.name == "AI.AGENT";
        if (!voice && app.info.name != "SETUP") continue;
        auto* b = phosphor::button(panel_, voice ? "> START CONVERSATION" : "> SYSTEM SETTINGS", 24, y, 272, 42,
            [](lv_event_t* e) {
                auto* self = static_cast<LauncherView*>(lv_event_get_user_data(e));
                self->clicked_ = static_cast<int>(reinterpret_cast<intptr_t>(lv_obj_get_user_data(lv_event_get_target_obj(e))));
            }, this);
        lv_obj_set_user_data(b, reinterpret_cast<void*>(static_cast<intptr_t>(app.appID)));
        y += 51;
    }
    GetHAL().clearWarmRebootRequest();
    GetHAL().bootLogo.reset();
}
void LauncherView::update() {
    if (clicked_ >= 0) { const int id = clicked_; clicked_ = -1; if (onAppClicked) onAppClicked(id); }
}
}
