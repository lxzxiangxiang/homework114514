cmake_minimum_required(VERSION 3.19)
project(AgarClone LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)

qt_standard_project_setup()

qt_add_executable(AgarClone
    MACOSX_BUNDLE
    main.cpp
    Ball.cpp
    Food.cpp
    SkillBall.cpp
    Hazard.cpp
    EjectBall.cpp
    SpatialGrid.cpp
    AIController.cpp
    GameScene.cpp
    GameView.cpp
    UIManager.cpp
    AIController.h Ball.h Constants.h EjectBall.h Entity.h Food.h GameScene.h GameView.h Hazard.h SkillBall.h SpatialGrid.h UIManager.h
    docs/AIController.md docs/Ball.md docs/EjectBall.md docs/Food.md docs/GameScene.md docs/GameView.md docs/Hazard.md docs/main.md docs/SkillBall.md docs/SpatialGrid.md docs/UIManager.md
)

target_include_directories(AgarClone PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(AgarClone
    PRIVATE
        Qt::Core
        Qt::Widgets
)

install(TARGETS AgarClone
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET AgarClone
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
