#include "pch.h"
#include "ZoomAnimation.h"
#include "QuickView.h"
#include "PaneContext.h"
#include "RenderEngine.h"

extern std::atomic<uint64_t> g_currentImageId;
extern std::unique_ptr<CRenderEngine> g_compEngine;
extern PaneContext g_panes[2];
extern bool StepRectTowardTarget(RECT& current, const RECT& target, float alpha);

SmoothZoomController::SmoothZoomController(AppContext& context) : m_context(context) {}

bool SmoothZoomController::IsActive() const {
    return m_context.SmoothZoom.Active;
}

void SmoothZoomController::Reset() {
    m_context.SmoothZoom.Reset();
}


void SmoothZoomController::ResolvePan(HWND hwnd, float zoom, float& outPanX, float& outPanY) const {
    RECT rc{};
    GetClientRect(hwnd, &rc);
    const float winW = (float)rc.right;
    const float winH = (float)rc.bottom;

    POINT anchorClient = m_context.SmoothZoom.AnchorScreenPt;
    ScreenToClient(hwnd, &anchorClient);

    const float dx = (float)anchorClient.x - winW * 0.5f;
    const float dy = (float)anchorClient.y - winH * 0.5f;
    outPanX = dx - zoom * m_context.SmoothZoom.AnchorImageX;
    outPanY = dy - zoom * m_context.SmoothZoom.AnchorImageY;
}


