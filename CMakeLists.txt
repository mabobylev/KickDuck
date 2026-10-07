cmake_minimum_required(VERSION 3.22)

project(KickDuck VERSION 1.0.0)

# Рекомендуемый стандарт для JUCE 7/8/9
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- 1. Подключение JUCE через FetchContent (без папки в репозитории) ---
include(FetchContent)

FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG origin/master # Или конкретный тег, например, 8.0.3 для стабильности
)

# Делаем модули JUCE доступными. 
# Эта команда скачает JUCE в папку ~/.cmake-builds/ и настроит цели.
FetchContent_MakeAvailable(juce)

# --- 2. Создание плагина ---
juce_add_plugin(KickDuck
    PRODUCT_NAME "KickDuck"
    COMPANY_NAME "Mabobylev"
    BUNDLE_ID "com.mabobylev.kickduck"
    FORMATS VST3
    VST3_COPY_DIR "$ENV{HOME}/.vst3"
)

# --- 3. Источники ---
target_sources(KickDuck PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

# --- 4. Заголовочные файлы ---
target_include_directories(KickDuck PRIVATE Source)

# --- 5. Привязка модулей JUCE ---
target_link_libraries(KickDuck PRIVATE
    juce::juce_core
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_gui_basics
    juce::juce_gui_extra
    # --- КРИТИЧЕСКИ ВАЖНО: Модуль DSP ---
    juce::juce_dsp 
)

# --- 6. Подавление предупреждений (кроссплатформенно) ---
# Применяем флаги к нашему плагину, а не к алиасам JUCE
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
