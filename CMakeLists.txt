cmake_minimum_required(VERSION 3.21.1)

project(Scribe C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if("${DISABLE_INSTALL_MSG}" STREQUAL "true")
	message("-- Install messages are now lazily shown")
	set(CMAKE_INSTALL_MESSAGE LAZY)
endif()

if(DEFINED ENV{PREFIX_DIR} AND NOT "$ENV{PREFIX_DIR}" STREQUAL "")
	set(CMAKE_INSTALL_PREFIX "$ENV{PREFIX_DIR}")
else()
	set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/.scribe")
endif()
set(CMAKE_PREFIX_PATH "${CMAKE_INSTALL_PREFIX};${CMAKE_BINARY_DIR}")
add_definitions(-DINSTALL_PREFIX=${CMAKE_INSTALL_PREFIX})
message("-- Using PREFIX = ${CMAKE_INSTALL_PREFIX}")

# add cmake_modules path
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake_modules")

set(COMPILER_VERSION_MAJOR 0)
set(COMPILER_VERSION_MINOR 0)
set(COMPILER_VERSION_PATCH 1)

execute_process(COMMAND date "+%a %b %d, %Y at %H:%M:%S" OUTPUT_VARIABLE BUILD_DATE OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND git config --get remote.origin.url OUTPUT_VARIABLE REPO_URL OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND git log --format=%H -n 1 OUTPUT_VARIABLE COMMIT_ID OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND git diff --quiet RESULT_VARIABLE ret)
if(ret EQUAL 0)
	set(TREE_STATUS "Clean")
else()
	set(TREE_STATUS "Dirty")
endif()

set(ORIGIN "$ORIGIN")
if(APPLE)
	set(ORIGIN "@loader_path")
endif()
set(CMAKE_INSTALL_RPATH "${ORIGIN}/../lib/scribe;${ORIGIN}/.")
set(CMAKE_BUILD_WITH_INSTALL_NAME_DIR TRUE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_MACOSX_RPATH TRUE)

include(CheckCXXCompilerFlag)

# For libGMP on macOS and BSD
if(APPLE OR ${CMAKE_SYSTEM_NAME} MATCHES ".*BSD.*")
	include_directories(/usr/local/include)
endif()

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
if(CMAKE_USE_PTHREADS_INIT)
	message("-- Using thread flags: -pthread")
	set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -pthread")
	set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -pthread")
else()
	message("-- Using thread flags: ${CMAKE_THREAD_LIBS_INIT}")
	set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${CMAKE_THREAD_LIBS_INIT}")
	set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${CMAKE_THREAD_LIBS_INIT}")
endif()

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
    message("-- Using ccache")
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES ".*BSD.*")
	add_definitions(-D_WITH_GETLINE)
endif()

# Generally, disabled for CI purposes
# Also needed when running valgrind as valgrind does not support instruction bytes for Vector<bool>
if(NOT DEFINED ENV{DISABLE_MARCH_NATIVE} OR "$ENV{DISABLE_MARCH_NATIVE}" STREQUAL "")
        check_cxx_compiler_flag(-march=native COMPILER_SUPPORTS_MARCH_NATIVE)
        if(COMPILER_SUPPORTS_MARCH_NATIVE)
                set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
        endif()
endif()

# Force DWARF4 as valgrind does not support anything newer (clang-14 defaults to DWARF5).
if(NOT CMAKE_BUILD_TYPE STREQUAL "Release")
	check_cxx_compiler_flag(-gdwarf-4 COMPILER_SUPPORTS_GDWARF4)
	if(COMPILER_SUPPORTS_GDWARF4)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -gdwarf-4")
	endif()
endif()

# Because Android has issues with dlopen()'s RTLD_GLOBAL; See:
# https://android.googlesource.com/platform/bionic/+/master/android-changes-for-ndk-developers.md#changes-to-library-search-order
# https://android.googlesource.com/platform/bionic/+/30b17e32f0b403a97cef7c4d1fcab471fa316340/linker/linker_namespaces.cpp#100
# https://android-ndk.narkive.com/iNWj05IV/weak-symbol-linking-when-loading-dynamic-libraries
set(EXTRA_LD_FLAGS "")
if(ANDROID)
	set(EXTRA_LD_FLAGS "-z global")
endif()

if(CMAKE_COMPILER_IS_GNUCXX AND DEFINED WITH_COVERAGE)
    set(WITH_COVERAGE "-D_WITH_COVERAGE")
    include(CodeCoverage)
    APPEND_COVERAGE_COMPILER_FLAGS()
    setup_target_for_coverage_gcovr_html(NAME scribe_cov EXECUTABLE ${PROJECT_SOURCE_DIR}/codecov.sh ${PROJECT_SOURCE_DIR})
endif()

# Finally! The sources!!!!!

