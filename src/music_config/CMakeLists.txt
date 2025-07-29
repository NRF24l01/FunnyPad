add_library(music_config STATIC
    playlist.cpp
    PlaylistManager.cpp
    track.cpp
)

# Make sure ffmpeg is available on the system
find_program(FFMPEG_EXECUTABLE NAMES ffmpeg)
if(NOT FFMPEG_EXECUTABLE)
    message(WARNING "ffmpeg not found. Audio processing will not work correctly.")
endif()

target_include_directories(music_config PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(music_config
    Qt6::Core
)