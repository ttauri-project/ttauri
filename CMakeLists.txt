
cmake_minimum_required(VERSION 3.19)

#-------------------------------------------------------------------
# Configure early CMAKE paths
#-------------------------------------------------------------------
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/CMake")
include(SetupVcpkg)
include(SetupCompiler)

#-------------------------------------------------------------------
# Build Options
#-------------------------------------------------------------------

option(BUILD_SHARED_LIBS    "Build shared libraries"            OFF)
option(TT_BUILD_EXAMPLES    "Build example applications"         ON)
option(TT_BUILD_TESTS       "Build tests"                        ON)
option(TT_BUILD_PCH         "Build precompiled headers"          ON)
option(TT_INSTALL           "Generate installation target"       ON)
option(TT_ENABLE_ANALYSIS   "Compile using -analyze"            OFF)

#-------------------------------------------------------------------
# Project
#-------------------------------------------------------------------

if(APPLE)
    set(LANGUAGES CXX OBJCXX)
else()
    set(LANGUAGES CXX)
endif()

# vcpkg.json is the primary source for version data
file(READ ${CMAKE_SOURCE_DIR}/vcpkg.json VCPKG_JSON_STRING)
string(JSON TT_LIB_NAME     GET ${VCPKG_JSON_STRING} "name")
string(JSON TT_LIB_VERSION  GET ${VCPKG_JSON_STRING} "version")
string(JSON TT_LIB_LICENSE  GET ${VCPKG_JSON_STRING} "license")
string(JSON TT_LIB_DESC     GET ${VCPKG_JSON_STRING} "description")
string(JSON TT_LIB_HOMEPAGE GET ${VCPKG_JSON_STRING} "homepage")

configure_file("package.json.in" "package.json" @ONLY)
file(READ ${CMAKE_CURRENT_BINARY_DIR}/package.json PACKAGE_JSON_STRING)
string(JSON TT_LIB_DISPLAY_NAME GET ${PACKAGE_JSON_STRING} "display-name")
string(JSON TT_LIB_VENDOR GET ${PACKAGE_JSON_STRING} "vendor")

project(${TT_LIB_NAME} VERSION ${TT_LIB_VERSION} LANGUAGES ${LANGUAGES})

#-------------------------------------------------------------------
# Define Constants
#-------------------------------------------------------------------

set(TT_WIN32 0)
set(TT_MACOS 0)
set(TT_POSIX 0)
set(TT_X64 0)

if (APPLE)
    set(TT_MACOS 1)
    set(TT_POSIX 1)
elseif (WIN32)
    set(TT_WIN32 1)
endif()

set(x64_list x86 X86 amd64 AMD64)
if (${CMAKE_SYSTEM_PROCESSOR} IN_LIST x64_list)
    set(TT_X64 1)
endif()

#-------------------------------------------------------------------
# Precompiled Headers (PCH) Configuration
#-------------------------------------------------------------------

if(WIN32)
    if(${CMAKE_CXX_COMPILER_ID} MATCHES Clang)
        set(CMAKE_DISABLE_PRECOMPILE_HEADERS ON)
    endif()
else(APPLE)
    # Unable to use precompiled headers with objc right now
    set(CMAKE_DISABLE_PRECOMPILE_HEADERS ON)
endif()

#-------------------------------------------------------------------
# Setup CMake Includes
#-------------------------------------------------------------------

include(AddStaticResource)
include(AddShader)
include(AddStaticResource)
include(ShowBuildTargetProperties)
include(FetchContent)
if(WIN32)
    include(GetWindowsNtVersion)
endif()

#-------------------------------------------------------------------
# Find Dependencies
#-------------------------------------------------------------------

