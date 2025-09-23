# NexusTerm

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

NexusTerm is a modern, cross-platform serial and network debugging assistant built with Qt 6. It provides a comprehensive suite of tools for hardware developers, embedded engineers, and network programmers, integrating Serial, TCP (Client/Server), and UDP communication into a single, intuitive graphical interface.

The application is particularly useful for debugging devices like FPGAs, microcontrollers, and IoT gadgets, with a special feature for automatically rendering received image and video streams.

![](https://pub-a7510641c4c0427886fce394cb093861.r2.dev/image-20250923174205769.png)

## ✨ Features

- **Multi-Protocol Support**:
  - **Serial**: Full configuration of port, baud rate, data bits, stop bits, and parity.
  - **TCP Server**: Listen on a local port, manage multiple clients, and send data to specific clients.
  - **TCP Client**: Connect to any TCP server.
  - **UDP**: Bind to a local port and send data to a target IP/port simultaneously.
- **Advanced Data Handling**:
  - Send data in ASCII or HEX format.
  - Display received data in ASCII, HEX, or Decimal formats.
  - Automatic periodic data transmission.
- **Media-Aware Display**:
  - Automatically detects and renders incoming image data.
  - Plays video data streams using an integrated media player with playback controls.
- **User-Friendly Interface**:
  - Real-time connection status and byte-count monitoring.
  - Separate logging for sent and received data.
  - Cross-platform support for Windows, macOS, and Linux.

## 📦 Prerequisites

Before you begin, ensure you have the following installed on your system:
- **Git**
- **CMake** (version 3.14 or higher)
- **A C++17 compliant compiler** (GCC, Clang, or MSVC)
- **Qt 6.2 or newer** with the following modules:
  - `Widgets`
  - `SerialPort`
  - `Network`
  - `Multimedia`
  - `MultimediaWidgets`

## 🚀 Building from Source

First, clone the repository to your local machine:
```bash
git clone [https://github.com/YourUsername/YourRepoName.git](https://github.com/YourUsername/YourRepoName.git)
cd YourRepoName
```

Then, follow the instructions for your specific operating system.

---

### 🐧 Linux

#### Arch Linux
1.  **Install dependencies:**
    ```bash
    sudo pacman -Syu base-devel qt6-base qt6-serialport qt6-multimedia cmake git
    ```
2.  **Compile the project:**
    ```bash
    mkdir build && cd build
    cmake ..
    make -j$(nproc)
    ```

#### Ubuntu / Debian
1.  **Install dependencies:**
    ```bash
    sudo apt update
    sudo apt install build-essential qt6-base-dev libqt6serialport6-dev libqt6multimedia6-dev cmake git
    ```
2.  **Compile the project:**
    ```bash
    mkdir build && cd build
    cmake ..
    make -j$(nproc)
    ```

---

### 🪟 Windows

1.  **Install dependencies:**
    - **Visual Studio 2019/2022**: Install with the "Desktop development with C++" workload. This provides the MSVC compiler, nmake, and the developer command prompt.
    - **Qt 6**: Use the official [Qt Online Installer](https://www.qt.io/download-qt-installer). During installation, select the Qt version that matches your compiler (e.g., `MSVC 2019 64-bit`) and ensure you check the `Serial Port` and `Multimedia` modules.
    - **Git** and **CMake**: Install from their official websites.

2.  **Compile the project:**
    - **Important**: Open a **Developer Command Prompt for VS**.
    - Navigate to the cloned repository directory.
    - Run the following commands:
      ```cmd
      mkdir build
      cd build
      cmake ..
      cmake --build .
      ```

---

### 🍎 macOS

1.  **Install dependencies:**
    - **Xcode Command Line Tools**: Open a terminal and run `xcode-select --install`.
    - **Homebrew**: If you don't have it, install it from [brew.sh](https://brew.sh).
    - **Qt 6 and other tools**: Use Homebrew to install everything needed.
      ```bash
      brew install qt6 cmake git
      ```

2.  **Compile the project:**
    - We need to tell CMake where to find the Homebrew-installed Qt.
    ```bash
    mkdir build && cd build
    cmake -D CMAKE_PREFIX_PATH=$(brew --prefix qt6) ..
    make -j$(sysctl -n hw.ncpu)
    ```

## 🏃 Usage

After a successful build, the executable will be located in the `build` directory. You can run it directly from there.

- On **Linux** and **macOS**: `./FpgaAssist`
- On **Windows**: The executable will be in `build\Debug\FpgaAssist.exe` or a similar path.

## 📄 License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.
