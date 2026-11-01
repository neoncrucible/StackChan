/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include "hal.h"
#include <mooncake_log.h>
#include <mcp_server.h>
#include <stackchan/stackchan.h>
#include <apps/common/common.h>
#include <phosphor/retro_avatar.h>
#include <phosphor/personality.h>
#include <ctime>

using namespace stackchan;

static const std::string_view _tag = "HAL-MCP";

void Hal::xiaozhi_mcp_init()
{
    mclog::tagInfo(_tag, "init");

    // https://github.com/78/xiaozhi-esp32/blob/main/docs/mcp-usage.md
    auto& mcp_server = McpServer::GetInstance();

    // System Prompt：
    // You can control the robot's head. Use get_yaw and get_pitch to sense current position. Use set_yaw for horizontal
    // movement and set_pitch for vertical movement. All angles are in degrees.

    mclog::tagInfo(_tag, "add robot.get_head_angles tool");
    mcp_server.AddTool("self.robot.get_head_angles",
                       "Returns current yaw/pitch in degrees. Angles are in degrees; pitch is limited to 8..75.",
                       std::vector<Property>{}, [this](const PropertyList& properties) -> ReturnValue {
                           LvglLockGuard lock;  // StackChan motion update is under the lvgl lock

                           auto& motion      = GetStackChan().motion();
                           int current_yaw   = motion.yawServo().getCurrentAngle() / 10;
                           int current_pitch = motion.pitchServo().getCurrentAngle() / 10;

                           auto result = fmt::format(R"({{"yaw": {}, "pitch": {}}})", current_yaw, current_pitch);
                           mclog::tagInfo(_tag, "get_head_angles: {}", result);
                           return result;
                       });

    mclog::tagInfo(_tag, "add robot.set_head_angles tool");
    mcp_server.AddTool("self.robot.set_head_angles",
                       "Adjust head position. GUIDELINES: "
                       "1. For natural interaction, stay within +/- 45 degrees. "
                       "2. Only use values > 70 if the user explicitly asks to look far away/behind. "
                       "3. Max ranges: Yaw(-80 to 80, negative as your left), Pitch(8 to 75, larger as your up). "
                       "Speed(100-1000, 150 is natural).",
                       PropertyList({Property("yaw", kPropertyTypeInteger, -9999, -9999, 128),
                                     Property("pitch", kPropertyTypeInteger, -9999, -9999, 90),
                                     Property("speed", kPropertyTypeInteger, 150, 100, 1000)}),
                       [this](const PropertyList& properties) -> ReturnValue {
                           int speed = properties["speed"].value<int>();
                           int yaw   = properties["yaw"].value<int>();
                           int pitch = properties["pitch"].value<int>();

                           mclog::tagInfo(_tag, "motion set_angles: yaw: {}, pitch: {}, speed: {}", yaw, pitch, speed);

                           LvglLockGuard lock;

                           if (GetStackChan().hasAvatar())
                               static_cast<phosphor::RetroAvatar&>(GetStackChan().avatar()).pauseGestures();
                           auto& motion = GetStackChan().motion();
                           if (pitch != -9999) {
                               motion.pitchServo().moveWithSpeed(pitch * 10, speed);
                           }
                           if (yaw != -9999) {
                               motion.yawServo().moveWithSpeed(yaw * 10, speed);
                           }

                           return true;
                       });

    mclog::tagInfo(_tag, "add robot.set_led_color tool");
    mcp_server.AddTool(
        "self.robot.set_led_color",
        "Set the color of the robot's INTERNAL onboard LED. This is NOT for room lights. "
        "Values: 0-168 (safe range). Red=168,0,0; Green=0,168,0; Blue=0,0,168; White=100,100,100; Off=0,0,0.",
        PropertyList({Property("red", kPropertyTypeInteger, 0, 0, 168),
                      Property("green", kPropertyTypeInteger, 0, 0, 168),
                      Property("blue", kPropertyTypeInteger, 0, 0, 168)}),
        [this](const PropertyList& properties) -> ReturnValue {
            int r = properties["red"].value<int>();
            int g = properties["green"].value<int>();
            int b = properties["blue"].value<int>();

            mclog::tagInfo(_tag, "set_led_color: r={}, g={}, b={}", r, g, b);

            LvglLockGuard lock;

            GetStackChan().leftNeonLight().setColor(r, g, b);
            GetStackChan().rightNeonLight().setColor(r, g, b);

            return true;
        });

    mclog::tagInfo(_tag, "add robot.create_reminder tool");
    mcp_server.AddTool("self.robot.create_reminder",
                       "Save a reminder on this robot (maximum eight). Shows text and plays a chime; does not speak the message. "
                       "Use duration_seconds for a countdown, or at_unix for an absolute Unix UTC time. "
                       "For local dates consult get_reminders for device time and timezone first. "
                       "repeat repeats every duration_seconds (minimum 60). Report errors; never claim an unsaved reminder is set.",
                       PropertyList({Property("duration_seconds", kPropertyTypeInteger, 60, 1, 86400),
                                     Property("message", kPropertyTypeString, std::string("Time's up!")),
                                     Property("repeat", kPropertyTypeBoolean, false),
                                     Property("at_unix", kPropertyTypeInteger, 0, 0, INT32_MAX)}),
                       [this](const PropertyList& properties) -> ReturnValue {
                           int duration_seconds = properties["duration_seconds"].value<int>();
                           std::string message  = properties["message"].value<std::string>();
                           bool repeat          = properties["repeat"].value<bool>();

                           // Default message
                           if (message.empty()) {
                               message = "Time's up!";
                           }

                           mclog::tagInfo(_tag, "create_reminder: duration={}s, message={}, repeat={}",
                                          duration_seconds, message, repeat);

                           int id = tools::create_reminder_at(properties["at_unix"].value<int>(), duration_seconds * 1000, message, repeat);
                           if (id < 0) return std::string(tools::reminder_error(id));

                           return id;
                       });

    mclog::tagInfo(_tag, "add robot.get_reminders tool");
    mcp_server.AddTool("self.robot.get_reminders", "Get list of active reminders.", std::vector<Property>{},
                       [this](const PropertyList& properties) -> ReturnValue {
                           mclog::tagInfo(_tag, "get_reminders");
                           auto reminders          = tools::get_active_reminders();
                           auto* result = cJSON_CreateObject();
                           const time_t now = time(nullptr);
                           cJSON_AddNumberToObject(result, "now_unix", now);
                           struct tm local{}; localtime_r(&now, &local);
                           char stamp[64]; strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S %z", &local);
                           cJSON_AddStringToObject(result, "local_time", stamp);
                           cJSON_AddStringToObject(result, "timezone", GetHAL().getTimezone().c_str());
                           auto* array = cJSON_AddArrayToObject(result, "reminders");
                           for (const auto& r : reminders) {
                               auto* entry = cJSON_CreateObject();
                               cJSON_AddNumberToObject(entry, "id", r.id);
                               cJSON_AddNumberToObject(entry, "due_unix", static_cast<double>(r.dueUnix));
                               cJSON_AddNumberToObject(entry, "remaining_seconds", r.remainingSeconds);
                               cJSON_AddNumberToObject(entry, "repeat_seconds", r.durationMs / 1000);
                               cJSON_AddStringToObject(entry, "message", r.message.c_str());
                               cJSON_AddItemToArray(array, entry);
                           }
                           return result;
                       });

    mclog::tagInfo(_tag, "add robot.stop_reminder tool");
    mcp_server.AddTool("self.robot.stop_reminder", "Stop a reminder by ID.",
                       PropertyList({Property("id", kPropertyTypeInteger, -1)}),
                       [this](const PropertyList& properties) -> ReturnValue {
                           int id = properties["id"].value<int>();
                           mclog::tagInfo(_tag, "stop_reminder: id={}", id);
                           return tools::stop_reminder(id);
                       });
    mcp_server.AddTool("self.robot.get_character",
        "Read PHOSPHOR's character and capability guidance. Use when establishing the robot persona.",
        std::vector<Property>{}, [](const PropertyList&) -> ReturnValue { return std::string(phosphor::personality); });
    mcp_server.AddTool("self.robot.gesture", "Perform a small nod, shake or curious tilt when idle. "
        "Returns false if busy or paused. Do not repeat requests during speech.",
        PropertyList({Property("name", kPropertyTypeString)}), [](const PropertyList& p) -> ReturnValue {
            LvglLockGuard lock;
            if (!GetStackChan().hasAvatar()) return false;
            return static_cast<phosphor::RetroAvatar&>(GetStackChan().avatar()).gesture(p["name"].value<std::string>());
        });

}
