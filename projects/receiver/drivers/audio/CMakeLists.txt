add_library(receiver_drivers_audio_lib OBJECT
  common_irq_handlers.c
  sl_dac.c
  sl_i2s.c
)

target_include_directories(receiver_drivers_audio_lib PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(receiver_drivers_audio_lib PRIVATE
  slc_receiver
  common_drivers_lib
)
