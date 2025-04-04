cmake_minimum_required (VERSION 3.15)
project(git-tui
  LANGUAGES CXX
  VERSION 1.3.0
)

# --- Dependencies--------------------------------------------------------------
include(cmake/find_or_fetch_package.cmake)

find_or_fetch_package(ftxui v6.0.0
  https://github.com/ArthurSonzogni/ftxui
  v6.0.0
)

find_or_fetch_package(subprocess v0.4.0
  https://github.com/benman64/subprocess
  e1cae5e86e5d089e65e906f7c13917b7bbb75d04
)

# ------------------------------------------------------------------------------

add_executable(git-tui
  src/diff.cpp
  src/diff.hpp
  src/git.cpp
  src/git.hpp
  src/help.cpp
  src/help.hpp
  src/log.cpp
  src/log.hpp
  src/main.cpp
  src/scroller.cpp
  src/scroller.hpp
  src/simple_button_options.hpp
  src/version.cpp
  src/version.hpp
)

target_include_directories(git-tui
  PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/src
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(git-tui
  PRIVATE subprocess
  PRIVATE ftxui::screen
  PRIVATE ftxui::dom
  PRIVATE ftxui::component
)

if (MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
else()
  target_compile_options(git-tui
    PRIVATE "-Wall"
    PRIVATE "-Wextra"
    PRIVATE "-pedantic"
    PRIVATE "-Werror"
    PRIVATE "-Wdeprecated"
  )
endif()

# ------------------------------------------------------------------------------

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/src/environment.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/src/environment.h
)

# ------------------------------------------------------------------------------

install(TARGETS git-tui RUNTIME DESTINATION "bin")

# ------------------------------------------------------------------------------
if (UNIX AND NOT APPLE)
  set(CPACK_GENERATOR "DEB;External;RPM;STGZ;TBZ2;TGZ;TXZ;TZ;TZST;ZIP")
elseif (UNIX AND APPLE)
  set(CPACK_GENERATOR "DragNDrop;NuGet;TGZ;ZIP")
elseif (WIN32)
  set(CPACK_GENERATOR "DEB;NuGet;TGZ;ZIP")
else()
  set(CPACK_GENERATOR "ZIP")
endif()

set(CPACK_DEBIAN_PACKAGE_DEPENDS " ")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE_URL "https://github.com/ArthurSonzogni/git-tui/")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Arthur Sonzogni")
set(CPACK_DEBIAN_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A collection of human friend git command with terminal UI")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/ArthurSonzogni/git-tui/")
set(CPACK_PACKAGE_NAME "git-tui")
set(CPACK_PACKAGE_VENDOR "Arthur Sonzogni")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")

include(CPack)
