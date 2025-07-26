qt6_wrap_ui(ui_files main_window.ui)

add_library(ui STATIC
    main_window.cpp
    main_window.h
    main.ui
)

target_link_libraries(ui Qt6::Widgets)
target_include_directories(ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
