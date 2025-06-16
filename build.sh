#!/bin/bash
# Build script for Linux Image Burner

set -e

echo "Building Linux Image Burner..."

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if running as root
if [[ $EUID -eq 0 ]]; then
    print_warning "Running as root. This is not recommended for building."
fi

# Check for required tools
print_status "Checking for required tools..."
REQUIRED_TOOLS="cmake make gcc g++"
MISSING_TOOLS=""

for tool in $REQUIRED_TOOLS; do
    if ! command -v $tool &> /dev/null; then
        MISSING_TOOLS="$MISSING_TOOLS $tool"
    fi
done

# Check for Qt6 qmake specifically
QT6_QMAKE=""
if command -v qmake6 &> /dev/null; then
    QT6_QMAKE="qmake6"
elif [ -f "/usr/lib/qt6/bin/qmake6" ]; then
    QT6_QMAKE="/usr/lib/qt6/bin/qmake6"
elif command -v qmake &> /dev/null; then
    # Check if it's Qt6
    QT_VERSION=$(qmake -query QT_VERSION 2>/dev/null | cut -d. -f1)
    if [ "$QT_VERSION" = "6" ]; then
        QT6_QMAKE="qmake"
    fi
fi

if [ -z "$QT6_QMAKE" ]; then
    MISSING_TOOLS="$MISSING_TOOLS qt6-qmake"
fi

if [ ! -z "$MISSING_TOOLS" ]; then
    print_error "Missing required tools:$MISSING_TOOLS"
    print_error "Please install the required dependencies."
    echo
    echo "Ubuntu/Debian: sudo apt install build-essential cmake qt6-base-dev qt6-tools-dev"
    echo "Fedora/RHEL:   sudo dnf install gcc-c++ cmake qt6-qtbase-devel qt6-qttools-devel"
    echo "Arch Linux:    sudo pacman -S base-devel cmake qt6-base qt6-tools"
    exit 1
fi

# Check Qt6 version
print_status "Checking Qt6 version..."
QT_VERSION=$($QT6_QMAKE -query QT_VERSION 2>/dev/null || echo "Unknown")
print_status "Qt6 version: $QT_VERSION"
print_status "Using qmake: $QT6_QMAKE"

# Create build directory
BUILD_DIR="build"
if [ -d "$BUILD_DIR" ]; then
    print_status "Removing existing build directory..."
    rm -rf "$BUILD_DIR"
fi

print_status "Creating build directory..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake
print_status "Configuring with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DCMAKE_INSTALL_PREFIX=/usr/local

# Build
print_status "Building application..."
make -j$(nproc)

# Check if build was successful
if [ $? -eq 0 ]; then
    print_status "Build completed successfully!"
    print_status "Executable: $(pwd)/linux-image-burner"
    echo
    print_status "To install system-wide, run: sudo make install"
    print_status "To run directly: sudo ./linux-image-burner"
else
    print_error "Build failed!"
    exit 1
fi