#
# GoogleTest - non-vcpkg, directly build from externals
#
if(TT_BUILD_TESTS)
    set(INSTALL_GTEST OFF CACHE INTERNAL "Don't install gtest")
    set(INSTALL_GMOCK OFF CACHE INTERNAL "Don't install gmock")
    FetchContent_Declare(googletest GIT_REPOSITORY https://github.com/google/googletest.git GIT_TAG release-1.11.0)
    FetchContent_MakeAvailable(googletest)
endif()

#
# Vulkan SDK Headers
#
if(NOT DEFINED ENV{VULKAN_SDK})
    message(FATAL_ERROR
        "Please set the VULKAN_SDK environment variable to the directory in which the SDK is installed.\n"
        "It looks like C:\VulkanSDK\1.0.0.0, but with the version number being the version that was installed.\n"
        "set VULKAN_SDK=\"C:\\VulkanSDK\\1.0.0.0\"\n")
endif()
find_package(Vulkan REQUIRED)

#
# Vulkan Memory Allocator
#
if(VCPKG_TOOLCHAIN)
    find_package(unofficial-vulkan-memory-allocator CONFIG REQUIRED)
else()
    FetchContent_Declare(VulkanMemoryAllocator GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator GIT_TAG v2.3.0)
    FetchContent_MakeAvailable(VulkanMemoryAllocator)
    add_library(unofficial::vulkan-memory-allocator::vulkan-memory-allocator INTERFACE IMPORTED)
    set_target_properties(
        unofficial::vulkan-memory-allocator::vulkan-memory-allocator
        PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${vulkanmemoryallocator_SOURCE_DIR}/src")
endif()


#-------------------------------------------------------------------
# Setup Build Targets
#-------------------------------------------------------------------

add_library(ttauri)
add_library(ttauri::ttauri ALIAS ttauri)

if(TT_BUILD_TESTS)
    add_executable(ttauri_tests)
endif()

#-------------------------------------------------------------------
# Setup Sources
#-------------------------------------------------------------------

# This needs to be below the build target commands (add_*).
# We omit adding source files to the `add_*` commands, but add them
# later using target_sources() in CMakeLists of subdirectories.

add_subdirectory(src/ttauri)
add_subdirectory(tools)

#-------------------------------------------------------------------
# Build Target: ttauri                                     (library)
#-------------------------------------------------------------------

set_target_properties(ttauri PROPERTIES VERSION ${TTAURI_PROJECT_VERSION_SHORT} SOVERSION ${TTAURI_MAJOR_VERSION})
set_target_properties(ttauri PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)

target_link_libraries(ttauri PUBLIC ${Vulkan_LIBRARIES})
target_link_libraries(ttauri PUBLIC unofficial::vulkan-memory-allocator::vulkan-memory-allocator)

if(APPLE)
    target_link_libraries(ttauri PUBLIC "-framework Foundation -framework AppKit")
endif()

target_include_directories(ttauri PUBLIC ${Vulkan_INCLUDE_DIRS})

target_include_directories(ttauri PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>"
    "$<INSTALL_INTERFACE:include>"
)
target_include_directories(ttauri PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(ttauri PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/src)

target_compile_features(ttauri INTERFACE cxx_std_20)

#
# We will support the following CPUs:
#   * Intel Ivy Bridge from 2012, still used by Mac Pro sold in 2019.
#   * AMD Steamroller from 2014
#
if (${CMAKE_CXX_COMPILER_ID} MATCHES Clang)
    # Set the execution character encoding to UTF-8
    target_compile_options(ttauri PUBLIC -fexec-charset=UTF-8)

    # Clang will complain about __builtin_assume even on constexpr functions and operators,
    # this warning is absolutely useless.
    target_compile_options(ttauri PUBLIC -Wno-assume)

    # Clang will complain about calling hidden-friend-template-functions.
    target_compile_options(ttauri PUBLIC -Wno-unknown-warning-option -Wno-c++20-extensions -Wno-c++2a-extensions)

    # vk_mem_alloc.h seems to turn on nullability completeness checks on clang.
    # It seems this check should only be used for interopability with swift
    target_compile_options(ttauri PUBLIC -Wno-nullability-completeness)

    # IvyBridge:                     F16C             FSGSBASE, AVX,           AES, PCLMUL        MMX, SSE, SSE2, SSE3,        SSSE3, SSE4.1, SSE4.2,     POPCNT, RDRND
    # Steamroller 0x15-v3: BMI, TBM, F16C, FMA, FMA4, FSGSBASE, AVX, XOP, LWP, AES, PCLMUL, CX16, MMX, SSE, SSE2, SSE3, SSE4A, SSSE3, SSE4.1, SSE4.2, ABM
    # POPCNT is supported by Steamroller through he SSE4A instruction set.
    target_compile_options(ttauri PUBLIC -mf16c -mfsgsbase -mavx -maes -mpclmul -mmmx -msse -msse2 -msse3 -mssse3 -msse4.1 -msse4.2 -mpopcnt)

    # The Microsoft version of clang does not implement all clang command line arguments.
    if (NOT MSVC)
        # The ttauri generic tokenizer uses large constexpr tables.
        target_compile_options(ttauri PUBLIC -fconstexpr-steps=100000000)

        # Tune according to a more modern CPU.
        target_compile_options(ttauri PUBLIC -mtune=skylake)
    endif()

elseif (MSVC)
    # Turn on a lot of warnings by default.
    target_compile_options(ttauri PUBLIC -W4)

    # Set the compiler to C++20 standard conforming as much as possible.
    target_compile_options(ttauri PUBLIC -permissive-)

    # suppress msbuild startup banner
    target_compile_options(ttauri PUBLIC -nologo)

    # By default MSVC reports a incorrect version in __cplusplus
    # This option will make it work correctly.
    target_compile_options(ttauri PUBLIC -Zc:__cplusplus)

    # Use the C++20 compliant preprocessor, which include __VA_OPT__()
    # This causes warning "C5105: macro expansion producing 'defined' has undefined behavior" in winbase.h
    # which is supposed to be fixed in a newer version of the SDK.
    target_compile_options(ttauri PUBLIC -Zc:preprocessor)
    target_compile_options(ttauri PUBLIC -wd5105)

    # Set the execution and source character encoding to UTF-8
    target_compile_options(ttauri PUBLIC -utf-8)

    # Support Intel's SkyLake and AMD's Excavator.
    # see hardware_support.md
    target_compile_options(ttauri PUBLIC -arch:AVX)

    # The ttauri generic tokenizer uses large constexpr tables.
    target_compile_options(ttauri PUBLIC -constexpr:steps 100000000)

    # C4068: unknown pragma.
    # Ignore unknown pragmas, needed for other compilers.
    target_compile_options(ttauri PUBLIC -wd4068)

    # C4324: structure was padded due to alignment specifier.
    # Of course it is, that is what the alignment specifier is used for??????
    target_compile_options(ttauri PUBLIC -wd4324)

    # C4100: unreferenced formal parameter.
    # This warning is in the way when you want to document a parameter of a virtual function
    # with an implementation that does not use the parameter.
    target_compile_options(ttauri PUBLIC -wd4100)

    # C4127: conditional expression is constant.
    # False positive with expressions that include template parameters.
    target_compile_options(ttauri PUBLIC -wd4127)

    # C6326: Potential comparison of a constant with another constant.
    # False positive in constexpr context.
    target_compile_options(ttauri PUBLIC -wd6326)

    # C6239: (<non-zero constant> && <expression>) always evaluates to the result of <expression>. Did you intend to use the bitwise-and operator?"
    # False positive when using logical operators in constexpr context.
    target_compile_options(ttauri PUBLIC -wd6239)

    # C6262: Function uses 'x' bytes of stack: exceeds /analyze:stacksize 'x'. Consider moving some data to heap.
    # False positives when returning data from a function in constexpr context.
    target_compile_options(ttauri PUBLIC -wd6262)

    # C4505: unreferenced local function has been removed.
    # False positive when calling a local function from within a "if constexpr" block.
    target_compile_options(ttauri PUBLIC -wd4505)

    # C4648: standard attribute 'no_unique_address' is ignored
    target_compile_options(ttauri PUBLIC -wd4648)

    if (NOT ${CMAKE_BUILD_TYPE} MATCHES "Release")
        # Enable Security Development Lifecycle checks.
        # This includes run-time checks, don't include this in the Release type
        # Do include it with ReleaseWithDebug.

        # -sdl forces deprecated warnings as errors so also not useful during development.
        #target_compile_options(ttauri PUBLIC -sdl)

        # Just my code enables the VS debugger to step over system, framework, library, and other non-user calls.
        target_compile_options(ttauri PUBLIC -JMC)
    endif()
endif()

if (WIN32)
    if(TT_BUILD_PCH)
        if(NOT TT_ENABLE_ANALYSIS)
            target_precompile_headers(ttauri PRIVATE <Windows.h>)
        endif()
    endif()

    # Add vulkan win32 surface support.
    target_compile_definitions(ttauri PUBLIC -DVK_USE_PLATFORM_WIN32_KHR)

    # Set defines to compile a win32 application.
	target_compile_options(ttauri PUBLIC -D_WIN64 -DUNICODE -D_UNICODE -D_CRT_SECURE_NO_WARNINGS)

    # winrt will include experimental headers that can not be turned off.
    #target_compile_options(ttauri PUBLIC -D_SILENCE_CLANG_COROUTINE_MESSAGE)

    # Make sure that Windows.h doesn't include all kind of weird things.
    target_compile_options(ttauri PUBLIC -DNOMINMAX -DWIN32_NO_STATUS)

    # define the version of the windows header files to use
    get_windows_ntversion(winnt_version)
    target_compile_options(ttauri PUBLIC -D_WIN32_WINNT=${winnt_version})

elseif (APPLE)
    # Add vulkan apple's Metal surface support.
    target_compile_definitions(ttauri PUBLIC -DVK_USE_PLATFORM_METAL_EXT)
endif()

#-------------------------------------------------------------------
# Copy resources needed by executables
#-------------------------------------------------------------------

add_shader(ttauri_shader_objects
    src/ttauri/GFX/utils.glsl
    src/ttauri/GFX/pipeline_image.vert
    src/ttauri/GFX/pipeline_image.frag
    src/ttauri/GFX/pipeline_flat.vert
    src/ttauri/GFX/pipeline_flat.frag
    src/ttauri/GFX/pipeline_box.vert
    src/ttauri/GFX/pipeline_box.frag
    src/ttauri/GFX/pipeline_SDF.vert
    src/ttauri/GFX/pipeline_SDF.frag
    src/ttauri/GFX/pipeline_tone_mapper.vert
    src/ttauri/GFX/pipeline_tone_mapper.frag
)

target_static_resource(ttauri
    ${ttauri_shader_objects}
    data/elusiveicons-webfont.ttf
    data/ttauri_icons.ttf
)

add_custom_target(ttauri_resources
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/resources ${CMAKE_CURRENT_BINARY_DIR}/resources
    DEPENDS
    resources/themes/ttauri_dark.theme.json
    resources/themes/ttauri_light.theme.json
    resources/win32.keybinds.json
)

add_dependencies(ttauri ttauri_resources)

#-------------------------------------------------------------------
# Installation Rules: ttauri                               (library)
#-------------------------------------------------------------------

include(GNUInstallDirs)

install(TARGETS ttauri RESOURCE)

# Copy all header files for the library.
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.hpp")
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h")

# Copy all resource files needing to be added to the resource directory of an application
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resources/
        DESTINATION share/ttauri/resources)

# On Windows, the dynamic libs go in the same dir as the executable and static libs go into "lib".
get_target_property(target_type ttauri TYPE)
if(WIN32 AND target_type STREQUAL DYNAMIC_LIBRARY)
    # Install the library and headers.
    install(TARGETS ttauri EXPORT ttauri
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_BINDIR} # <- .dll next to exe
    )
else()
    # Install the library and headers.
    install(TARGETS ttauri EXPORT ttauri
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} # <- .lib
    )
