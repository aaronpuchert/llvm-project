
include(CheckCXXCompilerFlag)

unset(add_flag_if_supported)

# Mangle the name of a compiler flag into a valid CMake identifier.
# Ex: --std=c++11 -> STD_EQ_CXX11
macro(mangle_name str output)
  string(STRIP "${str}" strippedStr)
  string(REGEX REPLACE "^/" "" strippedStr "${strippedStr}")
  string(REGEX REPLACE "^-+" "" strippedStr "${strippedStr}")
  string(REGEX REPLACE "-+$" "" strippedStr "${strippedStr}")
  string(REPLACE "-" "_" strippedStr "${strippedStr}")
  string(REPLACE "=" "_EQ_" strippedStr "${strippedStr}")
  string(REPLACE "+" "X" strippedStr "${strippedStr}")
  string(TOUPPER "${strippedStr}" ${output})
endmacro()

# Remove a list of flags from all CMake variables that affect compile flags.
# This can be used to remove unwanted flags specified on the command line
# or added in other parts of LLVM's cmake configuration.
macro(remove_flags)
  foreach(var ${ARGN})
    string(REPLACE "${var}" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
    string(REPLACE "${var}" "" CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL}")
    string(REPLACE "${var}" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    string(REPLACE "${var}" "" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
    string(REPLACE "${var}" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    string(REPLACE "${var}" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
    string(REPLACE "${var}" "" CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")
    string(REPLACE "${var}" "" CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}")
    string(REPLACE "${var}" "" CMAKE_SHARED_MODULE_FLAGS "${CMAKE_SHARED_MODULE_FLAGS}")
    remove_definitions(${var})
  endforeach()
endmacro(remove_flags)

macro(check_flag_supported flag)
    mangle_name("${flag}" flagname)
    check_cxx_compiler_flag("${flag}" "CXX_SUPPORTS_${flagname}_FLAG")
endmacro()

macro(append_flags DEST)
  foreach(value ${ARGN})
    list(APPEND ${DEST} ${value})
    list(APPEND ${DEST} ${value})
  endforeach()
endmacro()

# If the specified 'condition' is true then append the specified list of flags to DEST
macro(append_flags_if condition DEST)
  if (${condition})
    list(APPEND ${DEST} ${ARGN})
  endif()
endmacro()

# Add each flag in the list specified by DEST if that flag is supported by the current compiler.
macro(append_flags_if_supported DEST)
  foreach(flag ${ARGN})
    mangle_name("${flag}" flagname)
    check_cxx_compiler_flag("${flag}" "CXX_SUPPORTS_${flagname}_FLAG")
    append_flags_if(CXX_SUPPORTS_${flagname}_FLAG ${DEST} ${flag})
  endforeach()
endmacro()

# Add a macro definition if condition is true.
macro(define_if condition def)
  if (${condition})
    add_definitions(${def})
  endif()
endmacro()

# Add a macro definition if condition is not true.
macro(define_if_not condition def)
  if (NOT ${condition})
    add_definitions(${def})
  endif()
endmacro()

# Add a macro definition to the __config_site file if the specified condition
# is 'true'. Note that '-D${def}' is not added. Instead it is expected that
# the build include the '__config_site' header.
macro(config_define_if condition def)
  if (${condition})
    set(${def} ON)
  endif()
endmacro()

macro(config_define value def)
  set(${def} ${value})
endmacro()

# Turn a comma separated CMake list into a space separated string.
macro(split_list listname)
  string(REPLACE ";" " " ${listname} "${${listname}}")
endmacro()

# For each specified flag, add that compile flag to the provided target.
# The flags are added with the given visibility, i.e. PUBLIC|PRIVATE|INTERFACE.
function(target_add_compile_flags_if_supported target visibility)
  foreach(flag ${ARGN})
    mangle_name("${flag}" flagname)
    check_cxx_compiler_flag("${flag}" "CXX_SUPPORTS_${flagname}_FLAG")
    if (CXX_SUPPORTS_${flagname}_FLAG)
      target_compile_options(${target} ${visibility} ${flag})
    endif()
  endforeach()
endfunction()
