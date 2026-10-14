# Explicit native installation tree. No cache, private document or sibling copy.
get_filename_component(studio_source_root "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set_target_properties(RasgoSynthStudio PROPERTIES OUTPUT_NAME "rasgo-synth-studio")
    install(TARGETS RasgoSynthStudio RUNTIME DESTINATION bin)
    set(studio_install_assets "share/rasgo-synth/assets")
    set(studio_install_notices "share/doc/rasgo-synth-studio")
    install(FILES "${studio_source_root}/packaging/linux/rasgo-synth-studio.desktop" DESTINATION share/applications)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/assets/rasgo_logo_2026.svg"
        DESTINATION share/icons/hicolor/scalable/apps RENAME rasgo-synth-studio.svg)
else()
    install(TARGETS RasgoSynthStudio BUNDLE DESTINATION . RUNTIME DESTINATION .)
    set(studio_install_assets "assets")
    set(studio_install_notices "licenses")
endif()
if(NOT APPLE)
    foreach(asset IN LISTS studio_asset_files)
        get_filename_component(asset_directory "${asset}" DIRECTORY)
        install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/assets/${asset}"
            DESTINATION "${studio_install_assets}/${asset_directory}")
    endforeach()
endif()
foreach(notice LICENSE CREDITS.md DEPENDENCIES.md THIRD_PARTY_NOTICES.md INSTALL.md)
    install(FILES "${studio_source_root}/${notice}" DESTINATION "${studio_install_notices}")
    if(APPLE)
        add_custom_command(TARGET RasgoSynthStudio POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E make_directory
                "$<TARGET_BUNDLE_DIR:RasgoSynthStudio>/Contents/Resources/licenses"
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${studio_source_root}/${notice}"
                "$<TARGET_BUNDLE_DIR:RasgoSynthStudio>/Contents/Resources/licenses/${notice}" VERBATIM)
    endif()
endforeach()

# Notices for the locked JUCE code actually used in the native build.
set(studio_juce_notices
    "LICENSE.md|JUCE-LICENSE.md"
    "modules/juce_graphics/fonts/harfbuzz/COPYING|HarfBuzz-COPYING"
    "modules/juce_graphics/unicode/sheenbidi/LICENSE|SheenBidi-LICENSE"
    "modules/juce_graphics/drawables/lunasvg/LICENSE|LunaSVG-LICENSE"
    "modules/juce_graphics/drawables/lunasvg/plutovg/LICENSE|PlutoVG-LICENSE"
    "modules/juce_graphics/image_formats/jpglib/README|IJG-JPEG-README"
    "modules/juce_graphics/image_formats/pnglib/LICENSE|libpng-LICENSE"
    "modules/juce_core/zip/zlib/LICENSE|zlib-LICENSE")
foreach(pair IN LISTS studio_juce_notices)
    string(REPLACE "|" ";" fields "${pair}")
    list(GET fields 0 source)
    list(GET fields 1 name)
    install(FILES "${RASGO_JUCE_ROOT}/${source}"
        DESTINATION "${studio_install_notices}/third-party-licenses" RENAME "${name}")
    add_custom_command(TARGET RasgoSynthStudio POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_FILE_DIR:RasgoSynthStudio>/third-party-licenses"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${RASGO_JUCE_ROOT}/${source}"
            "$<TARGET_FILE_DIR:RasgoSynthStudio>/third-party-licenses/${name}" VERBATIM)
endforeach()
include("${CMAKE_CURRENT_LIST_DIR}/PackagePolicy.cmake")
studio_package_policy()
set(CPACK_RESOURCE_FILE_LICENSE "${studio_source_root}/LICENSE")
include(CPack)
