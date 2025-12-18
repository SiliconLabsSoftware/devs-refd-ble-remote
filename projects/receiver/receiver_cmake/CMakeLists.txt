# Define minimal required version of CMake.
cmake_minimum_required(VERSION "3.25")

# Project definition
project(
	receiver
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

# Include the definition of the slc_receiver target,
# which contains the content of the SLC project
include(receiver.cmake)
target_compile_definitions(slc_receiver PUBLIC
  "$<$<CONFIG:DEBUG>:DEBUG_EFM_USER>"
)

add_executable(receiver
    ../cli.c
)

add_subdirectory(../app ${CMAKE_BINARY_DIR}/app)
add_subdirectory(../middleware ${CMAKE_BINARY_DIR}/middleware)
add_subdirectory(../drivers ${CMAKE_BINARY_DIR}/drivers)

# Link with the content defined in the SLC project
target_link_libraries(receiver PRIVATE
    slc_receiver
    receiver_app_lib
    receiver_middleware_lib
    receiver_drivers_lib
)
target_include_directories(slc_receiver PUBLIC
    ../simplicity_sdk_ext
)

# Include managed project content if available
include(receiver_project.cmake OPTIONAL RESULT_VARIABLE managed_project)
if(managed_project)
    message(STATUS "Using managed project content from ${managed_project}")
endif()

# Force the gcc linker command
set_target_properties(receiver PROPERTIES LINKER_LANGUAGE C)

# Create .bin, .hex and .s37 artifacts after building the project
add_custom_command(TARGET receiver
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} ${OBJCOPY_SREC_CMD} "$<TARGET_FILE:receiver>" "$<TARGET_FILE_DIR:receiver>/$<TARGET_FILE_BASE_NAME:receiver>.s37"
    COMMAND ${CMAKE_OBJCOPY} ${OBJCOPY_IHEX_CMD} "$<TARGET_FILE:receiver>" "$<TARGET_FILE_DIR:receiver>/$<TARGET_FILE_BASE_NAME:receiver>.hex"
    COMMAND ${CMAKE_OBJCOPY} ${OBJCOPY_BIN_CMD}  "$<TARGET_FILE:receiver>" "$<TARGET_FILE_DIR:receiver>/$<TARGET_FILE_BASE_NAME:receiver>.bin"
    COMMAND ${CMAKE_SIZE_UTIL} "$<TARGET_FILE:receiver>" "-A"
)

# Run post-build pipeline to perform additional post-processing
if(post_build_command)
add_custom_command(TARGET receiver
    POST_BUILD
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/..
    COMMAND ${post_build_command}
)
endif()
