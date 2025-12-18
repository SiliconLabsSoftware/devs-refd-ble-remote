add_library(transmitter_app_lib OBJECT
  callbacks.c
  remote.c
)

add_subdirectory(user_interface)
target_sources(transmitter_app_lib PUBLIC
  $<TARGET_OBJECTS:transmitter_app_user_if_lib>
)

target_include_directories(transmitter_app_lib PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(transmitter_app_lib PRIVATE
  transmitter_app_user_if_lib
  transmitter_drivers_lib
  transmitter_middleware_lib
)
