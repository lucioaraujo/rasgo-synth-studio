cmake_minimum_required(VERSION 3.22)
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/src/AppVersion.h" studio_version_line REGEX "inline constexpr char appVersion")
string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+" studio_version "${studio_version_line}")
if(NOT studio_version)
    message(FATAL_ERROR "Missing Studio version in AppVersion.h")
endif()
project(RasgoSynthStudio VERSION "${studio_version}" LANGUAGES C CXX)

# Local dependency only: no FetchContent, sibling instrument or implicit download.
set(RASGO_JUCE_ROOT "" CACHE PATH "Clean JUCE checkout at the Studio dependency lock")
set(RASGO_JUCE_COMMIT "f8f8864172464b9adf9eba6101e1f784838d1597")
if(NOT EXISTS "${RASGO_JUCE_ROOT}/CMakeLists.txt")
    message(FATAL_ERROR "Set RASGO_JUCE_ROOT to the locked JUCE checkout; no download performed")
endif()
find_package(Git REQUIRED)
execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${RASGO_JUCE_ROOT}" rev-parse HEAD
    OUTPUT_VARIABLE juce_revision OUTPUT_STRIP_TRAILING_WHITESPACE RESULT_VARIABLE revision_result)
execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${RASGO_JUCE_ROOT}" status --porcelain
    OUTPUT_VARIABLE juce_changes RESULT_VARIABLE status_result)
if(NOT revision_result EQUAL 0 OR NOT status_result EQUAL 0 OR
   NOT juce_revision STREQUAL RASGO_JUCE_COMMIT OR NOT juce_changes STREQUAL "")
    message(FATAL_ERROR "JUCE must be clean and match DEPENDENCIES.md; no pin substitution")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# The Windows portable ZIP must not rely on a separately installed VC runtime.
# Set before JUCE creates its targets so all native libraries agree on the CRT.
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
add_subdirectory("${RASGO_JUCE_ROOT}" "${CMAKE_BINARY_DIR}/JUCE" EXCLUDE_FROM_ALL)
juce_add_gui_app(RasgoSynthStudio
    PRODUCT_NAME "Rasgo Synth Studio"
    COMPANY_NAME "Lucio de Araujo"
    BUNDLE_ID "net.arquiviagem.rasgo.synthstudio"
    VERSION "${PROJECT_VERSION}")
target_sources(RasgoSynthStudio PRIVATE src/Main.cpp src/LinuxKeyboardSymbols.cpp)
target_compile_definitions(RasgoSynthStudio PRIVATE
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1
    JUCE_STANDALONE_APPLICATION=1 JUCE_INCLUDE_JPEGLIB_CODE=1 JUCE_INCLUDE_PNGLIB_CODE=1)
if(MSVC)
    target_compile_options(RasgoSynthStudio PRIVATE /utf-8 /fp:precise /UNDEBUG /U_NDEBUG)
    target_compile_definitions(RasgoSynthStudio PRIVATE NOMINMAX _USE_MATH_DEFINES)
else()
    target_compile_options(RasgoSynthStudio PRIVATE -finput-charset=UTF-8 -fexec-charset=UTF-8
        -ffp-contract=off -UNDEBUG -U_NDEBUG "$<$<CONFIG:Release>:-O2>")
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(X11 REQUIRED)
    target_link_libraries(RasgoSynthStudio PRIVATE X11::X11)
    target_compile_definitions(RasgoSynthStudio PRIVATE JUCE_JACK=0 JUCE_ALSA=1
        JUCE_USE_XRANDR=0 JUCE_USE_XINERAMA=0 JUCE_USE_XCURSOR=0 JUCE_USE_XINPUT=0)
endif()
target_link_libraries(RasgoSynthStudio PRIVATE juce::juce_gui_basics
    juce::juce_audio_basics juce::juce_audio_devices)

# Explicit assets, not a recursive copy of the development directory.
if(APPLE)
    set(studio_assets "$<TARGET_BUNDLE_DIR:RasgoSynthStudio>/Contents/Resources/assets")
else()
    set(studio_assets "$<TARGET_FILE_DIR:RasgoSynthStudio>/assets")
endif()
set(studio_asset_files README.md rasgo_logo_2026.svg
    "Screenshot from 2026-08-01 03-29-34_b.png"
    fonts/BigShouldersDisplay.ttf fonts/IBMPlexMono-Regular.ttf fonts/IBMPlexMono-Bold.ttf
    fonts/LICENSE-BigShoulders.txt fonts/LICENSE-IBMPlexMono.txt)
foreach(asset IN LISTS studio_asset_files)
    get_filename_component(asset_directory "${asset}" DIRECTORY)
    add_custom_command(TARGET RasgoSynthStudio POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${studio_assets}/${asset_directory}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/${asset}" "${studio_assets}/${asset}" VERBATIM)
endforeach()

# Native packaging is explicit CPack execution, never publication or auto-install.
include("${CMAKE_CURRENT_SOURCE_DIR}/../packaging/cmake/InstallStudio.cmake")
