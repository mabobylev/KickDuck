cmake_minimum_required(VERSION 3.22)

project(KickDuck VERSION 1.0.0)

# Устанавливаем стандарт C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 1. Подключение JUCE ---
# Явно указываем путь к корню JUCE
add_subdirectory(JUCE)

# --- 2. Создание плагина ---
juce_add_plugin(KickDuck
    PRODUCT_NAME "KickDuck"
    COMPANY_NAME "Mabobylev"
    BUNDLE_ID "com.mabobylev.kickduck"
    FORMATS VST3               # Собираем только VST3
    VST3_COPY_DIR "$ENV{HOME}/.vst3" # Куда копировать после сборки
)

# --- 3. Источники ---
target_sources(KickDuck PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

# --- 4. Заголовочные файлы ---
# Указываем, где искать наши кастомные хедеры (Source/)
target_include_directories(KickDuck PRIVATE Source)

# --- 5. Привязка модулей JUCE ---
# Это критически важный шаг. Мы говорим компилятору, 
# какие модули JUCE нам нужны (audio_utils нужен для dsp::Gain и IIR)
target_link_libraries(KickDuck PRIVATE
    juce::juce_core
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_gui_basics
    juce::juce_gui_extra
)

# --- 6. Подавление предупреждений (Opus и др.) ---
if (TARGET juce::juce_core)
    target_compile_options(juce::juce_core PRIVATE 
        -Wno-stringop-overflow
        -Wno-deprecated-declarations)
endif()
