add_library(transmitter_driver_accelerometer_lib OBJECT
  sl_acc.c
  adxl/sl_acc_adxl_367.c
  icm/sl_acc_icm_20648.c
  lis2dx12/sl_acc_lis2dx12_common.c
  lis2dx12/lis2de12/lis2de12_reg.c
  lis2dx12/lis2de12/sl_acc_lis2de12.c
  lis2dx12/lis2dw12/lis2dw12_reg.c
  lis2dx12/lis2dw12/sl_acc_lis2dw12.c
)
target_include_directories(transmitter_driver_accelerometer_lib PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(transmitter_driver_accelerometer_lib PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/icm
  ${CMAKE_CURRENT_SOURCE_DIR}/lis2dx12
)
target_link_libraries(transmitter_driver_accelerometer_lib PRIVATE
  transmitter_driver_platform_abstraction_lib
  slc_transmitter_app
)
