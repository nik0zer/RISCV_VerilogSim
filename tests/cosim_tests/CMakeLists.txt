cmake_minimum_required(VERSION 3.10)

set(COSIM_TEST_BENCH_CPP ${CMAKE_CURRENT_SOURCE_DIR}/pipeline_cosim_tb.cpp)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
set(ELF_TO_MEMH_SCRIPT ${CMAKE_SOURCE_DIR}/scripts/elf_to_memh.py)
set(FILTER_SIM_OUTPUT_SCRIPT ${CMAKE_SOURCE_DIR}/scripts/filter_sim_output.py)
set(COMPARE_TRACE_FILES_SCRIPT ${CMAKE_SOURCE_DIR}/scripts/compare_trace_files.py)

if(NOT EXISTS ${ELF_TO_MEMH_SCRIPT})
    message(FATAL_ERROR "Script elf_to_memh.py not found at ${ELF_TO_MEMH_SCRIPT}")
endif()
if(NOT EXISTS ${FILTER_SIM_OUTPUT_SCRIPT})
    message(FATAL_ERROR "Script filter_sim_output.py not found at ${FILTER_SIM_OUTPUT_SCRIPT}")
endif()

find_program(RISCV_AS NAMES riscv64-unknown-elf-as DOC "RISC-V Assembler")
find_program(RISCV_LD NAMES riscv64-unknown-elf-ld DOC "RISC-V Linker")
find_program(RISCV_OBJCOPY NAMES riscv64-unknown-elf-objcopy DOC "RISC-V Objcopy")
find_program(RISCV_READELF NAMES riscv64-unknown-elf-readelf DOC "RISC-V Readelf")

set(SIMULATOR_TARGET_NAME "Simulator")
set(SIMULATOR_EXECUTABLE ${CMAKE_BINARY_DIR}/bin/${SIMULATOR_TARGET_NAME})
set(COSIM_PLUGIN_TARGET_NAME "1")
set(COSIM_PLUGIN_SO_PATH "${CMAKE_BINARY_DIR}/plugins/${COSIM_PLUGIN_TARGET_NAME}.so")

if(NOT RISCV_AS OR NOT RISCV_LD OR NOT RISCV_OBJCOPY OR NOT RISCV_READELF)
    message(FATAL_ERROR "One or more RISC-V toolchain utilities not found.")
endif()

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
    ${CMAKE_SOURCE_DIR}/rtl/modules/hazard_unit.sv
    ${CMAKE_SOURCE_DIR}/rtl/modules/mem_2_store.sv
)
set(RTL_INCLUDE_PATH ${CMAKE_SOURCE_DIR}/rtl)

