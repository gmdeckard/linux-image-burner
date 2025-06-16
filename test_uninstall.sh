#!/bin/bash
# Test script to verify uninstall.sh functionality

echo "=== Testing Uninstall Script ==="
echo

echo "1. Checking what files would be removed by uninstall.sh:"
echo

# Check for files that would be removed
files_to_check=(
    "/usr/local/bin/linux-image-burner"
    "/usr/local/bin/image-burner"
    "/usr/share/applications/linux-image-burner.desktop"
    "/usr/share/polkit-1/actions/org.linuxburner.image-burner.policy"
)

for file in "${files_to_check[@]}"; do
    if [ -f "$file" ] || [ -L "$file" ]; then
        echo "   ✓ Found: $file (would be removed)"
    else
        echo "   ✗ Not found: $file (nothing to remove)"
    fi
done

echo
echo "2. Checking for temporary files:"
temp_files=$(ls /tmp/burn_script_*.sh 2>/dev/null || true)
if [ -n "$temp_files" ]; then
    echo "   ✓ Found temporary files: $temp_files (would be cleaned up)"
else
    echo "   ✓ No temporary files found"
fi

echo
echo "3. Checking system commands used by uninstall script:"
commands=("update-desktop-database")

for cmd in "${commands[@]}"; do
    if command -v "$cmd" >/dev/null 2>&1; then
        echo "   ✓ $cmd is available"
    else
        echo "   ⚠ $cmd not found (script will skip this step)"
    fi
done

echo
echo "4. Testing script syntax:"
if bash -n /home/gdeckard/usb-burn/uninstall.sh; then
    echo "   ✓ Uninstall script syntax is valid"
else
    echo "   ✗ Uninstall script has syntax errors"
    exit 1
fi

echo
echo "5. Checking script permissions:"
if [ -x /home/gdeckard/usb-burn/uninstall.sh ]; then
    echo "   ✓ Uninstall script is executable"
else
    echo "   ⚠ Uninstall script is not executable (run: chmod +x uninstall.sh)"
fi

echo
echo "=== Test Complete ==="
echo
echo "To actually uninstall (if installed), run:"
echo "   sudo ./uninstall.sh"
echo
echo "Note: This test does not actually remove any files."
