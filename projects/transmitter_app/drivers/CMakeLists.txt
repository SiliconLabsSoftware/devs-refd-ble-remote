add_library(transmitter_drivers_lib OBJECT)

add_subdirectory(platform_abstraction) # Other libraries might depend on this
add_subdirectory(accelerometer)
add_subdirectory(battery)
add_subdirectory(keys)
add_subdirectory(leds)
add_subdirectory(../../common/src/drivers ${CMAKE_BINARY_DIR}/common/src/drivers)
target_sources(transmitter_drivers_lib PUBLIC
  $<TARGET_OBJECTS:transmitter_driver_platform_abstraction_lib>
  $<TARGET_OBJECTS:transmitter_driver_accelerometer_lib>
  $<TARGET_OBJECTS:transmitter_driver_battery_lib>
  $<TARGET_OBJECTS:transmitter_driver_keys_lib>
  $<TARGET_OBJECTS:transmitter_driver_leds_lib>
  $<TARGET_OBJECTS:common_drivers_lib>
)

target_include_directories(transmitter_drivers_lib PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(transmitter_drivers_lib PUBLIC
  transmitter_driver_platform_abstraction_lib
  transmitter_driver_accelerometer_lib
  transmitter_driver_battery_lib
  transmitter_driver_keys_lib
  transmitter_driver_leds_lib
  common_drivers_lib
)
