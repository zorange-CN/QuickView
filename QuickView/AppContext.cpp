#include "CompareController.h"
#include "DialogController.h"
#include "pch.h"
#include "AppContext.h"

void SmoothZoomState::Reset() {
    Active = false;
    ImageId = 0;
    AnimateWindow = false;
    HasAnchor = false;
    AnchorScreenPt = { 0, 0 };
    SourceWindowRect = { 0, 0, 0, 0 };
    TargetWindowRect = { 0, 0, 0, 0 };
    SourceZoom = 1.0f;
    CurrentZoom = 1.0f;
    CurrentPanX = 0.0f;
    CurrentPanY = 0.0f;
    TargetZoom = 1.0f;
    TargetPanX = 0.0f;
    TargetPanY = 0.0f;
    AnchorImageX = 0.0f;
    AnchorImageY = 0.0f;
    LastWinW = 0.0f;
    LastWinH = 0.0f;
    LastTick = 0;
}

AppContext& AppContext::GetInstance() {
    static AppContext instance;
    return instance;
}

AppContext::AppContext() {
    CompareCtrl = std::make_unique<CompareController>(*this);
    DialogCtrl = std::make_unique<DialogController>(*this);
}

AppContext::~AppContext() = default;

// Define global metrics
#include "DebugMetrics.h"
DebugMetrics g_debugMetrics{};
