cmake_minimum_required(VERSION 3.22)
project(KickDuck VERSION 1.0.0 LANGUAGES C CXX)

add_subdirectory(JUCE)

juce_add_plugin(KickDuck
    COMPANY_NAME "YourName"
    PLUGIN_MANUFACTURER_CODE Yrnm
    PLUGIN_CODE Kddk
    FORMATS VST3
    PRODUCT_NAME "KickDuck"
    VST3_CATEGORIES Fx
    COPY_PLUGIN_AFTER_BUILD TRUE
)

target_sources(KickDuck PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(KickDuck PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(KickDuck PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags)

