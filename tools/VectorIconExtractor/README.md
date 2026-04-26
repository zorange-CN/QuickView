# VectorIconExtractor

A universal utility to extract vector path data from Windows icon fonts (e.g., Segoe Fluent Icons) and generate C++ code.

## Features

- **Fully Automated**: Directly converts Unicode codepoints into C++ structures with normalized 1x1 coordinates.
- **Auto-Alignment**: Extracted paths are automatically scaled and centered to ensure consistent appearance in the UI.
- **Batch Processing**: Supports passing multiple codepoints in a single execution.
- **Name Mapping**: Supports assigning human-readable names to codepoints (e.g., `E711:Close`).

## How to Build

Run the following command in a **Visual Studio Developer Command Prompt**:

```powershell
cl.exe /O2 /std:c++20 /EHsc VectorIconExtractor.cpp /Fe:VectorIconExtractor.exe
```

## Usage

### Basic Usage

Pass hex codepoints directly:

```powershell
.\VectorIconExtractor.exe E711 E713 E74D
```

### With Custom Names (Recommended)

Use the `:` separator to map codepoints to names:

```powershell
.\VectorIconExtractor.exe E711:Close E713:Settings E74D:Delete > IconsOutput.h
```

The generated code will look like this:

```cpp
namespace GeekIcons {
    // --- Close (\uE711) ---
    static const IconPathCommand DATA_Close[] = {
        { 'M', 0.5000f, 0.1250f, 0.0000f, 0.0000f, 0.0000f, 0.0000f },
        ...
    };
    const VectorIcon CloseVector = { DATA_Close, 12 };
}
```

## Notes

1. The tool prioritizes `Segoe Fluent Icons` (Windows 11) and falls back to `Segoe MDL2 Assets` (Windows 10) if the former is not found.
2. The generated code is designed to be placed directly into the project's `GeekIconData.h`.
3. Ensure you have the necessary fonts installed on your system.
