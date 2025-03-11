cmake_minimum_required(VERSION 3.21)
message("Using toolchain file ${CMAKE_TOOLCHAIN_FILE}.")

########################################################################################################################
## Define project
########################################################################################################################
project(
        SkyrimOutfitSystemNG
        VERSION 1.0.0
        DESCRIPTION "Skyrim Outfit System for all version (SE/AE/VR)."
        LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

include(GNUInstallDirs)

configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.rc.in
        ${CMAKE_CURRENT_BINARY_DIR}/version.rc
        @ONLY)

################################################################################
# MSVC runtime library
################################################################################
string(CONCAT MSVC_RUNTIME_LIBRARY_STR
        $<$<CONFIG:Debug>:MultiThreadedDebugDLL>
        $<$<CONFIG:Release>:MultiThreadedDLL>)

################################################################################
# Target definition
################################################################################
set(headers
        include/version.h
        include/cobb/strings.h
        include/cobb/utf8naturalsort.h
        include/cobb/utf8string.h
        include/ArmorAddonOverrideService.h
        include/OutfitSystem.h
        include/PlayerSkinning.h
        include/RE/REAugments.h
)

set(sources
        src/Main.cpp
        src/main.cpp
        src/cobb/utf8naturalsort.cpp
        src/cobb/utf8string.cpp
        src/ArmorAddonOverrideService.cpp
        src/OutfitSystem.cpp
        src/hooking/Hooks_AE.cpp
        src/hooking/Hooks_PRE_AE.cpp
        src/hooking/Patches.cpp
        src/Utility.cpp
        src/RE/REAugments.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/version.rc)

source_group(
        TREE ${CMAKE_CURRENT_SOURCE_DIR}
        FILES
        ${headers}
        ${sources}
        ${tests})

###############################################################################
# Find Dependencies
###############################################################################
find_package(CommonLibSSE CONFIG REQUIRED)
find_package(span-lite REQUIRED CONFIG)
find_package(spdlog REQUIRED CONFIG)
find_package(xbyak REQUIRED CONFIG)
find_library(INIH_LIBRARY inih REQUIRED)
find_library(INIREADER_LIBRARY INIReader)  # Try to find the INIReader library
find_path(INIH_INCLUDE_DIRS "ini.h" REQUIRED)
find_path(INIREADER_INCLUDE_DIRS "INIReader.h" PATHS ${INIH_INCLUDE_DIRS} PATH_SUFFIXES "cpp")  # Look for INIReader.h
find_package(Protobuf REQUIRED)
message(STATUS "Found protoc at ${Protobuf_PROTOC_EXECUTABLE}")

###############################################################################
# Build Protocol Buffers
###############################################################################
protobuf_generate_cpp(
    PROTO_SRC PROTO_HDR
    src/protos/outfit.proto
)

