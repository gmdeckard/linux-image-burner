#!/bin/bash
# Uninstall script for Linux Image Burner

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

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

# Check if running as root
if [[ $EUID -ne 0 ]]; then
    print_error "This script must be run as root (use sudo)"
    exit 1
fi

print_status "Uninstalling Linux Image Burner..."

# Function to remove file if it exists
remove_file() {
    local file="$1"
    local description="$2"
    
    if [ -f "$file" ]; then
        print_status "Removing $description..."
        rm -f "$file"
        print_success "$description removed"
    else
        print_warning "$description not found (may already be removed)"
    fi
}

# Function to remove symbolic link if it exists
remove_link() {
    local link="$1"
    local description="$2"
    
    if [ -L "$link" ]; then
        print_status "Removing $description..."
        rm -f "$link"
        print_success "$description removed"
    else
        print_warning "$description not found (may already be removed)"
    fi
}

# Remove main executable
remove_file "/usr/local/bin/linux-image-burner" "main executable"

# Remove symbolic link
remove_link "/usr/local/bin/image-burner" "symbolic link"

# Remove desktop file
remove_file "/usr/share/applications/linux-image-burner.desktop" "desktop file"

# Remove polkit policy
remove_file "/usr/share/polkit-1/actions/org.linuxburner.image-burner.policy" "polkit policy"

# Update desktop database if available
if command -v update-desktop-database &> /dev/null; then
    print_status "Updating desktop database..."
    update-desktop-database /usr/share/applications/
    print_success "Desktop database updated"
fi

# Remove any temporary files that might be left behind
print_status "Cleaning up temporary files..."
rm -f /tmp/burn_script_*.sh 2>/dev/null || true

# Check for any remaining files
print_status "Checking for remaining files..."
remaining_files=()

if [ -f "/usr/local/bin/linux-image-burner" ]; then
    remaining_files+=("/usr/local/bin/linux-image-burner")
fi

if [ -L "/usr/local/bin/image-burner" ]; then
    remaining_files+=("/usr/local/bin/image-burner")
fi

if [ -f "/usr/share/applications/linux-image-burner.desktop" ]; then
    remaining_files+=("/usr/share/applications/linux-image-burner.desktop")
fi

if [ -f "/usr/share/polkit-1/actions/org.linuxburner.image-burner.policy" ]; then
    remaining_files+=("/usr/share/polkit-1/actions/org.linuxburner.image-burner.policy")
fi

if [ ${#remaining_files[@]} -eq 0 ]; then
    print_success "All files successfully removed!"
else
    print_warning "Some files could not be removed:"
    for file in "${remaining_files[@]}"; do
        echo "  - $file"
    done
fi

echo
print_status "Uninstallation completed!"
echo
print_status "The following items have been removed:"
echo "  - Linux Image Burner executable"
echo "  - Desktop application entry"
echo "  - Polkit policy for privilege escalation"
echo "  - Application symbolic link"
echo "  - Temporary script files"
echo
print_status "To completely remove the application:"
echo "  1. Delete the source directory: rm -rf /path/to/usb-burn"
echo "  2. Remove any user configuration files (if created)"
echo
print_warning "Note: Any custom configurations or user data are not removed by this script."
