#pragma once
#include "pch.h"
#include "GeekIconLibrary.h"

namespace QuickView::UI {

class GeekIconRenderer {
public:
    static void DrawVectorIcon(
        ID2D1RenderTarget* dc,
        const GeekIcons::VectorIcon& icon,
        const D2D1_RECT_F& rect,
        ID2D1Brush* brush,
        float rotationAngle = 0.0f
    );
};

} // namespace QuickView::UI::Menu
