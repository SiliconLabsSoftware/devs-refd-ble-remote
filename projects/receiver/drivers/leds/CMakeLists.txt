add_library(receiver_drivers_leds_lib OBJECT
  sl_activity_led.c
)
target_include_directories(receiver_drivers_leds_lib PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(receiver_drivers_leds_lib PRIVATE
  slc_receiver
  common_drivers_lib
)
