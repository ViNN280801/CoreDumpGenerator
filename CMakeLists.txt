cmake_minimum_required(VERSION 3.16)
project(CoreDumpGeneratorExample)

if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 11)
endif()

set(CMAKE_CXX_STANDARD_REQUIRED ON)

message(STATUS "CMAKE_CXX_STANDARD: ${CMAKE_CXX_STANDARD}")
message(STATUS "CMAKE_CXX_STANDARD_REQUIRED: ${CMAKE_CXX_STANDARD_REQUIRED}")
message(STATUS "CMAKE_CXX_EXTENSIONS: ${CMAKE_CXX_EXTENSIONS}")
message(STATUS "CMAKE_CXX_COMPILER_ID: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "CMAKE_CXX_COMPILER_VERSION: ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER}")
message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
message(STATUS "CMAKE_CXX_FLAGS_DEBUG: ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "CMAKE_CXX_FLAGS_RELEASE: ${CMAKE_CXX_FLAGS_RELEASE}")
message(STATUS "CMAKE_EXE_LINKER_FLAGS: ${CMAKE_EXE_LINKER_FLAGS}")
message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message(STATUS "CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")
message(STATUS "CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")
message(STATUS "CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "CMAKE_CURRENT_BINARY_DIR: ${CMAKE_CURRENT_BINARY_DIR}")
message(STATUS "CMAKE_CURRENT_LIST_FILE: ${CMAKE_CURRENT_LIST_FILE}")
message(STATUS "CMAKE_CURRENT_LIST_DIR: ${CMAKE_CURRENT_LIST_DIR}")
message(STATUS "CMAKE_CURRENT_LIST_LINE: ${CMAKE_CURRENT_LIST_LINE}")
message(STATUS "CMAKE_CURRENT_LIST_DIR: ${CMAKE_CURRENT_LIST_DIR}")

if(MSVC)
    # Maximum debugging information and warnings
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /WX /EHsc /utf-8")

    # Debug configuration - maximum debug info
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od /Zi /DEBUG /JMC /bigobj")

    # Release configuration - optimized but with full debug info
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /Zi /DEBUG /JMC /bigobj /DNDEBUG")

    # Enable maximum PDB generation for all configurations
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zi /bigobj")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DEBUG /OPT:REF /OPT:ICF")

    # Additional flags for maximum debug information
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /FS") # Allow multiple cl.exe processes
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP") # Multi-processor compilation
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g -ggdb")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -g -ggdb")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g -ggdb")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -g -ggdb")
endif()

add_executable(${PROJECT_NAME}
    CoreDumpGenerator.cpp
    main.cpp
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME}
        dbghelp
        psapi
    )
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    PDB_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    COMPILE_PDB_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Ensure PDB files are generated for Windows
if(MSVC)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        COMPILE_PDB_NAME "${PROJECT_NAME}"
        COMPILE_PDB_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"

        # Force full debug information
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )

    # Set additional properties for maximum debug info
    set_target_properties(${PROJECT_NAME} PROPERTIES
        VS_DEBUGGER_COMMAND "${CMAKE_BINARY_DIR}/bin/${PROJECT_NAME}.exe"
        VS_DEBUGGER_COMMAND_ARGUMENTS ""
        VS_DEBUGGER_ENVIRONMENT ""
    )
endif()

# Copy PDB files to output directory on Windows
if(MSVC)
    # Copy PDB files to output directory
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_PDB_FILE:${PROJECT_NAME}>
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMENT "Copying PDB file to output directory"
    )

    # Create debug symbols directory for additional PDB management
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
        "${CMAKE_BINARY_DIR}/bin/symbols"
        COMMENT "Creating symbols directory"
    )

    # Copy PDB to symbols directory for better organization
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_PDB_FILE:${PROJECT_NAME}>
        "${CMAKE_BINARY_DIR}/bin/symbols/"
        COMMENT "Copying PDB file to symbols directory"
    )

    # Set additional linker flags for maximum debug information
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_FLAGS "/DEBUG:FULL /INCREMENTAL:NO /OPT:REF /OPT:ICF"
    )
endif()
