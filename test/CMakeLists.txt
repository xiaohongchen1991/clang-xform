if (BUILD_TESTS)

  # Download and unpack googletest at configure time
  configure_file(CMakeLists.txt.in googletest-download/CMakeLists.txt)
  execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
  if(result)
    message(FATAL_ERROR "CMake step for googletest failed: ${result}")
  endif()
  execute_process(COMMAND ${CMAKE_COMMAND} --build .
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
  if(result)
    message(FATAL_ERROR "Build step for googletest failed: ${result}")
  endif()

  # Prevent overriding the parent project's compiler/linker
  # settings on Windows
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

  # Add googletest directly to our build. This defines
  # the gtest and gtest_main targets.
  add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/googletest-src
    ${CMAKE_CURRENT_BINARY_DIR}/googletest-build
    EXCLUDE_FROM_ALL)

  # The gtest/gtest_main targets carry header search path
  # dependencies automatically when using CMake 2.8.11 or
  # later. Otherwise we have to add them here ourselves.
  if (CMAKE_VERSION VERSION_LESS 2.8.11)
    include_directories("${gtest_SOURCE_DIR}/include")
  endif()

  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../include")

  # source files
  file(GLOB_RECURSE TEST_CPP
      ${CMAKE_CURRENT_SOURCE_DIR}/t*.cpp
  )   

  # remove main.cpp
  list(FILTER SRC_CPP EXCLUDE REGEX ".*main.cpp$")

  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test/bin)
  # Link GTest library
  add_executable(unittest ${TEST_CPP} ${SRC_CPP})
  target_link_libraries(unittest gtest_main ${CLANG_LIBS})

  add_test(NAME unittest COMMAND unittest)

  # copy baseline and helper script into ${CMAKE_BINARY_DIR}
  file(GLOB_RECURSE BASELINES
      ${CMAKE_CURRENT_SOURCE_DIR}/*.gold
  )
  file(GLOB_RECURSE JSON
      ${CMAKE_CURRENT_SOURCE_DIR}/*.json
  )
  file(GLOB_RECURSE SRC
      ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
  )
  list(FILTER SRC EXCLUDE REGEX ^t)
  
  FOREACH(FILE ${BASELINES} ${JSON} ${SRC})
      file(RELATIVE_PATH RELPATH ${CMAKE_SOURCE_DIR} ${FILE})
      get_filename_component(RELDIR ${RELPATH} DIRECTORY)
      set(DIR ${CMAKE_BINARY_DIR}/${RELDIR})
      file(COPY ${FILE} DESTINATION ${DIR})
  ENDFOREACH(FILE)

  set(SCRIPT ${CMAKE_SOURCE_DIR}/scripts/update-compile-commands.py)
  file(COPY ${SCRIPT} DESTINATION ${CMAKE_BINARY_DIR}/scripts
       FILE_PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ)
                
endif(BUILD_TESTS)
