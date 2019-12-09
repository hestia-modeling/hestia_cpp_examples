add_executable(basic main.cpp)

target_include_directories(basic
PRIVATE
    ${PROJECT_SOURCE_DIR}/include/basic
    ${PROJECT_SOURCE_DIR}/include/shared
    ${HESTIA_INCLUDE_DIR}
)

target_link_libraries(basic
PRIVATE
    ${HESTIA_LIBRARIES}
)

add_dependencies(basic hestia)
