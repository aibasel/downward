cmake_minimum_required(VERSION 3.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-O3")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

project(pydownward)

include_directories("${CMAKE_SOURCE_DIR}/src/search/")
include_directories("${CMAKE_SOURCE_DIR}/src/search/ext/")
include_directories("${CMAKE_SOURCE_DIR}/python")


file (GLOB SOURCE_FILES "src/search/*.cc" "src/search/*/*.cc")
file (GLOB HEADER_FILES "src/search/*.h*" "src/search/*/*.h*")
file (GLOB PYTHON_FILES "python/*.cc" "python/*.h")

# Set up such that XCode organizes the files
#source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_FILES} ${HEADER_FILES} ${PYTHON_FILES} )

find_package(pybind11 REQUIRED)
pybind11_add_module(pydownward 
	${SOURCE_FILES}
	${HEADER_FILES}
	${PYTHON_FILES}
)

target_link_libraries(pydownward PUBLIC)

install(TARGETS pydownward
  COMPONENT python
  LIBRARY DESTINATION "${PYTHON_LIBRARY_DIR}"
  )
