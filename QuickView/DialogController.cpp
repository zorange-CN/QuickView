#include "CompareController.h"
#include "pch.h"
#include "DialogController.h"
#include "QuickView.h"
#include "PaneContext.h"
#include "RenderEngine.h"

extern float g_uiScale;
extern void RequestRepaint(QuickView::PaintLayer layer);
extern void AdjustWindowForOverlay(HWND hwnd, bool animate);
extern void EnsureWindowSizeForDialog(HWND hwnd);

LRESULT CALLBACK DialogEditSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    DialogState& dialog = AppContext::GetInstance().Dialog;
    if (uMsg == WM_KEYDOWN) {
        if (wParam == VK_RETURN) {
            int len = GetWindowTextLengthW(hWnd);
            if (len > 0) {
                std::vector<wchar_t> buf(len + 1);
                GetWindowTextW(hWnd, buf.data(), len + 1);
                dialog.InputText = buf.data();
                dialog.FinalResult = DialogResult::Yes;
            } else {
                dialog.FinalResult = DialogResult::None;
            }
            dialog.IsVisible = false;
            return 0;
        }
        else if (wParam == VK_ESCAPE) {
            dialog.FinalResult = DialogResult::None;
            dialog.IsVisible = false;
            return 0;
        }
        else if (wParam == 'A' && GetKeyState(VK_CONTROL) < 0) {
            SendMessage(hWnd, EM_SETSEL, 0, -1);
            return 0;
        }
    }
    else if (uMsg == WM_CHAR) {
        if (wParam == VK_RETURN || wParam == VK_ESCAPE) return 0;
    }
    
    return CallWindowProc(dialog.oldEditProc, hWnd, uMsg, wParam, lParam);
}

DialogController::DialogController(AppContext& context) : m_context(context) {}

bool DialogController::IsVisible() const {
    return m_context.Dialog.IsVisible;
}

void DialogController::MarkDirty() {
    RequestRepaint(QuickView::PaintLayer::Dynamic);
}

static DialogLayout CalculateDialogLayoutInternal(D2D1_SIZE_F size, DialogState& dialog) {
    DialogLayout layout;
    float cx = size.width / 2.0f;
    float cy = size.height / 2.0f;
    
    if (dialog.UseCustomCenter) {
        cx = dialog.CustomCenter.x;
        cy = dialog.CustomCenter.y;
    }
    
    float boxWidth = 400.0f * g_uiScale;
    float baseBoxHeight = 220.0f * g_uiScale;
    if (dialog.HasCheckbox) baseBoxHeight += 40.0f * g_uiScale;
    if (dialog.HasInput) baseBoxHeight += 50.0f * g_uiScale;
    if (!dialog.QualityText.empty()) baseBoxHeight += 20.0f * g_uiScale;
    
    layout.Box = D2D1::RectF(cx - boxWidth/2, cy - baseBoxHeight/2, cx + boxWidth/2, cy + baseBoxHeight/2);
    
    float contentTop = layout.Box.top + 100.0f * g_uiScale;
    if (!dialog.QualityText.empty()) contentTop += 20.0f * g_uiScale;
    
    if (dialog.HasInput) {
        layout.Input = D2D1::RectF(layout.Box.left + 30.0f * g_uiScale, contentTop, layout.Box.right - 30.0f * g_uiScale, contentTop + 36.0f * g_uiScale);
        contentTop += 50.0f * g_uiScale;
    }
    
    if (dialog.HasCheckbox) {
        layout.Checkbox = D2D1::RectF(layout.Box.left + 30.0f * g_uiScale, contentTop, layout.Box.left + 50.0f * g_uiScale, contentTop + 20.0f * g_uiScale);
        contentTop += 40.0f * g_uiScale;
    }
    
    float buttonAreaY = layout.Box.bottom - 60.0f * g_uiScale;
    float buttonWidth = 100.0f * g_uiScale;
    float buttonHeight = 36.0f * g_uiScale;
    float spacing = 15.0f * g_uiScale;
    int btnCount = (int)dialog.Buttons.size();
    float totalBtnWidth = btnCount * buttonWidth + (btnCount - 1) * spacing;
    float startX = cx - totalBtnWidth / 2.0f;
    
    for (int i = 0; i < btnCount; ++i) {
        layout.Buttons.push_back(D2D1::RectF(startX, buttonAreaY, startX + buttonWidth, buttonAreaY + buttonHeight));
        startX += buttonWidth + spacing;
    }
    return layout;
}

