/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <smooth_ui_toolkit.hpp>
#include <uitk/short_namespace.hpp>
#include <string_view>

namespace tools {

struct ReminderInfo_t {
    int id              = -1;
    uint32_t durationMs = 0;
    std::string message;
    bool repeat = false;
    int64_t dueUnix = 0;
    uint32_t remainingSeconds = 0;
};

uitk::Signal<int, std::string_view>& on_reminder_triggered();
void init_reminders();
uint32_t reminder_revision();
int create_reminder(uint32_t durationMs, std::string_view message, bool repeat = false);
bool stop_reminder(int id);
int create_reminder_at(int64_t at, uint32_t durationMs, std::string_view message, bool repeat = false);
const char* reminder_error(int code);
void update_reminders();
bool reminders_need_power();
std::vector<ReminderInfo_t> get_active_reminders();

}  // namespace tools
