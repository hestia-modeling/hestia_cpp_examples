add_library(components
    functional_processor.cpp
    memory_bound_processor.cpp
    performant_processor.cpp
    pipelined_processor.cpp
)

target_include_directories(components
PRIVATE
    ${PROJECT_SOURCE_DIR}/include/first_soc/components
    ${PROJECT_SOURCE_DIR}/include/first_soc
    ${PROJECT_SOURCE_DIR}/include/shared
    ${PROJECT_SOURCE_DIR}/external/hestia/include
)

target_link_libraries(components
PRIVATE
    first_soc::functional
    hestia::component
    hestia::toolbox::component
)

add_library(first_soc::components ALIAS components)