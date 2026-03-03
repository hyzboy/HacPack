cmake_minimum_required(VERSION 3.10)

# Project
project(HacPack)

include(CTest)

# Option: use static C runtime
option(USE_STATIC_CRT "Use static C runtime (MSVC: /MT, others: -static-libgcc -static-libstdc++)" ON)

# Require C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Prefer static CRT on MSVC: replace /MD with /MT in flags when requested
if(MSVC)
    if(USE_STATIC_CRT)
        message(STATUS "Configuring MSVC to use static runtime (/MT)")
        foreach(flag_var
            CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE CMAKE_C_FLAGS_RELWITHDEBINFO CMAKE_C_FLAGS_MINSIZEREL
            CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_RELWITHDEBINFO CMAKE_CXX_FLAGS_MINSIZEREL)
            if(DEFINED ${flag_var})
                string(REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
            endif()
        endforeach()
    else()
        message(STATUS "Using dynamic C runtime on MSVC (/MD)")
    endif()
else()
    # Non-MSVC: if requested, instruct linker to use static libgcc/libstdc++ where possible
    if(USE_STATIC_CRT)
        message(STATUS "Requesting static libgcc/libstdc++ on non-MSVC toolchains")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libgcc -static-libstdc++")
        set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
    else()
        message(STATUS "Using dynamic runtime on non-MSVC toolchains")
    endif()
endif()

# Core writer library (no file-list-reader, no UTF conversions)
set(HACPACK_WRITER_SOURCES
    hacpack_builder.cpp
    hacpack_builder.h
    hacpack_builder_file.cpp
    hacpack_builder_file.h
    hacpack_writer_vector.cpp
    hacpack_writer_file.cpp
)
add_library(hacpack_writer STATIC ${HACPACK_WRITER_SOURCES})

# Export include directory for dependent projects
target_include_directories(hacpack_writer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# UTF/encoding support
set(HACPACK_UTF_SOURCES
    utf8_to_utf16.cpp
    utf16_to_utf8.cpp
    utf32_to_utf8.cpp
    utf_conv.h
    encoding.h
)
if(WIN32)
    list(APPEND HACPACK_UTF_SOURCES encoding_windows.cpp)
else()
    list(APPEND HACPACK_UTF_SOURCES encoding_posix.cpp)
endif()
add_library(hacpack_utf STATIC ${HACPACK_UTF_SOURCES})

# Reader/index support
set(HACPACK_READER_SOURCES
    pack_reader_index.cpp
    pack_reader.h
    pack_reader_io.h
    pack_reader_io.cpp
)
add_library(hacpack_reader STATIC ${HACPACK_READER_SOURCES})

# Reader depends on utf library
target_link_libraries(hacpack_reader PRIVATE hacpack_utf)

# Executable: CLI orchestration + core + plugins + dir scan
add_executable(${PROJECT_NAME}
    main.cpp
    dir_scan.cpp
    hacpack_core.cpp
    plugins/plugin_registry_writer.cpp
    plugins/plugin_registry_reader.cpp
    plugins/namelist_plugin_writer.cpp
    plugins/namelist_plugin_reader.cpp
    plugins/crc32_plugin_writer.cpp
    plugins/crc32_plugin_reader.cpp
)

# Link libraries to executable
target_link_libraries(${PROJECT_NAME} PRIVATE hacpack_writer hacpack_reader hacpack_utf)

# Compiler warnings/options
if(MSVC)
    target_compile_options(hacpack_writer PRIVATE /W4)
    target_compile_options(hacpack_reader PRIVATE /W4)
    target_compile_options(hacpack_utf PRIVATE /W4)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(hacpack_writer PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(hacpack_reader PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(hacpack_utf PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
    # link with iconv on posix if available (needed by utf conversions)
    find_library(ICONV_LIB NAMES iconv)
    if(ICONV_LIB)
        target_link_libraries(hacpack_utf PRIVATE ${ICONV_LIB})
    endif()
endif()

# Set executable output directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

if(BUILD_TESTING)
    add_test(
        NAME hacpack_pack_compare_source
        COMMAND ${CMAKE_COMMAND}
            -DHACPACK_EXE=$<TARGET_FILE:${PROJECT_NAME}>
            -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
            -DBUILD_DIR=${CMAKE_BINARY_DIR}
            -DOUTPUT_PACK=${CMAKE_BINARY_DIR}/../hacpack_source_test.hac
            -P ${CMAKE_SOURCE_DIR}/tests/pack_compare_source.cmake
    )

    add_test(
        NAME hacpack_pack_unpack_compare_source
        COMMAND ${CMAKE_COMMAND}
            -DHACPACK_EXE=$<TARGET_FILE:${PROJECT_NAME}>
            -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
            -DBUILD_DIR=${CMAKE_BINARY_DIR}
            -DOUTPUT_PACK=${CMAKE_BINARY_DIR}/../hacpack_roundtrip_test.hac
            -P ${CMAKE_SOURCE_DIR}/tests/pack_unpack_compare_source.cmake
    )
endif()

