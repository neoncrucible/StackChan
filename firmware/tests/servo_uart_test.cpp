// SPDX-License-Identifier: MIT
// Compile the real transport against a timed UART fake, at 100 and 1000 Hz.
#include <hal/drivers/FTServo_Arduino/src/SCSerial.h>
#include <cassert>
#include <cstdint>
#include <deque>
#include <iostream>
#include <utility>

static int64_t clock_us;
static int reads, blocking_reads;
static std::deque<std::pair<int64_t, unsigned char>> bytes;
int64_t esp_timer_get_time() { return clock_us; }
void vTaskDelay(TickType_t) { assert(false && "UART must block in its driver, not poll with delays"); }
int uart_read_bytes(int, void* data, int n, TickType_t wait) {
    assert(n == 1);
    ++reads;
    if (wait) ++blocking_reads;
    assert(reads < 100); // Silent line must never busy-spin.
    const int64_t until = clock_us + wait * (1000000 / TEST_TICK_HZ);
    if (!bytes.empty() && bytes.front().first <= until) {
        clock_us = std::max(clock_us, bytes.front().first);
        *static_cast<unsigned char*>(data) = bytes.front().second;
        bytes.pop_front();
        return 1;
    }
    clock_us = until;
    return 0;
}
struct TestBus : SCSerial { using SCSerial::readSCS; };
static void reset() { clock_us = 0; reads = blocking_reads = 0; bytes.clear(); }
int main() {
    TestBus bus;
    bus.uart_num = UART_NUM_1;
    bus.IOTimeOut = 10;
    unsigned char out[8]{};
    reset();
    assert(bus.readSCS(out, 4) == 0);
    assert(reads == 1 && blocking_reads == 1 && clock_us >= 10000);
    reset();
    bytes = {{1000, 0xff}, {1200, 0xff}, {1400, 0x01}, {1600, 0x04}};
    assert(bus.readSCS(out, 4) == 4);
    assert(out[0] == 0xff && out[2] == 1 && out[3] == 4);
    assert(clock_us == 1600); // A prompt packet needs no artificial sleep.
    reset();
    bytes = {{7000, 1}, {14000, 2}, {21000, 3}};
    assert(bus.readSCS(out, 3) == 3); // Inter-byte timeout renews.
    reset();
    bytes = {{7000, 1}, {14000, 2}, {21000, 3}};
    assert(bus.readSCS(out, 3, 10) < 3); // Whole-request timeout does not renew.
    assert(clock_us <= 10000 + 1000000 / TEST_TICK_HZ);
    reset();
    bytes = {{1000, 5}, {2000, 6}};
    assert(bus.readSCS(nullptr, 2) == 2); // Drain/discard path.
    assert(bus.readSCS(out, 0) == 0);
    bus.uart_num = UART_NUM_MAX;
    assert(bus.readSCS(out, 2) == 0);
    std::cout << "UART delivery, discard and bounded timeout checks passed at " << TEST_TICK_HZ << " Hz\n";
}
