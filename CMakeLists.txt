cmake_minimum_required(VERSION 3.22)

# ─── macOS build settings (must be set before project()) ────────────────────
if (APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS version")
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build architectures")
endif ()

project(ZSkeybpm VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
    # The sources carry Russian tooltips and no BOM. Without this, MSVC reads them
    # in the machine's ANSI code page and the plug-in ships mojibake on Windows,
    # depending on the locale of whoever built it. JUCE does not set this for us.
    add_compile_options(/utf-8)
endif ()

option(ZSKEYBPM_COPY_AFTER_BUILD
       "Copy the built plug-ins into the user plug-in folders after building" ON)

add_subdirectory(JUCE)

# ─── The plug-in ────────────────────────────────────────────────────────────
juce_add_plugin(ZSkeybpm
    COMPANY_NAME              "ZS Records"
    COMPANY_WEBSITE           "https://zsr.artspace1977.ru"
    BUNDLE_ID                 "ru.zsrecords.zskeybpm"
    PLUGIN_MANUFACTURER_CODE  ZSRc
    PLUGIN_CODE               Zkbp
    FORMATS                   AU VST3 Standalone
    PRODUCT_NAME              "ZS-keybpm"
    DESCRIPTION               "Tempo and key detector"
    IS_SYNTH                  FALSE
    NEEDS_MIDI_INPUT          FALSE
    NEEDS_MIDI_OUTPUT         FALSE
    IS_MIDI_EFFECT            FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD   ${ZSKEYBPM_COPY_AFTER_BUILD}
    VST3_CATEGORIES           Fx Analyzer
    AU_MAIN_TYPE              kAudioUnitType_Effect
    NEEDS_WEB_BROWSER         FALSE
    NEEDS_CURL                FALSE)

# ─── Embedded brand fonts ───────────────────────────────────────────────────
juce_add_binary_data(ZSkeybpmAssets
    NAMESPACE ZsAssets
    SOURCES
        Resources/Fonts/MontserratBold.ttf
        Resources/Fonts/MontserratMedium.ttf
        Resources/Fonts/InterMedium.ttf)

set_target_properties(ZSkeybpmAssets PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

target_sources(ZSkeybpm
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/dsp/AnalysisEngine.cpp
        Source/dsp/Decimator.cpp
        Source/dsp/TempoTracker.cpp
        Source/dsp/KeyTracker.cpp
        Source/gui/Theme.cpp
        Source/gui/ZsLookAndFeel.cpp
        Source/gui/BrandBackground.cpp
        Source/gui/BpmReadout.cpp
        Source/gui/KeyReadout.cpp
        Source/gui/ChromaWheel.cpp
        Source/gui/BeatStrip.cpp
        Source/gui/ChoiceStrip.cpp)

target_include_directories(ZSkeybpm PRIVATE Source)

target_compile_definitions(ZSkeybpm
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_REPORT_APP_USAGE=0
        JUCE_DSP_ENABLE_SNAP_TO_ZERO=1)

target_link_libraries(ZSkeybpm
    PRIVATE
        ZSkeybpmAssets
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# ─── Brand image renderer (editor shots and installer artwork) ──────────────
option(ZSKEYBPM_BUILD_ART "Build the offline brand image renderer" OFF)

if (ZSKEYBPM_BUILD_ART)
    juce_add_console_app(ZSkeybpmArt PRODUCT_NAME "ZS-keybpm Art")

    target_sources(ZSkeybpmArt
        PRIVATE
            Tools/RenderArt.cpp
            Source/PluginProcessor.cpp
            Source/PluginEditor.cpp
            Source/dsp/AnalysisEngine.cpp
            Source/dsp/Decimator.cpp
            Source/dsp/TempoTracker.cpp
            Source/dsp/KeyTracker.cpp
            Source/gui/Theme.cpp
            Source/gui/ZsLookAndFeel.cpp
            Source/gui/BrandBackground.cpp
            Source/gui/BpmReadout.cpp
            Source/gui/KeyReadout.cpp
            Source/gui/ChromaWheel.cpp
            Source/gui/BeatStrip.cpp
            Source/gui/ChoiceStrip.cpp)

    target_include_directories(ZSkeybpmArt PRIVATE Source)

    # Outside a plug-in wrapper these come from nowhere, so they are stated here.
    target_compile_definitions(ZSkeybpmArt
        PUBLIC
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
            JUCE_STANDALONE_APPLICATION=1
            JUCE_DSP_ENABLE_SNAP_TO_ZERO=1
            JUCE_MODAL_LOOPS_PERMITTED=1     # the renderer pumps the loop by hand
            JucePlugin_Name="ZS-keybpm"
            JucePlugin_VersionString="${PROJECT_VERSION}")

    target_link_libraries(ZSkeybpmArt
        PRIVATE
            ZSkeybpmAssets
            juce::juce_audio_utils
            juce::juce_dsp
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags)
endif ()

# ─── Offline DSP checks ─────────────────────────────────────────────────────
option(ZSKEYBPM_BUILD_TESTS "Build the offline DSP checks" OFF)

if (ZSKEYBPM_BUILD_TESTS)
    juce_add_console_app(ZSkeybpmTests PRODUCT_NAME "ZS-keybpm Tests")

    target_sources(ZSkeybpmTests
        PRIVATE
            Tests/DspTests.cpp
            Source/dsp/Decimator.cpp
            Source/dsp/TempoTracker.cpp
            Source/dsp/KeyTracker.cpp)

    target_compile_definitions(ZSkeybpmTests
        PUBLIC
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
            JUCE_STANDALONE_APPLICATION=1
            JUCE_DSP_ENABLE_SNAP_TO_ZERO=1)

    target_link_libraries(ZSkeybpmTests
        PRIVATE
            juce::juce_dsp
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags)

    enable_testing()
    add_test(NAME dsp COMMAND ZSkeybpmTests)
endif ()
