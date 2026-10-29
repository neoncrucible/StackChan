cmake_minimum_required(VERSION 3.16)

project(stackchan_firmware_tests LANGUAGES CXX)

enable_testing()

add_executable(motion_math_test
    motion_math_test.cpp
    ../main/stackchan/motion/motion_math.cpp
)

target_include_directories(motion_math_test PRIVATE
    ../main
)

target_compile_features(motion_math_test PRIVATE cxx_std_17)

add_test(NAME motion_math_test COMMAND $<TARGET_FILE:motion_math_test>)

add_executable(phosphor_core_test phosphor_core_test.cpp)
target_include_directories(phosphor_core_test PRIVATE ../main)
target_compile_features(phosphor_core_test PRIVATE cxx_std_20)
add_test(NAME phosphor_core COMMAND phosphor_core_test)

foreach(tick_hz 100 1000)
    add_executable(servo_uart_${tick_hz}
        servo_uart_test.cpp
        ../main/hal/drivers/FTServo_Arduino/src/SCSerial.cpp
        ../main/hal/drivers/FTServo_Arduino/src/SCS.cpp)
    target_include_directories(servo_uart_${tick_hz} PRIVATE uart_stubs ../main)
    target_compile_definitions(servo_uart_${tick_hz} PRIVATE TEST_TICK_HZ=${tick_hz})
    target_compile_features(servo_uart_${tick_hz} PRIVATE cxx_std_17)
    add_test(NAME servo_uart_${tick_hz} COMMAND servo_uart_${tick_hz})
endforeach()
