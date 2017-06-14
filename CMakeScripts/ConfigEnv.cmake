# -----------------------------------------------------------------------------
# Set platform defaults (originally copied from darktable)
# -----------------------------------------------------------------------------
if(WIN32)
  message("-- Windows build detected, setting default features")

  include(mingwenv.cmake)
  
  if(NOT HAVE_MSYS2)
    list(INSERT CMAKE_SYSTEM_INCLUDE_PATH 0 ${DEVLIBS_PATH})
    list(INSERT CMAKE_SYSTEM_LIBRARY_PATH 0 ${DEVLIBS_PATH})
  endif()
  
  set(CMAKE_C_COMPILER "${MINGW_BIN}/gcc.exe")
  set(CMAKE_C_LINK_EXECUTABLE "${MINGW_BIN}/gcc.exe")
  set(CMAKE_CXX_COMPILER "${MINGW_BIN}/g++.exe")
  set(CMAKE_CXX_LINK_EXECUTABLE "${MINGW_BIN}/g++.exe")
  set(CMAKE_CXX_STANDARD 11)
  
  # Setup Windows resource files compiler.
  set(CMAKE_RC_COMPILER "${MINGW_BIN}/windres.exe")
  set(CMAKE_RC_COMPILER_INIT windres)
  enable_language(RC)
  set(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> -O coff -i <SOURCE> -o <OBJECT>")
  
  # These options are required for having i18n support on Windows.
  option(ENABLE_NLS "Compile with Native Language Support (using gettext)" ON)
  option(HAVE_BIND_TEXTDOMAIN_CODESET "Compile with 'bind_textdomain_codeset' function" ON)
  
  # Does not compile on Windows with these options.
  option(BR_PTHREADS "Use binreloc thread support" OFF)
  option(ENABLE_BINRELOC "Use AutoPackage" OFF)
endif()

if(APPLE)
  message("-- Mac OS X build detected, setting default features")

  # use brew packages before macOS frameworks
  set(CMAKE_FIND_FRAMEWORK "LAST")
endif()
