add_library(functional
    functional_processor_library.cpp
    transactions/instruction.cpp
    transactions/isa.cpp
)

target_include_directories(functional
PRIVATE
    ${PROJECT_SOURCE_DIR}/include/first_soc/functional
    ${PROJECT_SOURCE_DIR}/external/hestia/include
)

add_library(first_soc::functional ALIAS functional)

