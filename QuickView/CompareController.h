#pragma once

#include <windows.h>
#include <d2d1_2.h>
#include <optional>
#include <string>
#include <functional>
#include "AppContext.h"
#include "CoroutineTypes.h"

class CompareController {
public:
    explicit CompareController(AppContext& context);
    ~CompareController() = default;

    std::optional<LRESULT> HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    bool RenderComposite(HWND hwnd);
    void MarkDirty();
    void EnterMode(HWND hwnd);
    void ExitMode(HWND hwnd);
    bool IsActive() const;
    
    ComparePane HitTest(HWND hwnd, POINT ptClient) const;
    D2D1_RECT_F GetViewport(HWND hwnd, ComparePane pane) const;
    float GetSplitRatio() const;

    void CaptureCurrentImageAsLeft();
    FireAndForget LoadImageIntoLeftSlot(HWND hwnd, std::wstring path, std::function<void(bool)> callback = nullptr);
    void ReloadPaneForDisplayChange(HWND hwnd, ComparePane pane);
    
    void UpdateRawButton();
    bool GetPaneRawState(ComparePane pane, bool& isRaw, bool& isFullDecode) const;
    void RefreshRawUI(HWND hwnd);
    void CenterDialogOnPaneIfNeeded(HWND hwnd, ComparePane pane);
    void ApplyZoomStep(HWND hwnd, float delta, bool fineInterval);

private:
    AppContext& m_context;

    // Internal message handlers
    std::optional<LRESULT> OnLButtonDown(HWND hwnd, int x, int y);
    std::optional<LRESULT> OnLButtonUp(HWND hwnd, int x, int y);
    std::optional<LRESULT> OnMouseMove(HWND hwnd, int x, int y);
    std::optional<LRESULT> OnKeyDown(HWND hwnd, WPARAM key);
};
