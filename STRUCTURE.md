# linux-image-burner - Project Structure

## Root Directory Files

### Build & Installation Scripts
- **`build.sh`** - Main build script with Qt6/Qt5 support
- **`install.sh`** - System installation script
- **`uninstall.sh`** - Complete removal script
- **`CMakeLists.txt`** - CMake build configuration

### Documentation
- **`README.md`** - Main project documentation
- **`HELP.md`** - Comprehensive user guide
- **`STATUS.md`** - Project completion status
- **`LICENSE`** - GNU GPL v3.0 license

### Configuration Files
- **`linux-image-burner.desktop`** - Desktop application entry
- **`org.linuxburner.image-burner.policy`** - PolicyKit privilege rules

### Test Scripts
- **`demo.sh`** - Feature demonstration script
- **`test_burn.sh`** - Burning functionality test
- **`test_uninstall.sh`** - Uninstall script verification
- **`test.sh`** - General functionality tests

## Source Code Structure

### Core Components (`src/core/`)
- **`DeviceManager.{h,cpp}`** - USB device detection and management
- **`ImageHandler.{h,cpp}`** - Image format support and analysis
- **`Burner.{h,cpp}`** - Main burning engine with dd integration
- **`FileSystemManager.{h,cpp}`** - File system operations

### User Interface (`src/ui/`)
- **`MainWindow.{h,cpp,ui}`** - Primary application interface
- **`ProgressDialog.{h,cpp,ui}`** - Burn progress monitoring
- **`DeviceInfoDialog.{h,cpp,ui}`** - Device information display

### Utilities (`src/utils/`)
- **`Utils.{h,cpp}`** - General utility functions
- **`Validation.{h,cpp}`** - Input validation and safety checks

### Application Entry
- **`main.cpp`** - Application initialization and theming

## Build System

### CMake Configuration
- Qt6 with Qt5 fallback support
- Automatic MOC, UIC, and RCC processing
- Package generation (DEB/RPM)
- Installation targets

### Build Process
1. Dependency checking (Qt6/Qt5, tools)
2. CMake configuration
3. Source compilation with Qt integration
4. Executable generation

## Key Features Implementation

### Device Management
- Real-time USB device monitoring
- Safety filtering (removable devices only)
- Mount point detection and unmounting
- Device information gathering (lsblk integration)

### Image Handling
- Multi-format support (ISO, IMG, DMG, VHD, VHDX, VMDK)
- Bootloader detection and analysis
- Image validation and integrity checking
- Size calculation and compatibility checking

### Burning Engine
- dd-based disk writing for reliable bootable creation
- pkexec privilege escalation (no sudo required)
- Real-time progress monitoring via stderr parsing
- Temporary script generation for secure execution
- Device synchronization and cleanup

### User Interface
- Modern Qt6 dark theme
- Responsive layout with progress feedback
- Advanced options panel
- Device information dialogs
- Error handling and user notifications

### Security Model
- Application starts without root privileges
- PolicyKit integration for privilege escalation
- Temporary script cleanup
- System disk protection
- Input validation and sanitization

## Dependencies

### Runtime Requirements
- Qt6 Core and Widgets (or Qt5 fallback)
- PolicyKit (pkexec) for privilege escalation
- Standard Linux utilities (dd, lsblk, parted, etc.)

### Build Requirements
- CMake 3.16+
- Qt6 development packages
- C++17 compatible compiler
- Make or Ninja build system

## Installation Layout

### System Files (after `sudo ./install.sh`)
```
/usr/local/bin/linux-image-burner          # Main executable
/usr/local/bin/image-burner                 # Symbolic link
/usr/share/applications/                    # Desktop integration
/usr/share/polkit-1/actions/               # Privilege policies
```

### Temporary Files (during operation)
```
/tmp/burn_script_*.sh                      # Secure burning scripts
```

## Quality Assurance

### Testing Strategy
- Unit tests for core functionality
- Integration tests for device operations
- GUI testing with Qt Test framework
- Manual testing with various image formats

### Code Quality
- C++17 modern standards
- Qt best practices
- Memory management with RAII
- Exception safety and error handling

### Documentation Standards
- Comprehensive API documentation
- User guide and troubleshooting
- Installation and build instructions
- Code comments and examples

This project structure provides a complete, professional USB burning tool that matches Rufus functionality while being designed specifically for Linux systems with modern security practices.
