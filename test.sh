#!/bin/bash
# Test script for Linux Image Burner core functionality

echo "Testing Linux Image Burner Core Components..."
echo "=============================================="

# Test 1: Check if executable exists and is working
echo "Test 1: Checking executable..."
if [ -f "build/linux-image-burner" ]; then
    echo "✓ Executable found"
    file build/linux-image-burner
else
    echo "✗ Executable not found"
    exit 1
fi

# Test 2: Check dependencies
echo
echo "Test 2: Checking Qt6 libraries..."
ldd build/linux-image-burner | grep -i qt

# Test 3: Check for required system tools
echo
echo "Test 3: Checking system tools..."
TOOLS="dd parted mkfs.fat mkfs.ntfs mkfs.ext4 blockdev sync"
for tool in $TOOLS; do
    if command -v $tool &> /dev/null; then
        echo "✓ $tool found"
    else
        echo "✗ $tool not found"
    fi
done

# Test 4: Check permissions
echo
echo "Test 4: Checking file permissions..."
ls -la build/linux-image-burner

# Test 5: Test basic help/version (if available)
echo
echo "Test 5: Basic functionality test..."
echo "Note: This test requires sudo privileges and may show permission warnings"

echo
echo "Test completed. To run the full application:"
echo "sudo ./build/linux-image-burner"
