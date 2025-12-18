add_library(transmitter_middleware_lib STATIC
  filter.c
)
target_include_directories(transmitter_middleware_lib PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)
add_subdirectory(../../common/src/middleware ${CMAKE_BINARY_DIR}/common/src/middleware)
target_link_libraries(transmitter_middleware_lib PUBLIC
  common_middleware_lib
)
target_link_libraries(transmitter_middleware_lib PRIVATE
  transmitter_drivers_lib
)
