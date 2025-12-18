add_library(common_drivers_lib OBJECT
  assert.c
  sl_log.c
)
target_include_directories(common_drivers_lib PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../${PROJECT_NAME}/${COPIED_SDK_PATH}/platform/common/toolchain/inc
)
target_link_libraries(common_drivers_lib PRIVATE
   slc_${PROJECT_NAME}
)
# TODO HACK: Inherit compile options from the SDK
#            These options should be rather in the toolchain file
target_compile_options(common_drivers_lib PUBLIC
  $<TARGET_PROPERTY:slc_${PROJECT_NAME},INTERFACE_COMPILE_OPTIONS>
)
