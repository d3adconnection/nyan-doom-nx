# CMake toolchain for building Nyan Doom as Nintendo Switch homebrew.
#
# Usage (from repo root):
#   cmake -S prboom2 -B build-switch \
#         -DCMAKE_TOOLCHAIN_FILE=prboom2/cmake/SwitchToolchain.cmake \
#         -DNYAN_HOST_TOOLCHAIN=<native toolchain for WAD tools>
#
# Requires the DEVKITPRO environment variable (set by devkitPro install).

if(NOT DEFINED ENV{DEVKITPRO})
  message(FATAL_ERROR "DEVKITPRO environment variable is not set. Install devkitPro first.")
endif()

# Delegate to the official devkitPro Switch toolchain which sets up the
# aarch64-none-elf cross compiler and defines NINTENDO_SWITCH / __SWITCH__.
include("$ENV{DEVKITPRO}/cmake/Switch.cmake")
