add_library(transmitter_driver_leds_lib OBJECT
  sl_ir_led.c
  sl_led_effect.c
  sl_led.c
)
target_include_directories(transmitter_driver_leds_lib PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(transmitter_driver_leds_lib PRIVATE
  transmitter_driver_platform_abstraction_lib
  slc_transmitter_app
)
