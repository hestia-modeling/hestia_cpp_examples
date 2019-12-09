add_subdirectory(applications)
add_subdirectory(components)
add_subdirectory(functional)


add_executable(first_soc main.cpp)


target_include_directories(first_soc
PRIVATE
    ${PROJECT_SOURCE_DIR}/include/first_soc
    ${PROJECT_SOURCE_DIR}/external/hestia/include
)

target_link_libraries(first_soc
PRIVATE
    first_soc::components
    first_soc::applications
    hestia::test_bench
)


find_package(PythonLibs 3.7 REQUIRED)


add_library(python_processor SHARED python_test_bench.cpp)

target_include_directories(python_processor
PRIVATE
    ${PROJECT_SOURCE_DIR}/include/first_soc
    ${PROJECT_SOURCE_DIR}/include/shared
    ${PROJECT_SOURCE_DIR}/external/hestia/include

    ${PYTHON_INCLUDE_DIRS}
)

target_link_libraries(python_processor
PRIVATE
    first_soc::components
    first_soc::applications
    hestia::test_bench
    ${PYTHON_LIBRARIES}
)
