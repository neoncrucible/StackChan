#pragma once
#include <cstddef>
#include <freertos/FreeRTOS.h>
using uart_port_t = int;
using esp_err_t = int;
enum { UART_NUM_1 = 1, UART_NUM_MAX = 3, ESP_OK = 0,
       UART_DATA_8_BITS = 8, UART_PARITY_DISABLE = 0, UART_STOP_BITS_1 = 1,
       UART_HW_FLOWCTRL_DISABLE = 0, UART_SCLK_DEFAULT = 0, UART_PIN_NO_CHANGE = -1 };
struct uart_config_t { int baud_rate, data_bits, parity, stop_bits, flow_ctrl, rx_flow_ctrl_thresh, source_clk; };
inline const char* esp_err_to_name(int) { return "fake"; }
inline int uart_driver_install(int,int,int,int,void*,int) { return 0; }
inline int uart_driver_delete(int) { return 0; }
inline int uart_param_config(int,const uart_config_t*) { return 0; }
inline int uart_set_pin(int,int,int,int,int) { return 0; }
inline int uart_write_bytes(int,const char*,int n) { return n; }
inline int uart_flush_input(int) { return 0; }
inline int uart_wait_tx_done(int,TickType_t) { return 0; }
int uart_read_bytes(int,void*,int,TickType_t);
