add_library(transmitter_driver_keys_lib OBJECT
  sl_key_matrix.c
)
target_include_directories(transmitter_driver_keys_lib PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(transmitter_driver_keys_lib PRIVATE
  transmitter_driver_platform_abstraction_lib
  slc_transmitter_app
)
