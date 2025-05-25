# Добавить поддиректорию с юнит-тестами
add_custom_target(run_all_unit_tests)
add_subdirectory(unit)

# В будущем можно добавить другие типы тестов (например, для всего ядра)
# add_subdirectory(core_sim)