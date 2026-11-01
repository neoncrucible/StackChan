/* SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 * SPDX-License-Identifier: MIT
 * PHOSPHOR terminal launcher, replacing M5Stack's carousel.
 */
#pragma once
#include <mooncake.h>
#include <lvgl.h>
#include <functional>
#include <vector>
namespace view {
class LauncherView {
public:
    ~LauncherView();
    std::function<void(int)> onAppClicked;
    void init(std::vector<mooncake::AppProps_t> apps);
    void update();
private:
    lv_obj_t* panel_ = nullptr;
    int clicked_ = -1;
};
}
