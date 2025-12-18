add_library(receiver_drivers_ir_nec_decoder_lib OBJECT
  sl_ir_nec_decoder.c
)
target_include_directories(receiver_drivers_ir_nec_decoder_lib PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(receiver_drivers_ir_nec_decoder_lib PRIVATE
  slc_receiver
  common_drivers_lib
  receiver_drivers_leds_lib
)
