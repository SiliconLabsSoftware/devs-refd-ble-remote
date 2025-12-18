# Define minimal required version of CMake.
cmake_minimum_required(VERSION "3.25")

# Project definition
project(
	transmitter_app
	VERSION 1.0
	LANGUAGES C CXX ASM
)

add_compile_options(
  -Wformat=2
  -Wformat-security
  -Wformat-extra-args
  -Wformat-nonliteral
  -Wformat-zero-length
  -Wstrict-overflow
  -Werror
  -fno-builtin-printf
  -fno-builtin-sprintf
  -fstack-protector
)

# Include the definition of the slc_transmitter_app target,
# which contains the content of the SLC project
include(transmitter_app.cmake)
target_compile_definitions(slc_transmitter_app PUBLIC
  "$<$<CONFIG:DEBUG>:DEBUG_EFM_USER>"
)

add_executable(transmitter_app
    ../cli.c
)
add_subdirectory(../app ${CMAKE_BINARY_DIR}/app)
add_subdirectory(../middleware ${CMAKE_BINARY_DIR}/middleware)
add_subdirectory(../drivers ${CMAKE_BINARY_DIR}/drivers)

target_link_libraries(transmitter_app PRIVATE
    slc_transmitter_app
    transmitter_app_lib
    transmitter_middleware_lib
    transmitter_drivers_lib
)

# Include managed project content if available
include(transmitter_app_project.cmake OPTIONAL RESULT_VARIABLE managed_project)
if(managed_project)
    message(STATUS "Using managed project content from ${managed_project}")
endif()

# Force the gcc linker command
set_target_properties(transmitter_app PROPERTIES LINKER_LANGUAGE C)

# Create .bin, .hex and .s37 artifacts after building the project
add_custom_command(TARGET transmitter_app
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} ${OBJCOPY_SREC_CMD} "$<TARGET_FILE:transmitter_app>" "$<TARGET_FILE_DIR:transmitter_app>/$<TARGET_FILE_BASE_NAME:transmitter_app>.s37"
    COMMAND ${CMAKE_OBJCOPY} ${OBJCOPY_IHEX_CMD} "$<TARGET_FILE:transmitter_app>" "$<TARGET_FILE_DIR:transmitter_app>/$<TARGET_FILE_BASE_NAME:transmitter_app>.hex"
    COMMAND ${CMAKE_OBJCOPY} ${OBJCOPY_BIN_CMD}  "$<TARGET_FILE:transmitter_app>" "$<TARGET_FILE_DIR:transmitter_app>/$<TARGET_FILE_BASE_NAME:transmitter_app>.bin"
    COMMAND ${CMAKE_SIZE_UTIL} "$<TARGET_FILE:transmitter_app>" "-A"
)

# Run post-build pipeline to perform additional post-processing
if(post_build_command)
add_custom_command(TARGET transmitter_app
    POST_BUILD
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/..
    COMMAND ${post_build_command}
)
endif()
