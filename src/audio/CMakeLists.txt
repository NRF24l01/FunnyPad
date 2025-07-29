find_path(PULSE_INCLUDE_DIR pulse/simple.h)
find_library(PULSE_SIMPLE_LIBRARY pulse-simple)

include_directories(${PULSE_INCLUDE_DIR} include)

add_library(soundpad_audio STATIC
    SoundpadAudio.cpp
)

target_include_directories(soundpad_audio PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(soundpad_audio
    ${PULSE_SIMPLE_LIBRARIES}
    ${PULSE_LIBRARIES}
    Qt6::Core
)