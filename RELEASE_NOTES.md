# 🚀 QuickView v5.3.0 - Vector UI & Interaction Update

**Release Date**: 2026-04-24

### 🍃 Our Philosophy: Green, Pure, and Portable
QuickView is built on the core principle of **Portable First**. By default, it does not write to the registry or pollute system paths.
- **Portable Preferred**: We strongly recommend users to choose the `Portable.zip` version for a truly standalone experience with all configurations kept in the application folder.
- **Installer Version**: The new installer (.exe) is provided solely for **WinGet** ecosystem compatibility. Even when using the installer, we ensure a "zero-footprint" experience through deep cleanup logic during uninstallation.

---

### 🎨 Vectorized UI Revolution
We have fully migrated all UI icons to our custom-built **GeekIcon** vector engine:
- **Pixel Perfection**: Rendered using Direct2D paths, ensuring crisp visuals on 4K/8K displays and high-DPI scaling.
- **Zero Dependencies**: Eliminated reliance on system icon fonts, guaranteeing a consistent look across all Windows environments.

### 🛠 Windows Integration & Deep Cleanup
To support automated distribution while maintaining our green roots, we've implemented several low-level optimizations:
- **Deep Uninstallation**: Introduced a new `--uninstall` flag for total removal of registry ProgIDs, association caches, and AppData remnants.
- **WinGet Automation**: Optimized our submission pipeline to ensure standard installers are verified and easily accessible via `winget install QuickView`.
- **Seamless Association**: Refined Windows 11 "Default Apps" integration for better system-level recognition.

### 🎥 Interaction & Animation
- **Dynamic UX**: Automatically switches to a **Hand Cursor** when panning images larger than the viewport (#144).
- **Precision Control**: Added a real-time **Frame Counter** to the animation scrubber for GIF/WebP files (#167), and added support for horizontal mouse wheels (#156).
- **Smart Zoom**: Refined the double-click zoom logic to intelligently cycle through common scaling modes.

### 🌈 HDR & Color (Experimental)
- **Luminance Correction**: Addressed "washed out" colors on certain HDR displays by prioritizing WinRT/DXGI hardware reports for peak luminance mapping (#131).

---

### 📦 Full Changelog

#### ✨ Features
- Migrated to the high-performance **GeekIcon** vector system, removing font dependencies.
- Implemented Windows Default Apps integration and enhanced Portable Mode UI (#168).
- Added deep uninstallation cleanup via the `--uninstall` parameter.
- Supported horizontal scrolling and thumb wheel zoom/pan (#156).
- Added `Ctrl+Wheel` zoom lock and `Alt+Wheel` zoom speed adjustment.

#### 🐞 Fixes & Improvements
- **UI**: Fixed context menu icons and shadow synchronization on Windows 10.
- **UI**: Beautified the progress bar as a capsule and integrated a frame counter (#167).
- **UX**: Refined panning hand cursor hit-testing and logic (#144).
- **HDR**: Resolved peak luminance detection issues by prioritizing physical hardware reports over ICC profiles (#131).
- **Performance**: Migrated to a zero-overhead TraceLogging/ETW structured logging system.
- **Stability**: Fixed ghost image artifacts during Standard to Titan mode transitions.

#### 🏗 Build & CI
- Established a fully automated packaging system using GitHub Actions and Inno Setup.
- Optimized vcpkg multi-tier caching for significantly faster pipeline builds.
- Implemented a WinGet auto-submission workflow with installer validation.

---

### 🤝 Acknowledgments
A special thank you to all users who participated in testing and provided feedback. Every millisecond of QuickView's speed is thanks to your support.
- Special thanks to **@bananakid**, **@PYCHBI**, **@lrbin50**, **@1kari-s**, **@Battler624**, and **@toxieainc** for their bug reports and technical insights.
