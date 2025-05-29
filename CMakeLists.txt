cmake_minimum_required(VERSION 3.16)
project(chatdot VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

find_package(Qt6 REQUIRED COMPONENTS 
    Core
    Gui
    Widgets
    Network
    Concurrent
)

set(PROJECT_SOURCES
    src/main.cpp
    src/models/chatmodel.cpp
    src/models/chatmodel.h
    src/models/settingsmodel.cpp
    src/models/settingsmodel.h
    src/models/imagemodel.cpp
    src/models/imagemodel.h
    src/viewmodels/chatviewmodel.cpp
    src/viewmodels/chatviewmodel.h
    src/viewmodels/settingsviewmodel.cpp
    src/viewmodels/settingsviewmodel.h
    src/views/mainwindow.cpp
    src/views/mainwindow.h
    src/views/settingsdialog.cpp
    src/views/settingsdialog.h
    src/services/llmservice.cpp
    src/services/llmservice.h
    src/services/apiservice.cpp
    src/services/apiservice.h
    src/services/ollamaservice.cpp
    src/services/ollamaservice.h
    src/services/localmodelservice.cpp
    src/services/localmodelservice.h
    src/services/logger.cpp
    src/services/logger.h
)

add_executable(${PROJECT_NAME}
    ${PROJECT_SOURCES}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Concurrent
)

# 设置包含目录
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# 设置输出目录
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# 安装目标
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
) 