endif()

#export(TARGETS
#        ttauri
#    NAMESPACE ttauri::
#    FILE "${CMAKE_CURRENT_BINARY_DIR}/ttauri-config.cmake"
#)
install(
    EXPORT ttauri
    DESTINATION share/ttauri
    NAMESPACE ttauri::
    FILE "ttauriTargets.cmake"
)

include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/CMake/ttauriConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/ttauriConfig.cmake"
    INSTALL_DESTINATION lib/cmake/ttauri
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/ttauriConfigVersion.cmake"
    COMPATIBILITY SameMajorVersion
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/ttauriConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/ttauriConfigVersion.cmake"
    DESTINATION share/ttauri
)
#-------------------------------------------------------------------
# Build Target: ttauri_tests                            (executable)
#-------------------------------------------------------------------

if(TT_BUILD_TESTS)

    enable_testing()

    target_link_libraries(ttauri_tests PRIVATE gtest_main ttauri)

    target_include_directories(ttauri_tests PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

    if(TT_BUILD_PCH)
        if(NOT TT_ENABLE_ANALYSIS)
            target_precompile_headers(ttauri_tests PRIVATE <gtest/gtest.h>)
        endif()
    endif()

    include(GoogleTest)

    # PRE_TEST delays test discovery until just prior to test execution
    gtest_discover_tests(ttauri_tests DISCOVERY_MODE PRE_TEST)

    add_custom_command(
        TARGET ttauri_tests PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/data
            ${CMAKE_CURRENT_BINARY_DIR}
    )

endif()

#-------------------------------------------------------------------
# Installation Rules: ttauri_tests                      (executable)
#-------------------------------------------------------------------
if(TT_BUILD_TESTS)
    install(TARGETS ttauri_tests DESTINATION tests)
    install(DIRECTORY tests/data/ DESTINATION tests)
    if (BUILD_SHARED_LIBS)
        x_vcpkg_install_local_dependencies(TARGETS ttauri_tests DESTINATION tests)
    endif()
endif()

#-------------------------------------------------------------------
# Build examples
#-------------------------------------------------------------------
if(TT_BUILD_EXAMPLES)
    add_subdirectory(examples/ttauri_demo)
    add_subdirectory(examples/widgets)
endif()

#-------------------------------------------------------------------
# Remove certain files from static analysis
#-------------------------------------------------------------------

if(TT_ENABLE_ANALYSIS)
    if(MSVC)
        set(ttauri_do_not_analyze
            src/ttauri/GUI/pipeline_flat.cpp
            src/ttauri/GUI/gui_surface_vulkan.cpp
            src/ttauri/GUI/gui_device_vulkan.cpp
            src/ttauri/GUI/gui_system_vulkan.cpp
            src/ttauri/GUI/gui_system.cpp
            src/ttauri/GUI/gui_system_vulkan_win32.cpp
            src/ttauri/GUI/gui_window.cpp
            src/ttauri/GUI/gui_window_win32.cpp
            src/ttauri/GUI/pipeline_flat_device_shared.cpp
            src/ttauri/GUI/pipeline_image.cpp
            src/ttauri/GUI/pipeline_tone_mapper.cpp
            src/ttauri/GUI/pipeline_image_texture_map.cpp
            src/ttauri/GUI/pipeline_box_device_shared.cpp
            src/ttauri/GUI/pipeline_image_device_shared.cpp
            src/ttauri/GUI/pipeline_tone_mapper_device_shared.cpp
            src/ttauri/GUI/pipeline_image_image.cpp
            src/ttauri/GUI/pipeline_SDF_texture_map.cpp
            src/ttauri/GUI/pipeline_SDF_device_shared.cpp
            src/ttauri/GUI/pipeline_SDF_atlas_rect.cpp
            src/ttauri/GUI/pipeline_SDF.cpp
            src/ttauri/GUI/pipeline_vulkan.cpp
            src/ttauri/GUI/pipeline_box.cpp
            src/ttauri/widgets/WindowWidget.cpp
            src/ttauri/widgets/SystemMenuWidget.cpp
            src/ttauri/widgets/grid_layout_widget.cpp
            src/ttauri/widgets/WindowTrafficLightsWidget.cpp
            src/ttauri/widgets/LineInputWidget.cpp
            src/ttauri/widgets/widget.cpp
            src/ttauri/stencils/glyph_stencil.cpp
            src/ttauri/stencils/pixel_map_stencil.cpp
            src/ttauri/stencils/text_stencil.cpp
            src/ttauri/stencils/stencil.cpp
            src/ttauri/application.cpp
        )
        set_source_files_properties(${ttauri_do_not_analyze} PROPERTIES COMPILE_FLAGS /analyze-)
        target_compile_options(ttauri PUBLIC -analyze)
        #target_compile_options(ttauri PUBLIC /analyze:plugin EspXEngine.dll)
   endif()
endif()

#-------------------------------------------------------------------
# Display Compiler and Linker properties of Build Targets
#-------------------------------------------------------------------

show_build_target_properties(ttauri)

if(TT_BUILD_EXAMPLES)
    show_build_target_properties(ttauri_demo)
endif()

if(TT_BUILD_TESTS)
    show_build_target_properties(ttauri_tests)
endif()

#-------------------------------------------------------------------
# Build Documentation
#-------------------------------------------------------------------

#
# Doxygen
#
find_package(Doxygen)

if(DOXYGEN_FOUND)
    set(DOXYGEN_EXCLUDE_PATTERNS *_tests.cpp)
    set(DOXYGEN_GENERATE_HTML YES)
    set(DOXYGEN_GENERATE_LATEX NO)
    set(DOXYGEN_QUIET YES)
    set(DOXYGEN_WARN_IF_UNDOCUMENTED NO)
    set(DOXYGEN_WARN_NO_PARAMDOC NO)
    set(DOXYGEN_STRIP_FROM_PATH src)
    set(DOXYGEN_STRIP_FROM_INC_PATH src)

    set(DOXYGEN_JAVADOC_AUTOBRIEF YES)
    set(DOXYGEN_ALWAYS_DETAILED_SEC YES)
    set(DOXYGEN_DISTRIBUTE_GROUP_DOC YES)

    set(DOXYGEN_OPTIMIZE_OUTPUT_FOR_C YES)
    set(DOXYGEN_BUILTIN_STL_SUPPORT YES)
    #set(DOXYGEN_CLANG_ASSISTED_PARSING YES)
    #set(DOXYGEN_CLANG_OPTIONS "-std:c++20")

    set(DOXYGEN_TAGFILES "${CMAKE_SOURCE_DIR}/docs/media/style/cppreference-doxygen-web.tag.xml=http://en.cppreference.com/w/")
    set(DOXYGEN_IMAGE_PATH "${CMAKE_SOURCE_DIR}/src/" "${CMAKE_SOURCE_DIR}/docs/")
    set(DOXYGEN_EXAMPLE_PATH "${CMAKE_SOURCE_DIR}/examples/")

    set(DOXYGEN_LAYOUT_FILE "${CMAKE_SOURCE_DIR}/docs/media/style/DoxygenLayout.xml")
    set(DOXYGEN_HTML_COLORSTYLE_HUE 24)
    set(DOXYGEN_HTML_COLORSTYLE_SAT 150)
    set(DOXYGEN_HTML_COLORSTYLE_GAMMA 80)
    set(DOXYGEN_HTML_EXTRA_STYLESHEET "${CMAKE_SOURCE_DIR}/docs/media/style/customdoxygen.css")


    # The following 4 settings are to get protected members to behave as private.
    set(DOXYGEN_ENABLE_PREPROCESSING YES)
    set(DOXYGEN_MACRO_EXPANSION YES)
    set(DOXYGEN_EXPAND_ONLY_PREDEF YES)
    set(DOXYGEN_PREDEFINED "protected=private")

    doxygen_add_docs(doc src/ttauri docs)
else()
    message("Please install Doxygen to generate the documentation.")
endif()
