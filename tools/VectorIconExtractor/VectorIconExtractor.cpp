/**
 * @file VectorIconExtractor.cpp
 * @brief Utility to extract vector path data from Windows Icon Fonts (Segoe Fluent Icons / MDL2).
 * 
 * This tool takes Unicode codepoints from icon fonts and generates C++ data structures
 * compatible with the QuickView GeekIconLibrary.
 * 
 * Usage: VectorIconExtractor.exe <codepoint[:name]> [codepoint[:name] ...]
 * Example: VectorIconExtractor.exe E711:Close E713:Settings
 * 
 * License: GPL-3.0
 */

#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>
#include <iostream>
#include <vector>
#include <iomanip>
#include <string>
#include <sstream>
#include <algorithm>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "ole32.lib")

using Microsoft::WRL::ComPtr;

struct IconInfo {
    std::string name;
    UINT32 codepoint;
};

class PathCaptureSink : public ID2D1GeometrySink {
public:
    struct Command {
        char type;
        std::vector<D2D1_POINT_2F> points;
    };
    std::vector<Command> commands;

    STDMETHOD_(void, SetFillMode)(D2D1_FILL_MODE fillMode) override {}
    STDMETHOD_(void, SetSegmentFlags)(D2D1_PATH_SEGMENT vertexFlags) override {}
    
    STDMETHOD_(void, BeginFigure)(D2D1_POINT_2F startPoint, D2D1_FIGURE_BEGIN figureBegin) override {
        commands.push_back({'M', {startPoint}});
    }
    
    STDMETHOD_(void, AddLine)(D2D1_POINT_2F point) override {
        commands.push_back({'L', {point}});
    }
    
    STDMETHOD_(void, AddBezier)(const D2D1_BEZIER_SEGMENT* bezier) override {
        commands.push_back({'B', {bezier->point1, bezier->point2, bezier->point3}});
    }
    
    STDMETHOD_(void, AddQuadraticBezier)(const D2D1_QUADRATIC_BEZIER_SEGMENT* bezier) override {
        commands.push_back({'Q', {bezier->point1, bezier->point2}});
    }
    
    STDMETHOD_(void, AddQuadraticBeziers)(const D2D1_QUADRATIC_BEZIER_SEGMENT* beziers, UINT32 beziersCount) override {
        for (UINT32 i = 0; i < beziersCount; i++) AddQuadraticBezier(&beziers[i]);
    }
    
    STDMETHOD_(void, AddLines)(const D2D1_POINT_2F* points, UINT32 pointsCount) override {
        for (UINT32 i = 0; i < pointsCount; i++) AddLine(points[i]);
    }
    
    STDMETHOD_(void, AddBeziers)(const D2D1_BEZIER_SEGMENT* beziers, UINT32 beziersCount) override {
        for (UINT32 i = 0; i < beziersCount; i++) AddBezier(&beziers[i]);
    }
    
    STDMETHOD_(void, AddArc)(const D2D1_ARC_SEGMENT* arc) override {}
    
    STDMETHOD_(void, EndFigure)(D2D1_FIGURE_END figureEnd) override {
        commands.push_back({'Z', {}});
    }
    
    STDMETHOD(Close)() override { return S_OK; }
    STDMETHOD(QueryInterface)(REFIID riid, void** ppvObject) override {
        if (riid == __uuidof(ID2D1GeometrySink) || riid == __uuidof(ID2D1SimplifiedGeometrySink) || riid == __uuidof(IUnknown)) {
            *ppvObject = static_cast<ID2D1GeometrySink*>(this);
            return S_OK;
        }
        return E_NOINTERFACE;
    }
    STDMETHOD_(ULONG, AddRef)() override { return 1; }
    STDMETHOD_(ULONG, Release)() override { return 1; }
};