add_library(ProtocolBuffers STATIC ${PROTO_SRC} ${PROTO_HDR})
target_link_libraries(ProtocolBuffers PUBLIC protobuf::libprotobuf)
target_include_directories(ProtocolBuffers PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
set_target_properties(ProtocolBuffers PROPERTIES MSVC_RUNTIME_LIBRARY ${MSVC_RUNTIME_LIBRARY_STR})

#########################################################################################################################
### Build options
#########################################################################################################################
message("Options:")
option(BUILD_TESTS "Build unit tests." OFF)
message("\tTests: ${BUILD_TESTS}")

########################################################################################################################
## Configure target DLL
########################################################################################################################
add_commonlibsse_plugin(${PROJECT_NAME} SOURCES ${headers} ${sources})
add_library("${PROJECT_NAME}::${PROJECT_NAME}" ALIAS "${PROJECT_NAME}")

# Configure includes
target_include_directories(${PROJECT_NAME}
        PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/src>
        $<INSTALL_INTERFACE:src>
        ${INIH_INCLUDE_DIRS})

target_include_directories(${PROJECT_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

# Configure compilation options
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        "/sdl"       # Enable Additional Security Checks
        "/utf-8"     # Set Source and Executable character sets to UTF-8
        "/Zi"        # Debug Information Format
        "/EHsc"      # C++ exception handling
        "/Zc:preprocessor"  # Enable preprocessor conformance mode
        "/experimental:external"
        "/external:anglebrackets"
        "/external:W0"
        "/wd4200"    # nonstandard extension used : zero-sized array in struct/union
    )

    target_link_options(${PROJECT_NAME} PRIVATE
        "$<$<CONFIG:DEBUG>:/INCREMENTAL;/OPT:NOREF;/OPT:NOICF>"
        "$<$<CONFIG:RELEASE>:/INCREMENTAL:NO;/OPT:REF;/OPT:ICF;/DEBUG:FULL>"
    )

    set_target_properties(${PROJECT_NAME} PROPERTIES MSVC_RUNTIME_LIBRARY ${MSVC_RUNTIME_LIBRARY_STR})
endif()

# Link with dependencies
target_link_libraries(${PROJECT_NAME} PUBLIC
    nonstd::span-lite
    spdlog::spdlog
    ${INIH_LIBRARY}
    ${INIREADER_LIBRARY} 
    ProtocolBuffers
    xbyak::xbyak
)

target_precompile_headers(${PROJECT_NAME}
        PRIVATE
        src/PCH.h
)

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/Sample"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

install(TARGETS ${PROJECT_NAME}
        DESTINATION "${CMAKE_INSTALL_LIBDIR}")

########################################################################################################################
## Configure unit tests
########################################################################################################################
# if(BUILD_TESTS)
#     find_package(Catch2 CONFIG REQUIRED)
# 
#     include(CTest)
#     include(Catch)
# 
#     add_executable(
#             ${PROJECT_NAME}Tests
#             ${headers}
#             ${tests})
# 
#     target_link_libraries(
#             ${PROJECT_NAME}Tests
#             PRIVATE
#             ${PROJECT_NAME}
#             Catch2::Catch2WithMain)
# 
#     target_precompile_headers(${PROJECT_NAME}Tests
#             PRIVATE
#             src/PCH.h)
# 
#     catch_discover_tests(${PROJECT_NAME}Tests)
#     add_test(NAME ${PROJECT_NAME}Tests COMMAND ${PROJECT_NAME}Tests)
# endif()

########################################################################################################################
## Automatic plugin deployment
########################################################################################################################
# Automatic deployment to FOMOD directory.
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(BUILD_NAME "Debug")
else()
    set(BUILD_NAME "Release")
endif()
install(DIRECTORY DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/contrib/Distribution/Papyrus${BUILD_NAME}/")
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_SOURCE_DIR}/contrib/Distribution/Plugin${BUILD_NAME}/")
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> "${CMAKE_CURRENT_SOURCE_DIR}/contrib/Distribution/Plugin${BUILD_NAME}/")
if(${CMAKE_BUILD_TYPE} STREQUAL Debug OR ${CMAKE_BUILD_TYPE} STREQUAL RelWithDebInfo)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PDB_FILE:${PROJECT_NAME}> "${CMAKE_CURRENT_SOURCE_DIR}/contrib/Distribution/Plugin${BUILD_NAME}/")
endif()
file(GLOB_RECURSE OUTPUT_DLLS "${CMAKE_CURRENT_SOURCE_DIR}/contrib/Distribution/**/*.dll")
file(GLOB_RECURSE OUTPUT_PDBS "${CMAKE_CURRENT_SOURCE_DIR}/contrib/Distribution/**/*.pdb")
file(GLOB_RECURSE OUTPUT_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/contrib/Distribution/**/*.pex")
set_property(TARGET ${PROJECT_NAME}
        APPEND PROPERTY ADDITIONAL_CLEAN_FILES "${OUTPUT_DLLS}")
set_property(TARGET ${PROJECT_NAME}
        APPEND PROPERTY ADDITIONAL_CLEAN_FILES "${OUTPUT_PDBS}")
set_property(TARGET ${PROJECT_NAME}
        APPEND PROPERTY ADDITIONAL_CLEAN_FILES "${OUTPUT_SCRIPTS}")

# Automatic deployment to Mod Organizer 2 mod directory.
foreach (DEPLOY_TARGET $ENV{SkyrimPluginTargets})
    message("Adding deployment target ${DEPLOY_TARGET}.")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> "${DEPLOY_TARGET}/SKSE/Plugins/")
    if(${CMAKE_BUILD_TYPE} STREQUAL Debug OR ${CMAKE_BUILD_TYPE} STREQUAL RelWithDebInfo)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PDB_FILE:${PROJECT_NAME}> "${DEPLOY_TARGET}/SKSE/Plugins/")
    endif()
endforeach()