add_library(receiver_middleware_lib STATIC)
add_subdirectory(../../common/src/middleware ${CMAKE_BINARY_DIR}/common/src/middleware)
target_include_directories(receiver_middleware_lib PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(receiver_middleware_lib PUBLIC
  common_middleware_lib
)
target_link_libraries(receiver_middleware_lib PRIVATE
  receiver_drivers_lib
)
