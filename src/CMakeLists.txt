cmake_minimum_required(VERSION 3.14)
project(FpgaAssist VERSION 2.0)

# Find Qt 6 components
find_package(Qt6 REQUIRED COMPONENTS Widgets SerialPort Network Multimedia MultimediaWidgets)

# Enable CMake's automation features for MOC, UIC, and RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Create the executable, adding the new TcpServerManager source files
add_executable(FpgaAssist
    main.cpp
    MainWindow.cpp
    MainWindow.h
    MainWindow.ui
    SerialManager.cpp
    SerialManager.h
    TcpManager.cpp
    TcpManager.h
    UdpManager.cpp
    UdpManager.h
    TcpServerManager.cpp
    TcpServerManager.h  
    WelcomeWindow.cpp   
    WelcomeWindow.h     
    WelcomeWindow.ui
)

# Link the target program to the required Qt modules
target_link_libraries(FpgaAssist PRIVATE
    Qt6::Widgets
    Qt6::SerialPort
    Qt6::Network
    Qt6::Multimedia
    Qt6::MultimediaWidgets
)

# Set the C++ standard to C++17 (recommended for Qt6)
set_target_properties(FpgaAssist PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)