static void CreateDialogInputInternal(HWND parent, DialogState& dialog) {
    if (!dialog.HasInput || dialog.hEdit) return;
    RECT clientRect; GetClientRect(parent, &clientRect);
    D2D1_SIZE_F size = D2D1::SizeF((float)(clientRect.right - clientRect.left), (float)(clientRect.bottom - clientRect.top));
    DialogLayout layout = CalculateDialogLayoutInternal(size, dialog);
    
    int x = (int)layout.Input.left;
    int y = (int)layout.Input.top;
    int w = (int)(layout.Input.right - layout.Input.left);
    int h = (int)(layout.Input.bottom - layout.Input.top);
    
    POINT pt{ x, y };
    ClientToScreen(parent, &pt);
    x = pt.x; y = pt.y;
    
    dialog.hInputHost = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, L"QuickViewInputHost", L"", 
        WS_POPUP | WS_VISIBLE, x, y, w, h, parent, nullptr, GetModuleHandle(nullptr), nullptr);
        
    if (dialog.hInputHost) {
        RECT rcHost; GetClientRect(dialog.hInputHost, &rcHost);
        dialog.hEdit = CreateWindowExW(0, L"EDIT", dialog.InputText.c_str(),
            WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL,
            0, 0, rcHost.right, rcHost.bottom,
            dialog.hInputHost, nullptr, GetModuleHandle(nullptr), nullptr);
            
        if (dialog.hEdit) {
          int fontHeight = (int)(22 * g_uiScale);
          HFONT hFont = CreateFontW(
              fontHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
              DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
          SendMessage(dialog.hEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
          dialog.oldEditProc = (WNDPROC)SetWindowLongPtr(
              dialog.hEdit, GWLP_WNDPROC, (LONG_PTR)DialogEditSubclassProc);
          SetFocus(dialog.hEdit);
          SendMessage(dialog.hEdit, EM_SETSEL, 0, -1);
        }
    }
}

static void DestroyDialogInputInternal(DialogState& dialog) {
    if (dialog.hEdit) {
        if (dialog.oldEditProc) {
            SetWindowLongPtr(dialog.hEdit, GWLP_WNDPROC, (LONG_PTR)dialog.oldEditProc);
        }
        dialog.hEdit = nullptr;
    }
    if (dialog.hInputHost) {
        DestroyWindow(dialog.hInputHost);
        dialog.hInputHost = nullptr;
    }
}

DialogResult DialogController::ShowDialog(HWND hwnd, const std::wstring& title, const std::wstring& messageContent,
                        D2D1_COLOR_F accentColor, const std::vector<DialogButton>& buttons,
                        bool hasCheckbox, const std::wstring& checkboxText, const std::wstring& qualityText)
{
    m_context.Dialog.IsVisible = true;
    m_context.Dialog.Title = title;
    m_context.Dialog.Message = messageContent;
    m_context.Dialog.QualityText = qualityText;
    m_context.Dialog.AccentColor = accentColor;
    m_context.Dialog.Buttons = buttons;
    m_context.Dialog.SelectedButtonIndex = 0;
    m_context.Dialog.HasCheckbox = hasCheckbox;
    m_context.Dialog.CheckboxText = checkboxText;
    m_context.Dialog.IsChecked = false;
    
    // Reset Input Mode for standard dialog
    if (!m_context.Dialog.HasInput) {
        m_context.Dialog.HasInput = false;
        m_context.Dialog.hEdit = nullptr;
    }
    m_context.Dialog.FinalResult = DialogResult::None;
    
    EnsureWindowSizeForDialog(hwnd);
    
    if (m_context.Dialog.HasInput) {
        CreateDialogInputInternal(hwnd, m_context.Dialog);
    }
    
    RequestRepaint(QuickView::PaintLayer::Dynamic);
    UpdateWindow(hwnd); 
    
    MSG msgStruct;
    while (m_context.Dialog.IsVisible && GetMessage(&msgStruct, NULL, 0, 0)) {
        TranslateMessage(&msgStruct);
        DispatchMessage(&msgStruct);
    }
    
    if (m_context.Dialog.HasInput) {
        DestroyDialogInputInternal(m_context.Dialog);
    }
    
    RequestRepaint(QuickView::PaintLayer::Dynamic);
    if (!IsCompareModeActive()) {
        AdjustWindowForOverlay(hwnd, true);
    }
    return m_context.Dialog.FinalResult;
}

std::optional<LRESULT> DialogController::HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (!IsVisible()) return std::nullopt;

    switch (message) {
        case WM_KEYDOWN:
            return OnKeyDown(hwnd, wParam);
        case WM_LBUTTONDOWN: {
            int x = (short)LOWORD(lParam);
            int y = (short)HIWORD(lParam);
            return OnLButtonDown(hwnd, x, y);
        }
        case WM_MOUSEMOVE:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_MOUSEWHEEL:
        case WM_LBUTTONDBLCLK:
            // Swallow mouse interactions to background when dialog is open
            SetCursor(LoadCursor(nullptr, IDC_ARROW));
            return 0;
    }
    
    // Default: if dialog is open, swallow keyboard events not handled above
    if (message >= WM_KEYFIRST && message <= WM_KEYLAST) {
        return 0;
    }

    return std::nullopt;
}

