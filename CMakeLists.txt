cmake_minimum_required(VERSION 3.29.2)

project(sandbox
    LANGUAGES CXX
)

include(CTest)

# add_subdirectory(app)
# add_subdirectory(benchmarks)
add_subdirectory(src)
# add_subdirectory(tests)

include(CPack)
