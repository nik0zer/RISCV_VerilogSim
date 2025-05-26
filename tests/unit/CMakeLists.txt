function(add_verilator_test test_name)
    set(VERILOG_MODULE_NAME ${test_name})
    set(CPP_TESTBENCH_FILE ${CMAKE_CURRENT_SOURCE_DIR}/${test_name}.cpp)

    set(TOP_RTL_FILE ${CMAKE_SOURCE_DIR}/rtl/modules/${VERILOG_MODULE_NAME}.sv)

    set(ALL_RTL_FILES ${TOP_RTL_FILE})
    foreach(EXTRA_RTL_MODULE_NAME ${ARGN})
        list(APPEND ALL_RTL_FILES ${CMAKE_SOURCE_DIR}/rtl/modules/${EXTRA_RTL_MODULE_NAME}.sv)
    endforeach()

    set(RTL_INCLUDE_PATH ${CMAKE_SOURCE_DIR}/rtl)

    set(OBJ_DIR ${CMAKE_CURRENT_BINARY_DIR}/obj_dir_${test_name})
    set(VERILATOR_GENERATED_EXE ${OBJ_DIR}/V${VERILOG_MODULE_NAME})

    set(BUILD_TARGET_NAME ${test_name}_build_verilated_exe)
    add_custom_target(${BUILD_TARGET_NAME} ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory ${OBJ_DIR}
        COMMAND ${PROJECT_VERILATOR_EXECUTABLE}
                -Wall --Wno-fatal --cc --exe --build --trace
                -Wno-UNUSED
                --top-module ${VERILOG_MODULE_NAME}
                -I${RTL_INCLUDE_PATH}
                ${ALL_RTL_FILES}
                "${CPP_TESTBENCH_FILE}"
                --Mdir "${OBJ_DIR}"
                -CFLAGS "-std=c++17 -Wall"
        DEPENDS ${ALL_RTL_FILES} ${CPP_TESTBENCH_FILE}
        COMMENT "Verilating and Building executable for ${test_name} using system Verilator"
        VERBATIM
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )

    set(RUN_TARGET_NAME run_${test_name}_unittest)
    add_custom_target(${RUN_TARGET_NAME}
        COMMAND "${VERILATOR_GENERATED_EXE}"
        DEPENDS ${BUILD_TARGET_NAME}
        WORKING_DIRECTORY ${OBJ_DIR}
        COMMENT "Running Verilated test for ${test_name}"
        VERBATIM
    )

    message(STATUS "Configured system Verilator test: ${test_name}")
    message(STATUS "  RTL Files: ${ALL_RTL_FILES}")
    message(STATUS "  Build target: ${BUILD_TARGET_NAME}")
    message(STATUS "  Run target: ${RUN_TARGET_NAME}")

    if(TARGET run_all_unit_tests)
        add_dependencies(run_all_unit_tests ${RUN_TARGET_NAME})
    endif()
endfunction()

add_verilator_test(regfile)
add_verilator_test(ram)
add_verilator_test(alu)
add_verilator_test(mux2)
add_verilator_test(mux3)
add_verilator_test(imm)
add_verilator_test(control_unit main_decoder alu_decoder)
add_verilator_test(flopr)
add_verilator_test(flopenr)
add_verilator_test(pipeline control_unit flopr flopenr ram regfile imm alu mux2 mux3 alu_decoder main_decoder)