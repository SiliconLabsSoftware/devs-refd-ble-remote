# Define minimal required version of CMake.
cmake_minimum_required(VERSION "3.25")

# Project definition
project(
	transmitter_btl
	VERSION 1.0
	LANGUAGES C CXX ASM
)
add_compile_options(
  -Werror
)

# Include the definition of the slc_transmitter_btl target,
# which contains the content of the SLC project
include(transmitter_btl.cmake)

add_executable(transmitter_btl
    # Add additional sources here
    "../sl_bootloader_custom/assert.c"
    "../sl_bootloader_custom/btl_apploader_common.c"
    "../sl_bootloader_custom/btl_main.c"
    "../sl_bootloader_custom/sl_led_handler.c"
    "../sl_bootloader_custom/sl_recovery_handler.c"
    "../sl_bootloader_custom/sl_dwt_handler.c"
)

set_source_files_properties("../${COPIED_SDK_PATH}/platform/bootloader/core/btl_main.c" PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties("../${COPIED_SDK_PATH}/platform/bootloader/communication/apploader/btl_apploader_common.c" PROPERTIES HEADER_FILE_ONLY TRUE)

target_include_directories(transmitter_btl PUBLIC
    # Add additional include paths here
    "../${COPIED_SDK_PATH}/platform/bootloader/communication/apploader"
    "../sl_bootloader_custom"
    "../../common/src"
)

target_compile_definitions(transmitter_btl PUBLIC
    # Add additional macros here
)

target_compile_options(transmitter_btl PUBLIC
    # Set additional compiler flags here
)

target_link_options(transmitter_btl PUBLIC
    # Set additional linker flags here
)

# Link with the content defined in the SLC project
target_link_libraries(transmitter_btl PRIVATE
    slc_transmitter_btl
)

# Include managed project content if available
include(transmitter_btl_project.cmake OPTIONAL RESULT_VARIABLE managed_project)
if(managed_project)
    message(STATUS "Using managed project content from ${managed_project}")
endif()

# Force the gcc linker command
set_target_properties(transmitter_btl PROPERTIES LINKER_LANGUAGE C)

# Create .bin, .hex and .s37 artifacts after building the project
add_custom_command(TARGET transmitter_btl
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} ${OBJCOPY_SREC_CMD} "$<TARGET_FILE:transmitter_btl>" "$<TARGET_FILE_DIR:transmitter_btl>/$<TARGET_FILE_BASE_NAME:transmitter_btl>.s37"
    COMMAND ${CMAKE_OBJCOPY} ${OBJCOPY_IHEX_CMD} "$<TARGET_FILE:transmitter_btl>" "$<TARGET_FILE_DIR:transmitter_btl>/$<TARGET_FILE_BASE_NAME:transmitter_btl>.hex"
    COMMAND ${CMAKE_OBJCOPY} ${OBJCOPY_BIN_CMD}  "$<TARGET_FILE:transmitter_btl>" "$<TARGET_FILE_DIR:transmitter_btl>/$<TARGET_FILE_BASE_NAME:transmitter_btl>.bin"
    COMMAND ${CMAKE_SIZE_UTIL} "$<TARGET_FILE:transmitter_btl>" "-A"
)

# Run post-build pipeline to perform additional post-processing
if(post_build_command)
add_custom_command(TARGET transmitter_btl
    POST_BUILD
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/..
    COMMAND ${post_build_command}
)
endif()
