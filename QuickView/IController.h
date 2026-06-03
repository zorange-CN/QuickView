#pragma once
#include <windows.h>
#include <d2d1_2.h>
#include <optional>

class IController {
public:
    virtual ~IController() = default;
    
    // Returns whether this controller is active.
    virtual bool IsActive() const = 0;
    
    // Processes Win32 messages. If handled, returns std::optional containing the LRESULT result.
    virtual std::optional<LRESULT> HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) = 0;
    
    // Render lifecycle method
    virtual void Render(ID2D1DeviceContext* ctx) = 0;
};
