project (rampage)
cmake_minimum_required (VERSION 2.8)
set (RAMPAGE_VERSION "0.5")
set (RAMPAGE_SOVERSION 0.5)

set(RAMPAGE_DISPLAY_VERSION "0.5")

# Create pkg-conf file.
if(NOT DEFINED CMAKE_INSTALL_LIBDIR)
  set(CMAKE_INSTALL_LIBDIR lib)
endif(NOT DEFINED CMAKE_INSTALL_LIBDIR)
set(prefix      ${CMAKE_INSTALL_PREFIX})
set(exec_prefix ${CMAKE_INSTALL_PREFIX})
set(libdir      ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR})
set(VERSION     ${RAMPAGE_DISPLAY_VERSION})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/rampage.pc.in
   ${CMAKE_CURRENT_BINARY_DIR}/rampage.pc @ONLY
)

# Add the include directories
include_directories("src")

# Add source files
file(GLOB_RECURSE RAMPAGE_SRC ${PROJECT_SOURCE_DIR}/src/*.c)
# find_package(PythonLibs REQUIRED)
# include_directories(${PYTHON_INCLUDE_DIRS})

add_library(rampage SHARED 
	${RAMPAGE_SRC} 
	# rampage.def
)

set_target_properties(rampage PROPERTIES PUBLIC_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/src/rampage.h)

set_target_properties(rampage PROPERTIES
	VERSION ${RAMPAGE_VERSION}
	SOVERSION ${RAMPAGE_SOVERSION}
)

install(TARGETS rampage
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	PUBLIC_HEADER DESTINATION "/usr/local/include/" COMPONENT dev)
