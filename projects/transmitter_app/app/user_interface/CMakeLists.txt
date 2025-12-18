add_library(transmitter_app_user_if_lib OBJECT
  key_handler.c
  led_activity.c
  led_backlight.c
  voice.c
)
target_include_directories(transmitter_app_user_if_lib PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(transmitter_app_user_if_lib PRIVATE
  transmitter_drivers_lib
  transmitter_middleware_lib
)
