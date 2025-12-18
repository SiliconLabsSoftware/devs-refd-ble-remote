add_library(common_middleware_lib STATIC
  adpcm.c
)
target_include_directories(common_middleware_lib PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(common_middleware_lib PRIVATE
  common_drivers_lib
)
