cmake_minimum_required(VERSION 3.19)
project(Whiteboard LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Svg)

qt_standard_project_setup()

set(SOURCES
    main.cpp
    src/core/AppSettings.h
    src/core/AppSettings.cpp
    src/core/Document.h
    src/core/Document.cpp
    src/canvas/CanvasScene.h
    src/canvas/CanvasScene.cpp
    src/canvas/CanvasView.h
    src/canvas/CanvasView.cpp
    src/items/BaseItem.h
    src/items/BaseItem.cpp
    src/items/RectangleItem.h
    src/items/RectangleItem.cpp
    src/items/EllipseItem.h
    src/items/EllipseItem.cpp
    src/items/DiamondItem.h
    src/items/DiamondItem.cpp
    src/items/LineItem.h
    src/items/LineItem.cpp
    src/items/ArrowItem.h
    src/items/ArrowItem.cpp
    src/items/FreehandItem.h
    src/items/FreehandItem.cpp
    src/items/TextItem.h
    src/items/TextItem.cpp
    src/items/ImageItem.h
    src/items/ImageItem.cpp
    src/items/StickyNoteItem.h
    src/items/StickyNoteItem.cpp
    src/renderer/RoughRenderer.h
    src/renderer/RoughRenderer.cpp
    src/commands/AddItemCommand.h
    src/commands/AddItemCommand.cpp
    src/commands/RemoveItemCommand.h
    src/commands/RemoveItemCommand.cpp
    src/commands/MoveItemCommand.h
    src/commands/MoveItemCommand.cpp
    src/commands/ChangePropertyCommand.h
    src/commands/ChangePropertyCommand.cpp
    src/commands/GroupCommand.h
    src/commands/GroupCommand.cpp
    src/commands/ToggleLockCommand.h
    src/commands/ToggleLockCommand.cpp
    src/tools/ToolManager.h
    src/tools/ToolManager.cpp
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
    src/ui/Toolbar.h
    src/ui/Toolbar.cpp
    src/ui/PropertiesPanel.h
    src/ui/PropertiesPanel.cpp
)

qt_add_executable(Whiteboard
    WIN32 MACOSX_BUNDLE
    ${SOURCES}
)

target_include_directories(Whiteboard PRIVATE src)

target_link_libraries(Whiteboard
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Svg
)

install(TARGETS Whiteboard
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