# Set Base.hpp.in template
configure_file("${PROJECT_SOURCE_DIR}/include/Core/Core.hpp.in" "${PROJECT_SOURCE_DIR}/include/Core/Core.hpp" @ONLY)

# Install CMake modules
# install(DIRECTORY "${CMAKE_SOURCE_DIR}/cmake_modules/"
# 	DESTINATION "${CMAKE_INSTALL_PREFIX}/cmake_modules"
# )

# Install C++ Includes
# install(DIRECTORY "${CMAKE_SOURCE_DIR}/include/"
# 	DESTINATION "${CMAKE_INSTALL_PREFIX}/include/scribe/compiler"
# 	FILES_MATCHING PATTERN "*.hpp"
# )

# Install Scribe Sources
install(DIRECTORY "${CMAKE_SOURCE_DIR}/lib/"
	DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/scribe"
	FILES_MATCHING PATTERN "*.sc"
)

# Core Library
file(GLOB_RECURSE COREINCS RELATIVE "${PROJECT_SOURCE_DIR}" "include/Core/*.hpp")
file(GLOB_RECURSE CORESRCS RELATIVE "${PROJECT_SOURCE_DIR}" "src/Core/*.cpp")
add_library(libcore SHARED ${CORESRCS} ${COREINCS})
target_include_directories(libcore PUBLIC "${PROJECT_SOURCE_DIR}/include/Core")
set_target_properties(libcore
    PROPERTIES
    PREFIX "" # 'lib' as prefix is not needed on linux since the name is libcore anyway
    LINK_FLAGS "${EXTRA_LD_FLAGS}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/scribe/" # for so (non-windows)
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<0:>" # for dll (windows)
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<0:>" # for lib (windows)
    INSTALL_RPATH_USE_LINK_PATH TRUE
)
install(TARGETS libcore
	LIBRARY
	  DESTINATION "lib/scribe/"
	RUNTIME
	  DESTINATION "bin/$<0:>"
	ARCHIVE
	  DESTINATION "bin/$<0:>"
)

# Scribe VM Library
file(GLOB_RECURSE VMINCS RELATIVE "${PROJECT_SOURCE_DIR}" "include/VM/*.hpp")
file(GLOB_RECURSE VMSRCS RELATIVE "${PROJECT_SOURCE_DIR}" "src/VM/*.cpp")
list(REMOVE_ITEM VMSRCS "src/VM/Main.cpp")
add_library(libvm SHARED ${VMSRCS} ${VMINCS})
target_include_directories(libvm PUBLIC "${PROJECT_SOURCE_DIR}/include/VM")
target_link_libraries(libvm libcore)
set_target_properties(libvm
    PROPERTIES
    PREFIX "" # 'lib' as prefix is not needed on linux since the name is libcore anyway
    LINK_FLAGS "${EXTRA_LD_FLAGS}"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib/scribe/" # for so (non-windows)
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<0:>" # for dll (windows)
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<0:>" # for lib (windows)
    INSTALL_RPATH_USE_LINK_PATH TRUE
)
install(TARGETS libvm
	LIBRARY
	  DESTINATION "lib/scribe/"
	RUNTIME
	  DESTINATION "bin/$<0:>"
	ARCHIVE
	  DESTINATION "bin/$<0:>"
)

# Scribe VM Binary
add_executable(scribevm "${PROJECT_SOURCE_DIR}/src/VM/Main.cpp" ${VMSRCS} ${VMINCS})
target_include_directories(scribevm PRIVATE "${PROJECT_SOURCE_DIR}/include/VM")
target_link_libraries(scribevm libvm)
set_target_properties(scribevm
	PROPERTIES
	LINK_FLAGS "${EXTRA_LD_FLAGS}"
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<0:>"
	INSTALL_RPATH_USE_LINK_PATH TRUE
)
install(TARGETS scribevm
	RUNTIME
	  DESTINATION bin
	  COMPONENT Binaries
)

# Scribe Compiler
file(GLOB_RECURSE COMPILERINCS RELATIVE "${PROJECT_SOURCE_DIR}" "include/Compiler/*.hpp")
file(GLOB_RECURSE COMPILERSRCS RELATIVE "${PROJECT_SOURCE_DIR}" "src/Compiler/*.cpp")
list(REMOVE_ITEM COMPILERSRCS "src/Compiler/Main.cpp")

add_executable(scribe "${PROJECT_SOURCE_DIR}/src/Compiler/Main.cpp" ${COMPILERSRCS} ${COMPILERINCS})
target_include_directories(scribe PRIVATE "${PROJECT_SOURCE_DIR}/include/Compiler")
target_link_libraries(scribe libvm)
set_target_properties(scribe
	PROPERTIES
	LINK_FLAGS "${EXTRA_LD_FLAGS}"
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<0:>"
	INSTALL_RPATH_USE_LINK_PATH TRUE
)
install(TARGETS scribe
	RUNTIME
	  DESTINATION bin
	  COMPONENT Binaries
)