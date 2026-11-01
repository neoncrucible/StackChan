/* SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 * SPDX-License-Identifier: MIT
 * PHOSPHOR persistent replacement for M5Stack's in-memory reminder manager.
 */
#include "reminder.h"
#include <phosphor/reminder_engine.h>
#include <nvs.h>
#include <cJSON.h>
#include <esp_log.h>
#include <ctime>
#include <esp_timer.h>
#include <mutex>
#include <atomic>
#include <algorithm>
#include <cmath>

namespace {
std::mutex mutex;
phosphor::Reminders reminders;
bool loaded = false;
std::atomic<bool> scheduled{false};
std::atomic<uint32_t> revision{0};
std::atomic<bool> alert_grace{false};
std::atomic<uint32_t> last_alert_ms{0};
uitk::Signal<int, std::string_view> triggered;

bool save() {
    auto* root = cJSON_CreateObject();
    if (!root) return false;
    bool ok = cJSON_AddNumberToObject(root, "v", 1) &&
              cJSON_AddNumberToObject(root, "next", reminders.next_id);
    auto* entries = cJSON_AddArrayToObject(root, "entries");
    ok = ok && entries;
    for (const auto& r : reminders.items) if (r.id) {
        auto* entry = cJSON_CreateObject();
        if (!entry) { ok = false; break; }
        ok = ok && cJSON_AddNumberToObject(entry, "id", r.id) &&
             cJSON_AddNumberToObject(entry, "due", static_cast<double>(r.due)) &&
             cJSON_AddNumberToObject(entry, "repeat", r.interval) &&
             cJSON_AddStringToObject(entry, "text", r.message.c_str());
        if (!ok || !cJSON_AddItemToArray(entries, entry)) { cJSON_Delete(entry); ok = false; break; }
    }
    if (!ok) { cJSON_Delete(root); return false; }
    char* json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!json) return false;
    nvs_handle_t handle;
    auto err = nvs_open("ph_reminders", NVS_READWRITE, &handle);
    if (err == ESP_OK) {
        err = nvs_set_str(handle, "schedule", json);
        if (err == ESP_OK) err = nvs_commit(handle);
        nvs_close(handle);
    }
    cJSON_free(json);
    if (err != ESP_OK) ESP_LOGE("Reminders", "Save failed: %s", esp_err_to_name(err));
    if (err == ESP_OK) {
        scheduled.store(std::any_of(reminders.items.begin(), reminders.items.end(),
            [](const auto& r) { return r.id != 0; }));
        ++revision;
    }
    return err == ESP_OK;
}

void load() {
    if (loaded) return;
    loaded = true;
    nvs_handle_t handle;
    if (nvs_open("ph_reminders", NVS_READONLY, &handle) != ESP_OK) return;
    size_t size = 0;
    auto err = nvs_get_str(handle, "schedule", nullptr, &size);
    if (err != ESP_OK || size < 2 || size > 8192) { nvs_close(handle); return; }
    std::string json(size, '\0');
    err = nvs_get_str(handle, "schedule", json.data(), &size);
    nvs_close(handle);
    if (err != ESP_OK) return;
    auto* root = cJSON_Parse(json.c_str());
    if (!root) return;
    auto number = [](const cJSON* obj, const char* key, double low, double high) {
        auto* p = cJSON_GetObjectItem(obj, key);
        return cJSON_IsNumber(p) && std::isfinite(p->valuedouble) && p->valuedouble >= low &&
               p->valuedouble <= high && std::floor(p->valuedouble) == p->valuedouble;
    };
    auto* entries = cJSON_GetObjectItem(root, "entries");
    bool valid = number(root, "v", 1, 1) && number(root, "next", 1, INT32_MAX) &&
                 cJSON_IsArray(entries) && cJSON_GetArraySize(entries) <= 8;
    phosphor::Reminders restored;
    if (valid) restored.next_id = cJSON_GetObjectItem(root, "next")->valueint;
    for (int i = 0; valid && i < cJSON_GetArraySize(entries); ++i) {
        auto* e = cJSON_GetArrayItem(entries, i);
        auto* text = cJSON_GetObjectItem(e, "text");
        valid = number(e, "id", 1, INT32_MAX) && number(e, "due", phosphor::clock_epoch, 4102444800LL) &&
                number(e, "repeat", 0, 86400) && cJSON_IsString(text) && text->valuestring[0] &&
                std::char_traits<char>::length(text->valuestring) <= 120;
        if (!valid) break;
        auto& r = restored.items[i];
        r = {cJSON_GetObjectItem(e, "id")->valueint,
             static_cast<int64_t>(cJSON_GetObjectItem(e, "due")->valuedouble),
             static_cast<uint32_t>(cJSON_GetObjectItem(e, "repeat")->valueint), text->valuestring};
        for (int j = 0; j < i; ++j) if (restored.items[j].id == r.id) valid = false;
    }
    if (valid) {
        reminders = std::move(restored);
        ++revision;
        scheduled.store(std::any_of(reminders.items.begin(), reminders.items.end(),
            [](const auto& r) { return r.id != 0; }));
    }
    else ESP_LOGE("Reminders", "Stored schedule invalid; not loading partial data");
    cJSON_Delete(root);
}
}