function(add_cosim_test test_case_name asm_file_rel_path num_cycles pc_start_hex_no_prefix)
    set(VERILOG_MODULE_NAME "pipeline")
    set(TEST_CASE_INPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR})
    set(OBJ_DIR ${CMAKE_CURRENT_BINARY_DIR}/obj_dir_cosim_${test_case_name})

    set(ASM_INPUT_FILE_FULL_PATH "${TEST_CASE_INPUT_PATH}/${asm_file_rel_path}")
    set(ASM_OBJECT_FILE_IN_OBJDIR "${OBJ_DIR}/${test_case_name}.o")
    set(LINKED_ELF_FILE_IN_OBJDIR "${OBJ_DIR}/${test_case_name}.elf")
    set(VERILOG_HEX_MEM_FILENAME_FOR_PARAM "${test_case_name}_instr_mem.hex")
    set(GENERATED_HEX_MEM_FILE_FULL_PATH_IN_OBJDIR "${OBJ_DIR}/${VERILOG_HEX_MEM_FILENAME_FOR_PARAM}")

    set(VERILOG_SIDE_OUTPUT_FILE "${OBJ_DIR}/${test_case_name}_verilog_trace.txt")
    set(SIMULATOR_SIDE_RAW_OUTPUT_FILE "${OBJ_DIR}/${test_case_name}_simulator_raw_stdout.txt")
    set(SIMULATOR_SIDE_FILTERED_OUTPUT_FILE "${OBJ_DIR}/${test_case_name}_simulator_filtered_trace.txt")

    set(VERILOG_PARAM_PC_START_ADDR "64'h${pc_start_hex_no_prefix}")

    set(ASSEMBLE_CMD ${RISCV_AS} -march=rv64i -mabi=lp64 -o ${ASM_OBJECT_FILE_IN_OBJDIR} ${ASM_INPUT_FILE_FULL_PATH})
    set(LINK_CMD ${RISCV_LD} --no-relax -Ttext=0x${pc_start_hex_no_prefix} -o ${LINKED_ELF_FILE_IN_OBJDIR} ${ASM_OBJECT_FILE_IN_OBJDIR})
    set(ELF_TO_HEX_CMD
        ${Python3_EXECUTABLE} "${ELF_TO_MEMH_SCRIPT}"
        "${LINKED_ELF_FILE_IN_OBJDIR}"
        "${GENERATED_HEX_MEM_FILE_FULL_PATH_IN_OBJDIR}"
        --objcopy "${RISCV_OBJCOPY}" --readelf "${RISCV_READELF}" --section ".text" --wordsize 4)

    set(VERILATOR_EXE ${OBJ_DIR}/V${VERILOG_MODULE_NAME})
    set(BUILD_FILES_AND_VERILOG_TARGET ${test_case_name}_build_and_verilog)
    add_custom_command(
        OUTPUT ${VERILATOR_EXE}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${OBJ_DIR}
        COMMAND ${ASSEMBLE_CMD}
        COMMAND ${LINK_CMD}
        COMMAND ${ELF_TO_HEX_CMD}
        COMMAND ${PROJECT_VERILATOR_EXECUTABLE}
                -Wall --Wno-fatal --cc --exe --build --trace
                --top-module ${VERILOG_MODULE_NAME} -I${RTL_INCLUDE_PATH}
                "-GINSTR_MEM_INIT_FILE=\"${VERILOG_HEX_MEM_FILENAME_FOR_PARAM}\""
                "-GPC_START_ADDR=${VERILOG_PARAM_PC_START_ADDR}"
                ${PIPELINE_RTL_FILES}
                "${COSIM_TEST_BENCH_CPP}" --Mdir "${OBJ_DIR}"
                -CFLAGS "-std=c++17 -Wall \
                    -DPIPELINE_COSIM_TEST_CASE_NAME_STR_RAW=${test_case_name} \
                    -DNUM_CYCLES_TO_RUN=${num_cycles} \
                    -DVERILOG_OUTPUT_FILE_PATH_STR_RAW=${VERILOG_SIDE_OUTPUT_FILE}"
        DEPENDS "${COSIM_TEST_BENCH_CPP}" "${ASM_INPUT_FILE_FULL_PATH}"
                "${ELF_TO_MEMH_SCRIPT}" ${PIPELINE_RTL_FILES}
        COMMENT "Building Verilog side and support files for co-sim test: ${test_case_name}" VERBATIM
    )
    add_custom_target(${BUILD_FILES_AND_VERILOG_TARGET} DEPENDS ${VERILATOR_EXE})


    set(RUN_AND_COMPARE_TARGET run_cosim_${test_case_name})
    add_custom_target(${RUN_AND_COMPARE_TARGET}
        COMMAND ${VERILATOR_EXE}
        COMMAND ${SIMULATOR_EXECUTABLE} ${LINKED_ELF_FILE_IN_OBJDIR} ${COSIM_PLUGIN_SO_PATH} > ${SIMULATOR_SIDE_RAW_OUTPUT_FILE} 2>&1
        COMMAND ${Python3_EXECUTABLE} "${FILTER_SIM_OUTPUT_SCRIPT}"
                "${SIMULATOR_SIDE_RAW_OUTPUT_FILE}"
                "${SIMULATOR_SIDE_FILTERED_OUTPUT_FILE}"
                --skip_header 4
                --skip_footer 3
        COMMAND ${Python3_EXECUTABLE} "${COMPARE_TRACE_FILES_SCRIPT}"
                "${VERILOG_SIDE_OUTPUT_FILE}"
                "${SIMULATOR_SIDE_FILTERED_OUTPUT_FILE}"

        DEPENDS ${BUILD_FILES_AND_VERILOG_TARGET} ${SIMULATOR_TARGET_NAME} ${COSIM_PLUGIN_TARGET_NAME}
                "${FILTER_SIM_OUTPUT_SCRIPT}" "${COMPARE_TRACE_FILES_SCRIPT}"

        WORKING_DIRECTORY ${OBJ_DIR}
        COMMENT "Running co-simulation and comparing for: ${test_case_name}"
        VERBATIM
    )

    if(TARGET run_all_cosim_tests)
        add_dependencies(run_all_cosim_tests ${RUN_AND_COMPARE_TARGET})
    endif()
    if(TARGET run_all_tests AND TARGET run_all_cosim_tests)
         add_dependencies(run_all_tests run_all_cosim_tests)
    endif()

    message(STATUS "Configured CO-SIMULATION test case: ${test_case_name}")
endfunction()

add_cosim_test(addi_basic_cosim "addi_basic_instr.s" 17 "10000")
# add_cosim_test(jump_basic_cosim "jump.s" 21 "10000")
# add_cosim_test(beq_basic_cosim "beq.s" 22 "10000")
add_cosim_test(mem_basic_cosim "mem.s" 12 "10000")
add_cosim_test(complex_cosim "complex.s" 55 "10000")
add_cosim_test(complex_cosim_1 "complex_1.s" 30 "10000")
add_cosim_test(addi_slti "addi_slti.s" 10 "10000")