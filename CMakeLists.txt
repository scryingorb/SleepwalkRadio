cmake_minimum_required(VERSION 3.22)
project(SleepwalkRadio VERSION 1.0.0 LANGUAGES C CXX)
# Numeric host/Windows version; candidate status is carried in packages and documentation.
set(SLEEPWALKRADIO_RELEASE_VERSION "1.0.0-rc.1")
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "SleepwalkRadio requires Windows x64")
endif()
include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 29396c22c93392d6738e021b83196283d6e4d850)
FetchContent_MakeAvailable(JUCE)
juce_add_plugin(SleepwalkRadio
    COMPANY_NAME "SleepwalkRadio"
    BUNDLE_ID "local.sleepwalkradio.effect"
    PLUGIN_MANUFACTURER_CODE Swrd
    PLUGIN_CODE Swr1
    PRODUCT_NAME "SleepwalkRadio"
    IS_SYNTH FALSE NEEDS_MIDI_INPUT FALSE NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx
    FORMATS VST3 Standalone)
target_sources(SleepwalkRadio PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp Source/TapeEngine.cpp Source/MidnightReceiver.cpp)
target_compile_definitions(SleepwalkRadio PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(SleepwalkRadio PUBLIC juce::juce_audio_utils juce::juce_dsp
    PRIVATE juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
include(CTest)
if(BUILD_TESTING)
    add_executable(SleepwalkRadioTests Tests/Phase1Tests.cpp Tests/TapeEngineTests.cpp Tests/EditorTests.cpp Tests/RealtimeAllocation.cpp)
    target_link_libraries(SleepwalkRadioTests PRIVATE SleepwalkRadio)
    add_test(NAME Phase1 COMMAND SleepwalkRadioTests)
    add_test(NAME Phase2DSP COMMAND SleepwalkRadioTests --dsp)
    add_test(NAME Phase3Editor COMMAND SleepwalkRadioTests --editor)
endif()
