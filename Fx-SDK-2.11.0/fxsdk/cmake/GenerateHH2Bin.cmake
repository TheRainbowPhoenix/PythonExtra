include(FxsdkUtils)

function(generate_hh2_bin)
  cmake_parse_arguments(HH2 "" "TARGET;OUTPUT" "" ${ARGN})

  # Check arguments

  if(DEFINED HH2_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "generate_hh2_bin: Unrecognized arguments ${HH2_UNPARSED_ARGUMENTS}")
  endif()

  if(NOT DEFINED HH2_TARGET)
    message(FATAL_ERROR "generate_hh2_bin: TARGET argument is required")
  endif()

  # Find output file name

  if(DEFINED HH2_OUTPUT)
    get_filename_component(HH2_OUTPUT "${HH2_OUTPUT}" ABSOLUTE
      BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    shell_escape("${HH2_OUTPUT}" HH2_OUTPUT)
  else()
    set(HH2_OUTPUT "${HH2_TARGET}-hh2.bin")
  endif()

  # Generate objcopy command

  string(REPLACE "gcc" "objcopy" OBJCOPY "${CMAKE_C_COMPILER}")

  add_custom_command(
    TARGET "${HH2_TARGET}" POST_BUILD
    COMMAND fxsdk script patch_hh2_filename.py ${HH2_TARGET} ${HH2_OUTPUT}
    COMMAND ${OBJCOPY} -O binary -R .bss -R .gint_bss ${HH2_TARGET} ${HH2_OUTPUT}
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMAND_EXPAND_LISTS)
endfunction()
