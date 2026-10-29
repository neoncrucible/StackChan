#pragma once
#include <cstdint>
using TickType_t = uint32_t;
#ifndef TEST_TICK_HZ
#define TEST_TICK_HZ 100
#endif
#define pdMS_TO_TICKS(ms) (static_cast<TickType_t>((ms) * TEST_TICK_HZ / 1000))
