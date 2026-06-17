macro(get_linker_script_path OUTVAR NAME)
  execute_process(
    COMMAND ${CMAKE_C_COMPILER} -print-file-name=${NAME}
    OUTPUT_VARIABLE ${OUTVAR}
    OUTPUT_STRIP_TRAILING_WHITESPACE)
endmacro()

# Determine platform Code
if("${FXSDK_PLATFORM_LONG}" STREQUAL fxCG50)
  set(PC cg)
  set(INTF_DEFN FXCG50)
  get_linker_script_path(LS1 fxcg50_fastload.ld)
  get_linker_script_path(LS2 fxcg50.ld)
  set(INTF_LINKER_SCRIPT
    "$<IF:$<CONFIG:FastLoad>,${LS1},${LS2}>")
elseif("${FXSDK_PLATFORM_LONG}" STREQUAL fx9860G)
  set(PC fx)
  set(INTF_DEFN FX9860G)
  get_linker_script_path(INTF_LINKER_SCRIPT "fx9860g.ld")
elseif("${FXSDK_PLATFORM_LONG}" STREQUAL fx9860G_G3A)
  set(PC fxg3a)
  set(INTF_DEFN FX9860G_G3A)
  get_linker_script_path(INTF_LINKER_SCRIPT "fxcg50.ld")
elseif("${FXSDK_PLATFORM_LONG}" STREQUAL fxCP)
  set(PC cp)
  set(INTF_DEFN FXCP)
  get_linker_script_path(INTF_LINKER_SCRIPT "fxcp_hh2.ld")
else()
  message(FATAL_ERROR "gint: unknown fxSDK platform '${FXSDK_PLATFORM}'")
endif()

execute_process(
  COMMAND ${CMAKE_C_COMPILER} -print-file-name=libgint-${PC}.a
  OUTPUT_VARIABLE GINT_PATH
  OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(
  COMMAND ${CMAKE_C_COMPILER} -print-file-name=libc.a
  OUTPUT_VARIABLE FXLIBC_PATH
  OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(
  COMMAND fxsdk path include
  OUTPUT_VARIABLE FXSDK_INCLUDE
  OUTPUT_STRIP_TRAILING_WHITESPACE)
set(GINT_CONFIG_PATH "${FXSDK_INCLUDE}/gint/config.h")

if("${GINT_PATH}" STREQUAL "libgint-${PC}.a")
  unset(PATH_TO_LIBGINT)
else()
  set(PATH_TO_LIBGINT TRUE)
  if(GINT_CONFIG_PATH)
    execute_process(
      COMMAND sed "s/#define GINT_VERSION \"\\([^\"]\\{1,\\}\\)\"/\\1/p; d" ${GINT_CONFIG_PATH}
      OUTPUT_VARIABLE GINT_VERSION
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  endif()
endif()

if("${FXLIBC_PATH}" STREQUAL "libc.a")
  unset(PATH_TO_FXLIBC)
else()
  set(PATH_TO_FXLIBC TRUE)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Gint
  REQUIRED_VARS PATH_TO_FXLIBC PATH_TO_LIBGINT
  VERSION_VAR GINT_VERSION)

if(Gint_FOUND)
  if(NOT TARGET Gint::Fxlibc)
    add_library(Gint::Fxlibc UNKNOWN IMPORTED)
  endif()
  set_target_properties(Gint::Fxlibc PROPERTIES
    IMPORTED_LOCATION "${FXLIBC_PATH}"
    INTERFACE_LINK_OPTIONS "-lgcc")

  if(NOT TARGET Gint::Gint)
    add_library(Gint::Gint UNKNOWN IMPORTED)
  endif()
  set_target_properties(Gint::Gint PROPERTIES
    IMPORTED_LOCATION "${GINT_PATH}"
    INTERFACE_COMPILE_OPTIONS -fstrict-volatile-bitfields
    INTERFACE_COMPILE_DEFINITIONS "${INTF_DEFN}"
    INTERFACE_LINK_LIBRARIES "-lopenlibm;-lgcc"
    INTERFACE_LINK_OPTIONS "-T;${INTF_LINKER_SCRIPT}"
    INTERFACE_LINK_DEPENDS "${INTF_LINKER_SCRIPT}")

  target_link_libraries(Gint::Gint INTERFACE Gint::Fxlibc)
  target_link_libraries(Gint::Fxlibc INTERFACE Gint::Gint)
endif()
