cmake_minimum_required (VERSION 3.15)

# --- Fetch FTXUI --------------------------------------------------------------
include(FetchContent)
set(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)
set(FETCHCONTENT_QUIET FALSE)

FetchContent_Declare(ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
  GIT_TAG 81b7207121e41c3b838eca6f54396e4f676fa3d8
  GIT_PROGRESS TRUE
  GIT_SHALLOW FALSE
)

FetchContent_Declare(subprocess
  GIT_REPOSITORY https://github.com/benman64/subprocess
  GIT_TAG e1cae5e86e5d089e65e906f7c13917b7bbb75d04
  GIT_PROGRESS TRUE
  GIT_SHALLOW FALSE
)

FetchContent_GetProperties(ftxui)
if(NOT ftxui_POPULATED)
  FetchContent_Populate(ftxui)
  add_subdirectory(
    ${ftxui_SOURCE_DIR}
    ${ftxui_BINARY_DIR}
    EXCLUDE_FROM_ALL
  )
endif()


FetchContent_GetProperties(subprocess)
if(NOT subprocess_POPULATED)
  FetchContent_Populate(subprocess)
  add_subdirectory(
    ${subprocess_SOURCE_DIR}
    ${subprocess_BINARY_DIR}
    EXCLUDE_FROM_ALL
  )
endif()

# ------------------------------------------------------------------------------

set(git_tag 0)
find_package(Git QUIET)
if (Git_FOUND)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --tags
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE git_tag
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  string(REGEX REPLACE "v\(.*\)-.*" "\\1" git_tag ${git_tag})
  string(REGEX REPLACE "v" "" git_tag ${git_tag})
  string(REGEX REPLACE "-" "."  git_tag ${git_tag})
endif()


project(git-tui
  LANGUAGES CXX
  VERSION ${git_tag}
)

add_executable(git-tui
  src/diff.cpp
  src/diff.hpp
  src/log.cpp
  src/log.hpp
  src/help.cpp
  src/help.hpp
  src/version.cpp
  src/version.hpp
  src/main.cpp
  src/scroller.cpp
  src/scroller.hpp
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

# C++17 is used. We requires fold expressions at least.
set_target_properties(git-tui PROPERTIES CXX_STANDARD 17)

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
