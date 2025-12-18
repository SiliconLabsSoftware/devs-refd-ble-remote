add_library(transmitter_driver_platform_abstraction_lib OBJECT
  sl_atomic.c
  sl_ble.c
  sl_drv_mic.c
  sl_i2c_drv.c
  sl_nvm.c
  sl_pwr_ctrl.c
  sl_sleep.c
)
target_include_directories(transmitter_driver_platform_abstraction_lib PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(transmitter_driver_platform_abstraction_lib PUBLIC
  common_drivers_lib
)
target_link_libraries(transmitter_driver_platform_abstraction_lib PRIVATE
  slc_transmitter_app
)
