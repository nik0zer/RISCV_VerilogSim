cmake_minimum_required(VERSION 3.10)

set(PIPELINE_TEST_BENCH_CPP ${CMAKE_CURRENT_SOURCE_DIR}/pipeline_tb.cpp)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
set(ELF_TO_MEMH_SCRIPT ${CMAKE_SOURCE_DIR}/scripts/elf_to_memh.py)
if(NOT EXISTS ${ELF_TO_MEMH_SCRIPT})
    message(FATAL_ERROR "Script elf_to_memh.py not found at ${ELF_TO_MEMH_SCRIPT}")
endif()

find_program(RISCV_AS NAMES riscv64-unknown-elf-as DOC "RISC-V Assembler")
find_program(RISCV_LD NAMES riscv64-unknown-elf-ld DOC "RISC-V Linker")
find_program(RISCV_OBJCOPY NAMES riscv64-unknown-elf-objcopy DOC "RISC-V Objcopy")
find_program(RISCV_READELF NAMES riscv64-unknown-elf-readelf DOC "RISC-V Readelf")

if(NOT RISCV_AS OR NOT RISCV_LD OR NOT RISCV_OBJCOPY OR NOT RISCV_READELF)
    message(FATAL_ERROR "One or more RISC-V toolchain utilities not found.")
endif()

function(add_pipeline_test test_case_name asm_file_rel_path expected_wd3_file_rel_path num_cycles pc_start_hex_no_prefix)
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

    set(ASM_INPUT_FILE_FULL_PATH "${TEST_CASE_INPUT_PATH}/${asm_file_rel_path}")
    set(ASM_OBJECT_FILE_IN_OBJDIR "${OBJ_DIR}/${test_case_name}.o")
    set(LINKED_ELF_FILE_IN_OBJDIR "${OBJ_DIR}/${test_case_name}.elf")
    set(VERILOG_HEX_MEM_FILENAME_FOR_PARAM "${test_case_name}_instr_mem.hex")
    set(GENERATED_HEX_MEM_FILE_FULL_PATH_IN_OBJDIR "${OBJ_DIR}/${VERILOG_HEX_MEM_FILENAME_FOR_PARAM}")

    set(VERILOG_PARAM_PC_START_ADDR "64'h${pc_start_hex_no_prefix}")
    set(EXPECTED_WD3_FILE_FULL_PATH "${TEST_CASE_INPUT_PATH}/${expected_wd3_file_rel_path}")

    set(BUILD_TARGET_NAME ${test_case_name}_build_verilated_pipeline)
    add_custom_target(${BUILD_TARGET_NAME} ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory ${OBJ_DIR}
        COMMAND ${RISCV_AS} -march=rv64i -mabi=lp64 -o ${ASM_OBJECT_FILE_IN_OBJDIR} ${ASM_INPUT_FILE_FULL_PATH}
        COMMAND ${RISCV_LD} --no-relax -Ttext=0x${pc_start_hex_no_prefix} -o ${LINKED_ELF_FILE_IN_OBJDIR} ${ASM_OBJECT_FILE_IN_OBJDIR}
        COMMAND ${Python3_EXECUTABLE} "${ELF_TO_MEMH_SCRIPT}"
                "${LINKED_ELF_FILE_IN_OBJDIR}"
                "${GENERATED_HEX_MEM_FILE_FULL_PATH_IN_OBJDIR}"
                --objcopy "${RISCV_OBJCOPY}" --readelf "${RISCV_READELF}" --section ".text" --wordsize 4

        COMMAND ${PROJECT_VERILATOR_EXECUTABLE}
                -Wall --Wno-fatal --cc --exe --build --trace
                --top-module ${VERILOG_MODULE_NAME}
                -I${RTL_INCLUDE_PATH}
                "-GINSTR_MEM_INIT_FILE=\"${VERILOG_HEX_MEM_FILENAME_FOR_PARAM}\""
                "-GPC_START_ADDR=${VERILOG_PARAM_PC_START_ADDR}"
                ${PIPELINE_RTL_FILES}
                "${PIPELINE_TEST_BENCH_CPP}"
                --Mdir "${OBJ_DIR}"
                -CFLAGS "-std=c++17 -Wall \
                    -DPIPELINE_TEST_CASE_NAME_STR_RAW=${test_case_name} \
                    -DEXPECTED_WD3_FILE_PATH_STR_RAW=${EXPECTED_WD3_FILE_FULL_PATH} \
                    -DNUM_CYCLES_TO_RUN=${num_cycles}"
        DEPENDS "${PIPELINE_TEST_BENCH_CPP}" "${ASM_INPUT_FILE_FULL_PATH}"
                "${EXPECTED_WD3_FILE_FULL_PATH}" "${ELF_TO_MEMH_SCRIPT}"
                ${PIPELINE_RTL_FILES}
        COMMENT "Building pipeline for test case: ${test_case_name}"
        VERBATIM
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
    if(TARGET run_all_tests AND TARGET run_all_pipeline_tests)
         add_dependencies(run_all_tests run_all_pipeline_tests)
    endif()

    message(STATUS "Configured pipeline test case: ${test_case_name}")
    message(STATUS "  ASM file: ${ASM_INPUT_FILE_FULL_PATH}")
    message(STATUS "  Expected WD3 file: ${EXPECTED_WD3_FILE_FULL_PATH}")
endfunction()

add_pipeline_test(addi_basic_asm "addi_basic_instr.s" "addi_basic_expected.txt" 17 "10000")
add_pipeline_test(jump_basic_asm "jump.s" "jump_expected.txt" 21 "10000")
add_pipeline_test(beq_basic_asm "beq.s" "beq_expected.txt" 21 "10000")