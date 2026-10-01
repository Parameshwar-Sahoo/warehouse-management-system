# Unit & Integration Tests CMake configuration

add_executable(wms_tests
    test_inventory.cpp
    test_device_driver.cpp
    test_ipc_signals.cpp
)

target_link_libraries(wms_tests
    wms_core
    pthread
    rt
)

add_test(NAME AllWarehouseTests COMMAND wms_tests)
