#pragma once

#include <windows.h>
#include <d2d1_2.h>
#include "AppContext.h"

class SmoothZoomController {
public:
    explicit SmoothZoomController(AppContext& context);
    ~SmoothZoomController() = default;

    void Reset();
    void Configure(HWND hwnd,
                   float sourceZoom,
                   float sourcePanX,
                   float sourcePanY,
                   float targetZoom,
                   float targetPanX,
                   float targetPanY,
                   const POINT* anchorScreenPt,
                   bool animateWindow,
                   const RECT* targetWindowRect);
    void SyncToLogical(float winW, float winH, bool activate);
    bool Tick(HWND hwnd);

    void ResolvePan(HWND hwnd, float zoom, float& outPanX, float& outPanY) const;
    bool IsActive() const;
    
private:
    AppContext& m_context;
};
