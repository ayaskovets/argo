set(TARGET wip)

find_package(Boost REQUIRED)

add_library(${TARGET} STATIC)
target_sources(${TARGET}
    PRIVATE
    async/executors/current_executor.cpp
    datetime/iso8601.cpp
    logging/logger.cpp
    logging/renderer.cpp
    logging/writer.cpp
)
target_compile_features(${TARGET}
    PRIVATE cxx_std_23
)
target_include_directories(${TARGET}
    PUBLIC ${PROJECT_SOURCE_DIR}/include
    PUBLIC ${Boost_INCLUDE_DIRS}
)
target_link_libraries(${TARGET}
    PUBLIC ${Boost_LIBRARIES}
)