namespace tools {
void init_reminders() { std::lock_guard<std::mutex> lock(mutex); load(); }
uint32_t reminder_revision() { return revision.load(); }
uitk::Signal<int, std::string_view>& on_reminder_triggered() { return triggered; }
int create_reminder(uint32_t durationMs, std::string_view message, bool repeat) {
    return create_reminder_at(0, durationMs, message, repeat);
}
int create_reminder_at(int64_t at, uint32_t durationMs, std::string_view message, bool repeat) {
    std::lock_guard<std::mutex> lock(mutex); load();
    auto before = reminders;
    int id = reminders.add(time(nullptr), durationMs / 1000, message, repeat, at);
    if (id > 0 && !save()) { reminders = std::move(before); return -4; }
    return id;
}
bool stop_reminder(int id) {
    std::lock_guard<std::mutex> lock(mutex); load();
    auto before = reminders;
    if (!reminders.remove(id)) return false;
    if (!save()) { reminders = std::move(before); return false; }
    return true;
}
void update_reminders() {
    std::array<phosphor::Reminder, 8> events;
    size_t count;
    {
        std::lock_guard<std::mutex> lock(mutex); load();
        static int64_t last_check = -1000000;
        const int64_t tick = esp_timer_get_time();
        if (tick - last_check < 1000000) return;
        last_check = tick;
        if (alert_grace.load() && uint32_t(tick / 1000 - last_alert_ms.load()) >= 60000)
            alert_grace.store(false);
        const auto now = time(nullptr);
        if (now < phosphor::clock_epoch || std::none_of(reminders.items.begin(), reminders.items.end(),
            [&](const auto& r) { return r.id && now >= r.due; })) return;
        auto before = reminders;
        count = reminders.take_due(now, events);
        if (count) {
            last_alert_ms.store(static_cast<uint32_t>(tick / 1000));
            alert_grace.store(true);
        }
        if (count && !save()) { reminders = std::move(before); return; }
    }
    // Never call UI/audio callbacks with the storage mutex held.
    for (size_t i = 0; i < count; ++i) triggered.emit(events[i].id, events[i].message);
}
std::vector<ReminderInfo_t> get_active_reminders() {
    std::lock_guard<std::mutex> lock(mutex); load();
    std::vector<ReminderInfo_t> result;
    auto now = time(nullptr);
    for (const auto& r : reminders.items) if (r.id) {
        ReminderInfo_t info;
        info.id = r.id; info.durationMs = r.interval * 1000;
        info.message = r.message; info.repeat = r.interval != 0; info.dueUnix = r.due;
        info.remainingSeconds = static_cast<uint32_t>(std::clamp<int64_t>(r.due - now, 0, UINT32_MAX));
        result.push_back(std::move(info));
    }
    std::sort(result.begin(), result.end(), [](auto& a, auto& b) { return a.dueUnix < b.dueUnix; });
    return result;
}
bool reminders_need_power() {
    // No allocations, NVS access or mutex waits in the power timer callback.
    return scheduled.load() || (alert_grace.load() &&
        uint32_t(esp_timer_get_time() / 1000 - last_alert_ms.load()) < 60000);
}
const char* reminder_error(int code) {
    switch(code) {
        case -2: return "Clock not set. Connect Wi-Fi once to set the time.";
        case -3: return "Eight reminders already saved. Cancel one first.";
        case -4: return "Could not save reminder. Storage is unavailable or full.";
        default: return "Invalid reminder time or message.";
    }
}
}
