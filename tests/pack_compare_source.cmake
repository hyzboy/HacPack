if(NOT DEFINED HACPACK_EXE)
    message(FATAL_ERROR "HACPACK_EXE is not defined")
endif()

if(NOT DEFINED SOURCE_DIR)
    message(FATAL_ERROR "SOURCE_DIR is not defined")
endif()

if(NOT DEFINED OUTPUT_PACK)
    message(FATAL_ERROR "OUTPUT_PACK is not defined")
endif()

if(NOT DEFINED BUILD_DIR)
    message(FATAL_ERROR "BUILD_DIR is not defined")
endif()

set(STAGE_DIR "${BUILD_DIR}/hacpack_test_source")
file(REMOVE_RECURSE "${STAGE_DIR}")
file(MAKE_DIRECTORY "${STAGE_DIR}")

set(EXCLUDE_PATTERNS
    "^build/"
    "^bin/"
    "^CMakeFiles/"
    "^Debug/"
    "^Release/"
    "^RelWithDebInfo/"
    "^MinSizeRel/"
    "^Testing/"
    "^x64/"
    ".*\\.dir/"
)

set(INCLUDE_PATTERNS
    ".*\\.h$"
    ".*\\.cpp$"
    ".*\\.cppm$"
    ".*\\.cmake$"
    "(^|.*/)CMakeLists\\.txt$"
    ".*\\.md$"
    ".*\\.txt$"
    ".*\\.sln$"
    ".*\\.vcxproj$"
    ".*\\.vcxproj\\.filters$"
    ".*\\.vcxproj\\.user$"
)

file(GLOB_RECURSE ALL_FILES RELATIVE "${SOURCE_DIR}" "${SOURCE_DIR}/*")
foreach(REL_PATH IN LISTS ALL_FILES)
    if(IS_DIRECTORY "${SOURCE_DIR}/${REL_PATH}")
        continue()
    endif()

    set(SKIP_FILE FALSE)
    foreach(EX_RE IN LISTS EXCLUDE_PATTERNS)
        if(REL_PATH MATCHES "${EX_RE}")
            set(SKIP_FILE TRUE)
            break()
        endif()
    endforeach()
    if(SKIP_FILE)
        continue()
    endif()

    set(INCLUDE_FILE FALSE)
    foreach(IN_RE IN LISTS INCLUDE_PATTERNS)
        if(REL_PATH MATCHES "${IN_RE}")
            set(INCLUDE_FILE TRUE)
            break()
        endif()
    endforeach()
    if(NOT INCLUDE_FILE)
        continue()
    endif()

    get_filename_component(REL_DIR "${REL_PATH}" DIRECTORY)
    if(NOT REL_DIR STREQUAL "")
        file(MAKE_DIRECTORY "${STAGE_DIR}/${REL_DIR}")
    endif()
    configure_file("${SOURCE_DIR}/${REL_PATH}" "${STAGE_DIR}/${REL_PATH}" COPYONLY)
endforeach()

execute_process(
    COMMAND "${HACPACK_EXE}" pack "${STAGE_DIR}" "${OUTPUT_PACK}"
    RESULT_VARIABLE PACK_RESULT
    OUTPUT_VARIABLE PACK_STDOUT
    ERROR_VARIABLE PACK_STDERR
)

if(NOT PACK_RESULT EQUAL 0)
    message(FATAL_ERROR "Pack step failed (code=${PACK_RESULT})\nSTDOUT:\n${PACK_STDOUT}\nSTDERR:\n${PACK_STDERR}")
endif()

execute_process(
    COMMAND "${HACPACK_EXE}" compare "${OUTPUT_PACK}" "${STAGE_DIR}"
    RESULT_VARIABLE COMPARE_RESULT
    OUTPUT_VARIABLE COMPARE_STDOUT
    ERROR_VARIABLE COMPARE_STDERR
)

if(NOT COMPARE_RESULT EQUAL 0)
    message(FATAL_ERROR "Compare step failed (code=${COMPARE_RESULT})\nSTDOUT:\n${COMPARE_STDOUT}\nSTDERR:\n${COMPARE_STDERR}")
endif()

message(STATUS "HacPack source pack/compare test passed")

