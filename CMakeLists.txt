# Файл: RISCV_VerilogSim/CMakeLists.txt
cmake_minimum_required(VERSION 3.16) # Можно вернуть к 3.16, если FetchContent не нужен
project(RISCV_VerilogSim LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Найти системный Verilator ---
find_program(PROJECT_VERILATOR_EXECUTABLE
    NAMES verilator verilator_bin
    DOC "Verilator executable"
    REQUIRED # Если не найден, CMake выдаст ошибку
)
message(STATUS "Found system Verilator executable: ${PROJECT_VERILATOR_EXECUTABLE}")

# Переменная окружения VERILATOR_ROOT для системного Verilator'а обычно не нужна,
# так как он сам знает свои пути. Но если вдруг понадобится, можно оставить возможность ее установить.
# Обычно системный Verilator настроен так, что Vxxx.mk находит verilated.mk без VERILATOR_ROOT.
# set(PROJECT_ENV_VERILATOR_ROOT "") # Оставляем пустым, если не требуется

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(tests)