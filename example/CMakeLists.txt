set(TARGET app)

add_executable(${TARGET})
target_sources(${TARGET}
    PRIVATE
    main.cpp
)
target_compile_features(${TARGET}
    PRIVATE cxx_std_23
)
target_link_libraries(${TARGET}
    PRIVATE sandbox
)
