#!/bin/bash
# Install script for Linux Image Burner

set -e

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
if [[ $EUID -ne 0 ]]; then
    print_error "This script must be run as root (use sudo)"
    exit 1
fi

print_status "Installing Linux Image Burner..."

# Check if build exists
if [ ! -f "build/linux-image-burner" ]; then
    print_error "Build not found. Please run ./build.sh first."
    exit 1
fi

# Install binary
print_status "Installing executable to /usr/local/bin..."
cp build/linux-image-burner /usr/local/bin/
chmod +x /usr/local/bin/linux-image-burner

# Install desktop file
print_status "Installing desktop file..."
cp linux-image-burner.desktop /usr/share/applications/
chmod 644 /usr/share/applications/linux-image-burner.desktop

# Install polkit policy
print_status "Installing polkit policy..."
if [ -d "/usr/share/polkit-1/actions" ]; then
    cp org.linuxburner.image-burner.policy /usr/share/polkit-1/actions/
    chmod 644 /usr/share/polkit-1/actions/org.linuxburner.image-burner.policy
    print_status "Polkit policy installed - application can now request privileges as needed"
else
    print_warning "Polkit not found - you may need to install policykit-1 package"
fi

# Update desktop database
if command -v update-desktop-database &> /dev/null; then
    print_status "Updating desktop database..."
    update-desktop-database /usr/share/applications/
fi

# Create symbolic link for easy access
if [ ! -L /usr/local/bin/image-burner ]; then
    print_status "Creating symbolic link..."
    ln -s /usr/local/bin/linux-image-burner /usr/local/bin/image-burner
fi

print_status "Installation completed successfully!"
echo
print_status "You can now run the application:"
echo "  - From Applications menu: Linux Image Burner"
echo "  - From terminal: linux-image-burner"
echo "  - From terminal: image-burner"
echo
print_status "The application will request administrator privileges only when needed for burning operations."
