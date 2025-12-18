add_library(receiver_app_lib OBJECT
  callbacks.c
  receiver.c
  voice.c
)
target_include_directories(receiver_app_lib PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(receiver_app_lib PRIVATE
  receiver_drivers_lib
  receiver_middleware_lib
)
