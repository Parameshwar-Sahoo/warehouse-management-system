cmake_minimum_required(VERSION 3.20)
project(WarehouseManagementSystem VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
add_compile_options(-Wall -Wextra -Wpedantic -pthread)

# Include directories
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/driver/include
)

# Core WMS Library
add_library(wms_core STATIC
    src/core/StorageBay.cpp
    src/core/WarehouseZone.cpp
    src/core/InventoryManager.cpp
    src/core/OrderProcessor.cpp
    src/hal/LinuxCharDevice.cpp
    src/hal/SimulatedDevice.cpp
    src/ipc/SharedMemoryState.cpp
    src/ipc/SignalDispatcher.cpp
    src/ui/TerminalDashboard.cpp
)

target_link_libraries(wms_core
    pthread
    rt
)

# Main Executable
add_executable(wms_app
    src/main.cpp
)

target_link_libraries(wms_app
    wms_core
    pthread
    rt
)

# Unit & Integration Tests
enable_testing()
add_subdirectory(tests)