void ExportIcon(IDWriteFontFace* fontFace, ID2D1Factory* d2dFactory, const IconInfo& icon) {
    UINT16 glyphIndex;
    fontFace->GetGlyphIndices(&icon.codepoint, 1, &glyphIndex);

    if (glyphIndex == 0) {
        std::cerr << "Warning: Glyph not found for codepoint U+" << std::hex << icon.codepoint << std::dec << std::endl;
        return;
    }

    ComPtr<ID2D1PathGeometry> geometry;
    d2dFactory->CreatePathGeometry(&geometry);
    ComPtr<ID2D1GeometrySink> sink;
    geometry->Open(&sink);

    fontFace->GetGlyphRunOutline(1024.0f, &glyphIndex, nullptr, nullptr, 1, FALSE, FALSE, sink.Get());
    sink->Close();

    PathCaptureSink capturer;
    // Simplify to lines and cubics for consistent rendering
    geometry->Simplify(D2D1_GEOMETRY_SIMPLIFICATION_OPTION_CUBICS_AND_LINES, nullptr, &capturer);

    D2D1_RECT_F bounds;
    geometry->GetBounds(nullptr, &bounds);
    
    float width = bounds.right - bounds.left;
    float height = bounds.bottom - bounds.top;
    float size = (std::max)(width, height);
    if (size < 0.1f) size = 1024.0f; // Fallback
    
    float scale = 1.0f / size;

    std::cout << "    // --- " << icon.name << " (\\u" << std::hex << icon.codepoint << std::dec << ") ---" << std::endl;
    std::cout << "    static const IconPathCommand DATA_" << icon.name << "[] = {" << std::endl;
    
    for (const auto& cmd : capturer.commands) {
        if (cmd.type == 'Z') {
            std::cout << "        { 'Z', 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f }," << std::endl;
            continue;
        }
        
        std::cout << "        { '" << cmd.type << "', ";
        for (size_t i = 0; i < 3; i++) {
            if (i < cmd.points.size()) {
                // Normalize to 0.0 - 1.0 range and center
                float x = (cmd.points[i].x - bounds.left) * scale + (1.0f - width * scale) / 2.0f;
                float y = (cmd.points[i].y - bounds.top) * scale + (1.0f - height * scale) / 2.0f;
                std::cout << std::fixed << std::setprecision(4) << x << "f, " << y << "f";
            } else {
                std::cout << "0.0f, 0.0f";
            }
            if (i < 2) std::cout << ", ";
        }
        std::cout << " }," << std::endl;
    }
    
    std::cout << "    };" << std::endl;
    std::cout << "    const VectorIcon " << icon.name << "Vector = { DATA_" << icon.name << ", sizeof(DATA_" << icon.name << ") / sizeof(IconPathCommand) };" << std::endl << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "VectorIconExtractor - QuickView Utility" << std::endl;
        std::cout << "Usage: VectorIconExtractor.exe <codepoint[:name]> [codepoint[:name] ...]" << std::endl;
        std::cout << "Example: VectorIconExtractor.exe E711:Close E713:Settings" << std::endl;
        std::cout << "Default Font: Segoe Fluent Icons (fallback to Segoe MDL2 Assets)" << std::endl;
        return 0;
    }

    std::vector<IconInfo> icons;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        size_t colon = arg.find(':');
        std::string hexPart = (colon == std::string::npos) ? arg : arg.substr(0, colon);
        std::string namePart = (colon == std::string::npos) ? "Icon_" + hexPart : arg.substr(colon + 1);
        
        try {
            UINT32 cp = std::stoul(hexPart, nullptr, 16);
            icons.push_back({namePart, cp});
        } catch (...) {
            std::cerr << "Error parsing codepoint: " << hexPart << std::endl;
        }
    }

    CoInitialize(NULL);
    
    ComPtr<IDWriteFactory> dwFactory;
    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&dwFactory);
    
    ComPtr<IDWriteFontCollection> sysFonts;
    dwFactory->GetSystemFontCollection(&sysFonts);

    const wchar_t* fontName = L"Segoe Fluent Icons";
    UINT32 index; BOOL exists;
    sysFonts->FindFamilyName(fontName, &index, &exists);
    if (!exists) {
        fontName = L"Segoe MDL2 Assets";
        sysFonts->FindFamilyName(fontName, &index, &exists);
    }
    
    if (!exists) {
        std::cerr << "Error: Segoe Fluent Icons or Segoe MDL2 Assets not found." << std::endl;
        CoUninitialize();
        return 1;
    }

    ComPtr<IDWriteFontFamily> family;
    sysFonts->GetFontFamily(index, &family);
    ComPtr<IDWriteFont> font;
    family->GetFirstMatchingFont(DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STRETCH_NORMAL, DWRITE_FONT_STYLE_NORMAL, &font);
    
    ComPtr<IDWriteFontFace> fontFace;
    font->CreateFontFace(&fontFace);

    ComPtr<ID2D1Factory> d2dFactory;
    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory), (void**)&d2dFactory);

    std::cout << "// Generated by VectorIconExtractor" << std::endl;
    std::cout << "// Font Used: " << (fontName == L"Segoe Fluent Icons" ? "Segoe Fluent Icons" : "Segoe MDL2 Assets") << std::endl << std::endl;
    std::cout << "#include \"GeekIconLibrary.h\"" << std::endl << std::endl;
    std::cout << "namespace GeekIcons {" << std::endl << std::endl;

    for (const auto& icon : icons) {
        ExportIcon(fontFace.Get(), d2dFactory.Get(), icon);
    }

    std::cout << "} // namespace GeekIcons" << std::endl;

    CoUninitialize();
    return 0;
}
