cmake_minimum_required(VERSION 3.22)

project(KickDuck VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG origin/master
)

FetchContent_MakeAvailable(juce)

juce_add_plugin(KickDuck
    PRODUCT_NAME "KickDuck"
    COMPANY_NAME "Mabobylev"
    BUNDLE_ID "com.mabobylev.kickduck"
    FORMATS VST3
    VST3_COPY_DIR "$ENV{HOME}/.vst3"
)

target_sources(KickDuck PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

target_include_directories(KickDuck PRIVATE Source)

target_link_libraries(KickDuck PRIVATE
    juce::juce_core
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_dsp
    juce::juce_opengl
)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(KickDuck PRIVATE 
        -Wno-stringop-overflow
        -Wno-deprecated-declarations
    )
elseif (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(KickDuck PRIVATE 
        /wd4996
    )
endif()
