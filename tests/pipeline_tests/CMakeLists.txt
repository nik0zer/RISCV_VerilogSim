# Файл: RISCV_VerilogSim/tests/pipeline_tests/CMakeLists.txt
cmake_minimum_required(VERSION 3.10)

set(PIPELINE_TEST_BENCH_CPP ${CMAKE_CURRENT_SOURCE_DIR}/pipeline_tb.cpp)

function(add_pipeline_test test_case_name instruction_hex_file expected_wd3_file num_cycles)
    set(VERILOG_MODULE_NAME "pipeline")

    set(PIPELINE_RTL_FILES
        ${CMAKE_SOURCE_DIR}/rtl/modules/pipeline.sv
        ${CMAKE_SOURCE_DIR}/rtl/modules/control_unit.sv
        ${CMAKE_SOURCE_DIR}/rtl/modules/main_decoder.sv
        ${CMAKE_SOURCE_DIR}/rtl/modules/alu_decoder.sv
        ${CMAKE_SOURCE_DIR}/rtl/modules/flopr.sv
        ${CMAKE_SOURCE_DIR}/rtl/modules/flopenr.sv
        ${CMAKE_SOURCE_DIR}/rtl/modules/ram.sv
        ${CMAKE_SOURCE_DIR}/rtl/modules/regfile.sv
        ${CMAKE_SOURCE_DIR}/rtl/modules/imm.sv
        ${CMAKE_SOURCE_DIR}/rtl/modules/alu.sv
        ${CMAKE_SOURCE_DIR}/rtl/modules/mux2.sv
        ${CMAKE_SOURCE_DIR}/rtl/modules/mux3.sv
    )

    set(RTL_INCLUDE_PATH ${CMAKE_SOURCE_DIR}/rtl)
    set(TEST_CASE_INPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR})

    set(OBJ_DIR ${CMAKE_CURRENT_BINARY_DIR}/obj_dir_pipeline_${test_case_name})
    set(VERILATOR_GENERATED_EXE ${OBJ_DIR}/V${VERILOG_MODULE_NAME})

    set(LOCAL_INSTRUCTION_HEX_FILE_NAME "instr_mem_for_${test_case_name}.hex")
    set(COPIED_INSTRUCTION_FILE_PATH "${OBJ_DIR}/${LOCAL_INSTRUCTION_HEX_FILE_NAME}")

    # Готовим строку параметра для Verilog (строка в кавычках)
    set(VERILOG_STR_INSTR_MEM_INIT_FILE "\"${LOCAL_INSTRUCTION_HEX_FILE_NAME}\"")
    # Для памяти данных, если понадобится:
    # set(LOCAL_DATA_HEX_FILE_NAME "data_mem_for_${test_case_name}.hex")
    # set(COPIED_DATA_FILE_PATH "${OBJ_DIR}/${LOCAL_DATA_HEX_FILE_NAME}")
    # set(VERILOG_STR_DATA_MEM_INIT_FILE "\"${LOCAL_DATA_HEX_FILE_NAME}\"")

    set(BUILD_TARGET_NAME ${test_case_name}_build_verilated_pipeline)
    add_custom_target(${BUILD_TARGET_NAME} ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory ${OBJ_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy
                "${TEST_CASE_INPUT_PATH}/${instruction_hex_file}"
                "${COPIED_INSTRUCTION_FILE_PATH}"
                # Если используется инициализация памяти данных:
                # COMMAND ${CMAKE_COMMAND} -E copy
                #         "${TEST_CASE_INPUT_PATH}/имя_файла_данных.hex" # Замени на реальное имя, если нужно
                #         "${COPIED_DATA_FILE_PATH}"
                COMMAND ${PROJECT_VERILATOR_EXECUTABLE}
                -Wall --Wno-fatal --cc --exe --build --trace
                --top-module ${VERILOG_MODULE_NAME}
                -I${RTL_INCLUDE_PATH}
                -GINSTR_MEM_INIT_FILE=${VERILOG_STR_INSTR_MEM_INIT_FILE}
                # -GDATA_MEM_INIT_FILE=${VERILOG_STR_DATA_MEM_INIT_FILE} # Если используется
                ${PIPELINE_RTL_FILES}
                "${PIPELINE_TEST_BENCH_CPP}"
                --Mdir "${OBJ_DIR}"
                -CFLAGS "-std=c++17 -Wall \
                    -DPIPELINE_TEST_CASE_NAME_STR_RAW=${test_case_name} \
                    -DEXPECTED_WD3_FILE_PATH_STR_RAW=${TEST_CASE_INPUT_PATH}/${expected_wd3_file} \
                    -DNUM_CYCLES_TO_RUN=${num_cycles}"
        DEPENDS ${PIPELINE_RTL_FILES} "${PIPELINE_TEST_BENCH_CPP}"
                "${TEST_CASE_INPUT_PATH}/${instruction_hex_file}"
                "${TEST_CASE_INPUT_PATH}/${expected_wd3_file}"
        COMMENT "Verilating and Building pipeline for test case: ${test_case_name}"
        VERBATIM
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

    set(RUN_TARGET_NAME run_${test_case_name}_pipeline_test)
    add_custom_target(${RUN_TARGET_NAME}
        COMMAND "${VERILATOR_GENERATED_EXE}"
        DEPENDS ${BUILD_TARGET_NAME}
        WORKING_DIRECTORY ${OBJ_DIR}
        COMMENT "Running pipeline test case: ${test_case_name}"
        VERBATIM
    )

    if(TARGET run_all_pipeline_tests)
        add_dependencies(run_all_pipeline_tests ${RUN_TARGET_NAME})
    endif()
    if(TARGET run_all_tests)
         add_dependencies(run_all_tests ${RUN_TARGET_NAME})
    endif()

    message(STATUS "Configured pipeline test case: ${test_case_name}")
endfunction()

add_pipeline_test(addi_basic "addi_basic_instr.hex" "addi_basic_expected.txt" 12)
# add_pipeline_test(load_after_addi "load_addi_instr.hex" "load_addi_expected_wd3.txt" 15)