add_library(applications
    simple_application.cpp
    loop_application.cpp
)

target_include_directories(applications
PRIVATE
    ${PROJECT_SOURCE_DIR}/include/first_soc/applications
    ${PROJECT_SOURCE_DIR}/include/first_soc
    ${PROJECT_SOURCE_DIR}/include/shared
    ${PROJECT_SOURCE_DIR}/external/hestia/include
)

target_link_libraries(applications
PRIVATE
    hestia::toolbox::component
)

add_library(first_soc::applications ALIAS applications)