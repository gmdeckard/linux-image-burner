# linux-image-burner

A modern, feature-rich USB/DVD image burning tool for Linux systems. Similar to Rufus on Windows, this application provides a user-friendly interface for creating bootable USB drives from ISO, IMG, and other disk image formats.

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Platform](https://img.shields.io/badge/platform-Linux-green.svg)
![Qt](https://img.shields.io/badge/Qt-6-blue.svg)
![C++](https://img.shields.io/badge/C++-17-blue.svg)

## Quick Start

**TL;DR - Get it running in 3 commands:**

**Install dependencies listed below first!!**

Then: 

```bash
git clone https://github.com/gmdeckard/linux-image-burner.git
cd linux-image-burner
./build.sh && sudo ./install.sh
```

Then run: `linux-image-burner`

## Features

### **Core Functionality**
- **Multi-format support**: ISO, IMG, DMG, VHD, VHDX, VMDK
- **Reliable burning**: Uses `dd` with optimized parameters for bootable USB creation
- **Real-time progress**: Live progress monitoring with speed, percentage, and ETA
- **Progress parsing**: Advanced dd output parsing with multiple regex patterns
- **Bootloader detection**: Automatic detection of bootable images

### **Security & Safety**
- **No root required**: Starts as regular user, uses PolicyKit for privilege escalation
- **System disk protection**: Prevents accidental overwriting of system drives
- **Mount detection**: Automatically handles mounted partitions
- **Device validation**: Comprehensive safety checks before burning

### **User Experience**
- **Modern Qt6 interface** with dark theme
- **Real-time device detection**: Automatic USB device monitoring
- **Comprehensive device info**: Size, vendor, model, file system details
- **Progress feedback**: Speed, percentage, time remaining
- **Error handling**: Clear error messages and recovery suggestions

### **Advanced Options**
- **File system formatting**: FAT32, NTFS, ext4 support
- **Custom volume labels**: Set drive names during formatting
- **Cluster size control**: Optimize for different use cases
- **Verification**: Built-in image integrity checking

## Installation

### Prerequisites

- **Linux distribution** (Ubuntu 20.04+, Fedora 35+, or equivalent)
- **Qt6** development libraries
- **CMake** 3.16 or newer
- **PolicyKit** for privilege management

### Install Dependencies

**Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install build-essential cmake qt6-base-dev qt6-tools-dev libqt6widgets6 policykit-1

```

**Fedora/RHEL:**
```bash
sudo dnf install gcc-c++ cmake qt6-qtbase-devel qt6-qttools-devel polkit
```

**Arch Linux:**
```bash
sudo pacman -S base-devel cmake qt6-base qt6-tools polkit
```

### Build from Source

1. **Clone the repository:**
```bash
git clone https://github.com/gmdeckard/linux-image-burner.git
cd linux-image-burner
```

2. **Build the application:**
```bash
./build.sh
```

3. **Install system-wide:**
```bash
sudo ./install.sh
```

### Alternative: Manual Build

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo make install
```

## Usage

### Starting the Application

**From Applications Menu:**
- Look for "linux-image-burner" in your applications

**From Terminal:**
```bash
linux-image-burner
# or
image-burner
```

### Basic Workflow

1. **Select Image**: Click "Select Image" and choose your ISO/IMG file
2. **Choose Device**: Select your USB drive from the dropdown
3. **Configure Options** (optional): Set file system, label, etc.
4. **Burn**: Click "Start Burn" - you'll be prompted for authentication
5. **Wait**: Monitor progress until completion
6. **Done**: Your bootable USB is ready!

### Safety Features

- **Automatic detection** of removable devices only
- **System disk protection** prevents accidental overwrites
- **Mount handling** automatically unmounts before burning
- **Size validation** ensures image fits on device
- **Permission management** via PolicyKit (no sudo needed)

## Technical Details

### Architecture

- **Core Engine**: Built on Qt6 for cross-platform compatibility
- **Burning Backend**: Uses `dd` with optimized parameters (`bs=1M conv=fdatasync`)
- **Device Detection**: Real-time monitoring via `lsblk` and filesystem watchers
- **Privilege Management**: PolicyKit integration for secure privilege escalation
- **Progress Monitoring**: Real-time parsing of `dd` output for live feedback

### Security Model

1. **Application starts** as regular user (no privileges required)
2. **Device detection** uses standard system calls (`lsblk`, `/sys/block`)
3. **Burning operation** prompts for authentication via `pkexec`
4. **Temporary scripts** are created securely and cleaned up automatically
5. **Device validation** prevents writing to system disks

### File Support

| Format | Status | Notes |
|--------|--------|-------|
| ISO    | Full | Standard ISO 9660 images |
| IMG    | Full | Raw disk images |
| DMG    | Full | Apple disk images |
| VHD    | Full | Virtual Hard Disk |
| VHDX   | Full | Virtual Hard Disk v2 |
| VMDK   | Full | VMware disk images |

## Troubleshooting

### Common Issues

**"Insufficient permissions" error:**
- Ensure PolicyKit is installed and running
- Check that polkit policy is installed: `/usr/share/polkit-1/actions/org.linuxburner.image-burner.policy`

**USB device not detected:**
- Verify device is properly connected
- Check if device is mounted (will be auto-unmounted)
- Ensure device is formatted as a standard partition

**Build errors:**
- Verify all dependencies are installed
- Try cleaning build directory: `rm -rf build && ./build.sh`
- Check Qt6 installation: `qmake6 --version`

## Comparison with Other Tools

| Feature | linux-image-burner | Rufus | Etcher | dd command |
|---------|-------------------|-------|---------|------------|
| GUI Interface | ✅ Modern Qt6 | ✅ Windows only | ✅ Electron | ❌ CLI only |
| Real-time progress | ✅ Live updates | ✅ | ✅ | ❌ |
| Multi-format support | ✅ 6+ formats | ✅ | ✅ Limited | ✅ Any |
| Safety features | ✅ Comprehensive | ✅ | ✅ Basic | ❌ Manual |
| Device auto-detection | ✅ Real-time | ✅ | ✅ | ❌ Manual |
| Bootloader detection | ✅ Automatic | ✅ | ❌ | ❌ |

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---

