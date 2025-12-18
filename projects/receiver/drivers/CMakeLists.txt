add_library(receiver_drivers_lib OBJECT)

add_subdirectory(../../common/src/drivers ${CMAKE_BINARY_DIR}/common/src/drivers)
add_subdirectory(audio)
add_subdirectory(ble)
add_subdirectory(ir_nec_decoder)
add_subdirectory(leds)
target_sources(receiver_drivers_lib PUBLIC
  $<TARGET_OBJECTS:common_drivers_lib>
  $<TARGET_OBJECTS:receiver_drivers_audio_lib>
  $<TARGET_OBJECTS:receiver_drivers_ble_lib>
  $<TARGET_OBJECTS:receiver_drivers_ir_nec_decoder_lib>
  $<TARGET_OBJECTS:receiver_drivers_leds_lib>
)

target_include_directories(receiver_drivers_lib PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(receiver_drivers_lib PUBLIC
  common_drivers_lib
  receiver_drivers_audio_lib
  receiver_drivers_ble_lib
  receiver_drivers_ir_nec_decoder_lib
  receiver_drivers_leds_lib
)