std::optional<LRESULT> DialogController::OnKeyDown([[maybe_unused]] HWND hwnd, WPARAM key) {
    if (key == VK_ESCAPE) {
        m_context.Dialog.FinalResult = DialogResult::None;
        m_context.Dialog.IsVisible = false;
        return 0;
    }
    
    if (m_context.Dialog.HasInput) {
        if (key == VK_TAB) return 0; // Swallow tab
        return std::nullopt; // Let input control handle text
    }
    
    // Standard dialog key handling
    if (key == VK_LEFT) {
        if (m_context.Dialog.SelectedButtonIndex > 0) m_context.Dialog.SelectedButtonIndex--;
        MarkDirty();
        return 0;
    } else if (key == VK_RIGHT) {
        if (m_context.Dialog.SelectedButtonIndex < static_cast<int>(m_context.Dialog.Buttons.size()) - 1) m_context.Dialog.SelectedButtonIndex++;
        MarkDirty();
        return 0;
    } else if (key == VK_TAB || key == VK_SPACE) { 
        if (m_context.Dialog.HasCheckbox) {
            m_context.Dialog.IsChecked = !m_context.Dialog.IsChecked;
            MarkDirty();
        }
        return 0;
    } else if (key == VK_RETURN) {
        m_context.Dialog.FinalResult = m_context.Dialog.Buttons[m_context.Dialog.SelectedButtonIndex].Result;
        m_context.Dialog.IsVisible = false;
        return 0;
    }
    return 0; // Swallow other keys
}

std::optional<LRESULT> DialogController::OnLButtonDown(HWND hwnd, int x, int y) {
    RECT clientRect; GetClientRect(hwnd, &clientRect);
    D2D1_SIZE_F size = D2D1::SizeF((float)(clientRect.right - clientRect.left), (float)(clientRect.bottom - clientRect.top));
    DialogLayout layout = CalculateDialogLayoutInternal(size, m_context.Dialog);
    
    float mouseX = (float)x;
    float mouseY = (float)y;
    
    if (m_context.Dialog.HasCheckbox) {
        if (mouseX >= layout.Checkbox.left - 10 && mouseX <= layout.Box.right - 20 &&
            mouseY >= layout.Checkbox.top - 10 && mouseY <= layout.Checkbox.bottom + 10) {
            m_context.Dialog.IsChecked = !m_context.Dialog.IsChecked;
            MarkDirty();
            return 0;
        }
    }
    
    for (size_t i = 0; i < layout.Buttons.size(); ++i) {
        if (mouseX >= layout.Buttons[i].left && mouseX <= layout.Buttons[i].right &&
            mouseY >= layout.Buttons[i].top && mouseY <= layout.Buttons[i].bottom) {
            
            if (m_context.Dialog.HasInput && i == 0) {
                 int len = GetWindowTextLengthW(m_context.Dialog.hEdit);
                 if (len > 0) {
                    std::vector<wchar_t> buf(len + 1);
                    GetWindowTextW(m_context.Dialog.hEdit, buf.data(), len + 1);
                    m_context.Dialog.InputText = buf.data();
                    m_context.Dialog.FinalResult = DialogResult::Yes;
                 } else {
                    m_context.Dialog.FinalResult = DialogResult::None;
                 }
            } else {
                m_context.Dialog.FinalResult = m_context.Dialog.Buttons[i].Result;
            }
            m_context.Dialog.IsVisible = false;
            return 0;
        }
    }
    return 0; // Swallow
}


std::wstring DialogController::ShowInputDialog(HWND hwnd, const std::wstring& title, const std::wstring& message, const std::wstring& initialText) 
{
    m_context.Dialog.IsVisible = true;
    m_context.Dialog.Title = title;
    m_context.Dialog.Message = message;
    m_context.Dialog.QualityText.clear();
    m_context.Dialog.AccentColor = D2D1::ColorF(D2D1::ColorF::Orange); 
    m_context.Dialog.Buttons = { { DialogResult::Yes, L"Rename", true }, { DialogResult::None, L"Cancel" } };
    m_context.Dialog.SelectedButtonIndex = 0;
    m_context.Dialog.HasCheckbox = false;
    m_context.Dialog.HasInput = true;
    m_context.Dialog.InputText = initialText;
    m_context.Dialog.FinalResult = DialogResult::None;
    
    EnsureWindowSizeForDialog(hwnd);
    CreateDialogInputInternal(hwnd, m_context.Dialog);
    
    RequestRepaint(QuickView::PaintLayer::Dynamic);
    UpdateWindow(hwnd); 
    
    MSG msgStruct;
    SetCursor(LoadCursor(nullptr, IDC_ARROW));

    while (m_context.Dialog.IsVisible && GetMessage(&msgStruct, NULL, 0, 0)) {
        TranslateMessage(&msgStruct);
        DispatchMessage(&msgStruct);
    }
    
    DestroyDialogInputInternal(m_context.Dialog);
    RequestRepaint(QuickView::PaintLayer::Dynamic);
    if (!IsCompareModeActive()) {
        AdjustWindowForOverlay(hwnd, true);
    }
    
    if (m_context.Dialog.FinalResult == DialogResult::Yes) {
        return m_context.Dialog.InputText;
    }
    return L"";
}
