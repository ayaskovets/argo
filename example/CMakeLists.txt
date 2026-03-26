# TODO: reimpl that to a separate project
set(TARGET example)

add_executable(example)
target_sources(example
    PRIVATE
    main.cpp
)
target_compile_features(example
    PRIVATE cxx_std_23
)
target_link_libraries(example
    PRIVATE wip
)
