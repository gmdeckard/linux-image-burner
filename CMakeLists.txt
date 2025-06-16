cmake_minimum_required(VERSION 3.16)
project(linux-image-burner VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

# Enable automatic MOC, UIC, and RCC processing
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Include directories
include_directories(src)

# Source files
set(SOURCES
    src/main.cpp
    src/ui/MainWindow.cpp
    src/ui/ProgressDialog.cpp
    src/ui/DeviceInfoDialog.cpp
    src/core/DeviceManager.cpp
    src/core/ImageHandler.cpp
    src/core/Burner.cpp
    src/core/FileSystemManager.cpp
    src/utils/Utils.cpp
    src/utils/Validation.cpp
)

# Header files
set(HEADERS
    src/ui/MainWindow.h
    src/ui/ProgressDialog.h
    src/ui/DeviceInfoDialog.h
    src/core/DeviceManager.h
    src/core/ImageHandler.h
    src/core/Burner.h
    src/core/FileSystemManager.h
    src/utils/Utils.h
    src/utils/Validation.h
)

# UI files
set(UI_FILES
    src/ui/MainWindow.ui
    src/ui/ProgressDialog.ui
    src/ui/DeviceInfoDialog.ui
)

# Create executable
add_executable(linux-image-burner ${SOURCES} ${HEADERS} ${UI_FILES})

# Link libraries
target_link_libraries(linux-image-burner Qt6::Core Qt6::Widgets)

# Install target
install(TARGETS linux-image-burner DESTINATION bin)

# Package configuration
set(CPACK_PACKAGE_NAME "linux-image-burner")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_DESCRIPTION "Full-featured USB/DVD image burning tool for Linux")
set(CPACK_GENERATOR "DEB;RPM")
include(CPack)
