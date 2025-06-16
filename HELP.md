# linux-image-burner - Help & Documentation

## Overview

linux-image-burner is a comprehensive USB/DVD image burning tool designed as a Linux alternative to Rufus. It provides a modern Qt-based GUI with all the features needed to create bootable USB drives from various image formats.

## Installation Guide

### Quick Installation
```bash
# 1. Install dependencies
sudo apt install build-essential cmake qt6-base-dev qt6-tools-dev policykit-1

# 2. Build the application
./build.sh

# 3. Run directly
./build/linux-image-burner

# 4. (Optional) Install system-wide
sudo ./install.sh
```

### System Requirements
- **OS**: Any modern Linux distribution
- **Architecture**: x86_64, i386, ARM64
- **RAM**: 512 MB minimum
- **Storage**: 50 MB for installation
- **Qt**: Qt6 or Qt5 framework

## Usage Guide

### Basic Operation

1. **Launch Application**
   - No sudo required to start
   - GUI will appear with device list

2. **Select Image File**
   - Click "Select Image..." button
   - Choose ISO, IMG, DMG, VHD, VHDX, or VMDK file
   - Image information will be displayed

3. **Select Target Device**
   - Choose USB drive from dropdown
   - Only removable devices are shown for safety
   - Click "Device Info" for detailed information

4. **Configure Options**
   - File system: FAT32 (recommended), NTFS, exFAT, ext4
   - Partition scheme: MBR (legacy) or GPT (UEFI)
   - Volume label: Custom name for the drive

5. **Start Burning**
   - Click "Start" button
   - System will request admin password via pkexec
   - Progress will be shown in real-time
   - Device will be ejected when complete

### Advanced Options

**Show Advanced Options** reveals additional settings:

- **Quick Format**: Faster but less thorough formatting
- **Verify after burning**: Check data integrity after writing
- **Create bootable USB**: Enable boot sector creation
- **Check for bad blocks**: Scan for defective sectors

### Progress Monitoring

During burning, you'll see:
- **Progress bar**: Percentage completed
- **Speed**: Current transfer rate (MB/s)
- **Time remaining**: Estimated completion time
- **Bytes written**: Amount of data transferred
- **Status messages**: Current operation

## File Formats Supported

### Image Formats
| Format | Description | Bootable | Notes |
|--------|-------------|----------|-------|
| ISO | CD/DVD/Blu-ray images | Yes | Most common format |
| IMG | Raw disk images | Yes | Linux/Windows images |
| DMG | Apple Disk Images | Yes | macOS installer images |
| VHD | Virtual Hard Disk | Yes | Microsoft virtual disks |
| VHDX | Enhanced VHD | Yes | Newer VHD format |
| VMDK | VMware disk | Yes | VMware virtual disks |

### File Systems
| System | Max Size | Max File | Compatibility | Bootable |
|--------|----------|----------|---------------|----------|
| FAT32 | 2 TB | 4 GB | Excellent | Yes |
| NTFS | 256 TB | 16 TB | Windows/Linux | Yes |
| exFAT | 128 PB | 16 EB | Modern systems | No |
| ext4 | 1 EB | 16 TB | Linux only | Yes |

## Troubleshooting

### Common Issues

**"No devices found"**
- Ensure USB drive is connected
- Try a different USB port
- Check if device is mounted elsewhere

**"Permission denied"**
- Install policykit: `sudo apt install policykit-1`
- Check pkexec is working: `pkexec --version`
- Try running as root if pkexec fails

**"Image too large"**
- Use a larger USB drive
- Check available space on device
- Verify image file isn't corrupted

**"Burning failed"**
- Check USB drive isn't write-protected
- Try a different USB drive
- Verify image file integrity

**"Boot doesn't work"**
- Ensure BIOS/UEFI boot order is correct
- Try different partition scheme (MBR vs GPT)
- Check if image is actually bootable

### Log Information

Enable detailed logging by running:
```bash
QT_LOGGING_RULES="*=true" ./linux-image-burner
```

This provides detailed debug information in the terminal.

### Getting Help

1. Check this documentation
2. Review error messages carefully
3. Check system logs: `journalctl -f`
4. Verify dependencies are installed
5. Report bugs with detailed information

## Safety Features

The application includes multiple safety measures:

### Device Protection
- **System drive filtering**: Prevents writing to system disks
- **Removable device detection**: Only shows safe devices
- **Mount point checking**: Warns about mounted filesystems
- **Confirmation dialogs**: Confirms destructive operations

### Data Verification
- **Image validation**: Checks image file integrity
- **Burn verification**: Compares written data to source
- **Bad block detection**: Identifies defective sectors
- **Checksum verification**: MD5/SHA256 validation

## Command Line Options

While primarily a GUI application, some options are available:

```bash
# Show version information
linux-image-burner --version

# Enable debug output
linux-image-burner --debug

# Specify initial image file
linux-image-burner /path/to/image.iso
```

## Uninstallation

To remove linux-image-burner from your system:

```bash
sudo ./uninstall.sh
```

This removes:
- Main executable (`/usr/local/bin/linux-image-burner`)
- Desktop entry (`/usr/share/applications/`)
- Polkit policy (`/usr/share/polkit-1/actions/`)
- Symbolic links and temporary files

## Technical Details

### Architecture
- **Core**: Device management, image handling, burning engine
- **UI**: Qt6-based responsive interface
- **Utils**: Validation, file operations, system info

### Burning Process
1. Image validation and analysis
2. Device preparation and unmounting
3. Temporary script creation for dd command
4. Privilege escalation via pkexec
5. dd execution with progress monitoring
6. Device synchronization and cleanup

### Security Model
- Application runs without privileges
- Uses PolicyKit (pkexec) for elevation
- Temporary scripts for secure execution
- Automatic cleanup of sensitive files

## Development

### Building from Source
```bash
git clone <repository>
cd linux-image-burner
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Dependencies
- Qt6 Core and Widgets (or Qt5 fallback)
- CMake 3.16+
- GCC/Clang with C++17 support
- PolicyKit development files

### Contributing
1. Fork the repository
2. Create feature branch
3. Add tests for new functionality
4. Submit pull request with clear description

## License

GNU General Public License v3.0 - see LICENSE file for details.

## Version History

- **v1.0.0**: Initial release with full Rufus-equivalent functionality
- Comprehensive image format support
- Modern Qt6 interface
- Security-focused privilege model
- Cross-distribution compatibility
