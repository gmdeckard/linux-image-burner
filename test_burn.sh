#!/bin/bash

# Test script for linux-image-burner
echo "Testing linux-image-burner functionality..."

# Check if pkexec is available
if command -v pkexec >/dev/null 2>&1; then
    echo "✓ pkexec is available for privilege escalation"
else
    echo "✗ pkexec not found - this will cause issues with burning"
    echo "  Install with: sudo apt install policykit-1"
fi

# Check if dd command is available
if command -v dd >/dev/null 2>&1; then
    echo "✓ dd command is available"
else
    echo "✗ dd command not found"
fi

# Check for required tools
for tool in lsblk parted mkfs.fat mkfs.ntfs; do
    if command -v $tool >/dev/null 2>&1; then
        echo "✓ $tool is available"
    else
        echo "✗ $tool not found"
    fi
done

# Test creating a temporary script (similar to what the burner does)
TEST_SCRIPT="/tmp/test_burn_script.sh"
cat > "$TEST_SCRIPT" << 'EOF'
#!/bin/bash
echo "Test script execution successful"
echo "This simulates the dd burning process"
EOF

chmod +x "$TEST_SCRIPT"

echo ""
echo "Testing script creation and execution..."
if "$TEST_SCRIPT"; then
    echo "✓ Script creation and execution works"
else
    echo "✗ Script execution failed"
fi

# Clean up
rm -f "$TEST_SCRIPT"

echo ""
echo "Running the GUI application..."
echo "Note: The application no longer requires sudo to start"
echo "It will request privileges only when needed for burning operations"

# Launch the application
cd /home/gdeckard/usb-burn/build
./linux-image-burner
