cmake_minimum_required(VERSION 3.16)

project(chatdot VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# 添加调试信息
message(STATUS "VCPKG_ROOT: $ENV{VCPKG_ROOT}")
message(STATUS "CMAKE_PREFIX_PATH: ${CMAKE_PREFIX_PATH}")

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Network
    Concurrent
)

# 设置 OpenSSL 查找路径
set(OPENSSL_ROOT_DIR "D:/vcpkg/installed/x64-windows")
find_package(OpenSSL REQUIRED)

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
    # src/utils/encryption.h
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
    # OpenSSL::SSL
    # OpenSSL::Crypto
)

# 设置包含目录
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils
)

# 设置输出目录
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# 安装目标
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

# 添加测试程序
add_executable(test_app src/test.cpp)
target_link_libraries(test_app PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)

# 设置测试程序输出目录
set_target_properties(test_app PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# 复制 OpenSSL DLL
if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:OpenSSL::SSL>
            $<TARGET_FILE:OpenSSL::Crypto>
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
endif()
