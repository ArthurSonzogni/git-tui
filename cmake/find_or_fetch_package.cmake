# Some developers would be happier with the dependency provided from their
# package manager. Use them if they are installed already.
function(find_or_fetch_package name version repository hash)
  find_package(${name} QUIET)
  if (${name}_FOUND)
    message(STATUS "Dependency: ${name}. Version ${version} from ${${name}_DIR}")
    return()
  endif()

  message(STATUS "Dependency: ${name}. Version ${version} from ${repository}")

  option(FETCHCONTENT_UPDATES_DISCONNECTED TRUE)
  option(FETCHCONTENT_QUIET FALSE)
  include(FetchContent)

  FetchContent_Declare(${name}
    GIT_REPOSITORY ${repository}
    GIT_TAG ${hash}
    GIT_PROGRESS TRUE
    GIT_SHALLOW FALSE
  )

  FetchContent_GetProperties(${name})
  if(${name}_POPULATED)
    return()
  endif()

  FetchContent_Populate(${name})
  add_subdirectory(
    ${${name}_SOURCE_DIR}
    ${${name}_BINARY_DIR}
    EXCLUDE_FROM_ALL
  )
endfunction()
