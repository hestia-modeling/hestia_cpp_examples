find_package(PythonLibs 3.7 REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

add_library(basic_python SHARED main.cpp)

target_include_directories(basic_python
PRIVATE
    ${PROJECT_SOURCE_DIR}/include/basic
    ${PROJECT_SOURCE_DIR}/include/shared
    ${HESTIA_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
)

target_link_libraries(basic_python
PRIVATE
    ${HESTIA_LIBRARIES}
    ${PYTHON_LIBRARIES}
)

add_dependencies(basic_python hestia)
