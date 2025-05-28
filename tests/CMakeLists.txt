# Добавить поддиректорию с юнит-тестами
add_custom_target(run_all_unit_tests)
add_subdirectory(unit)

# Добавить поддиректорию с тестами на пайплаины
add_custom_target(run_all_pipeline_tests)
add_subdirectory(pipeline_tests)

add_custom_target(run_all_cosim_tests)
add_subdirectory(cosim_tests)