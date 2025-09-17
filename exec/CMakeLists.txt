set(TARGET exec)

add_executable(${TARGET})
target_sources(${TARGET}
    PRIVATE
    src/main.cpp
)
target_compile_features(${TARGET}
    PRIVATE cxx_std_23
)
target_link_libraries(${TARGET}
    PRIVATE core
)
