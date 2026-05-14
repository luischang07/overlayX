#pragma once

#include <windows.h>
#include <functional>

namespace overlayx
{

  /// Manages the transparent, click-through Win32 overlay window.
  class OverlayWindow
  {
  public:
    using PaintCallback = std::function<void()>;
    static constexpr UINT RenderMessage = WM_APP + 1;

    bool create()
    {
      WNDCLASSEXW wc = {};
      wc.cbSize = sizeof(wc);
      wc.lpfnWndProc = WndProc;
      wc.hInstance = GetModuleHandle(nullptr);
      wc.lpszClassName = L"OverlayXEngineWnd";
      RegisterClassExW(&wc);

      HDC hdc = GetDC(nullptr);
      int screenW = GetDeviceCaps(hdc, DESKTOPHORZRES);
      int screenH = GetDeviceCaps(hdc, DESKTOPVERTRES);
      ReleaseDC(nullptr, hdc);

      m_hwnd = CreateWindowExW(
          WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
          wc.lpszClassName,
          L"OverlayX Engine",
          WS_POPUP,
          0, 0,
          screenW,
          screenH,
          nullptr, nullptr, wc.hInstance, nullptr);

      if (!m_hwnd)
        return false;

      // Store `this` pointer for WndProc
      SetWindowLongPtr(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

      ShowWindow(m_hwnd, SW_SHOWNOACTIVATE);
      return true;
    }

    void startRefreshTimer(UINT intervalMs)
    {
      if (!m_hwnd)
        return;
      if (m_refreshTimerId != 0 && m_refreshIntervalMs == intervalMs)
        return;
      if (m_refreshTimerId != 0)
      {
        KillTimer(m_hwnd, m_refreshTimerId);
        m_refreshTimerId = 0;
      }
      m_refreshIntervalMs = intervalMs;
      m_refreshTimerId = SetTimer(m_hwnd, 1, intervalMs, nullptr);
    }

    void stopRefreshTimer()
    {
      if (m_hwnd && m_refreshTimerId != 0)
      {
        KillTimer(m_hwnd, m_refreshTimerId);
        m_refreshTimerId = 0;
      }
      m_refreshIntervalMs = 0;
    }

    HWND handle() const { return m_hwnd; }

    void setEditMode(bool edit)
    {
      LONG_PTR exStyle = GetWindowLongPtr(m_hwnd, GWL_EXSTYLE);
      if (edit)
      {
        exStyle &= ~WS_EX_TRANSPARENT; // Receive clicks
      }
      else
      {
        exStyle |= WS_EX_TRANSPARENT; // Click-through
      }
      SetWindowLongPtr(m_hwnd, GWL_EXSTYLE, exStyle);
    }

    void invalidate()
    {
      if (m_hwnd)
      {
        PostMessage(m_hwnd, RenderMessage, 0, 0);
      }
    }

    void setPaintCallback(PaintCallback cb) { m_paintCallback = std::move(cb); }

    int runMessageLoop()
    {
      MSG msg = {};
      while (GetMessage(&msg, nullptr, 0, 0))
      {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
      return static_cast<int>(msg.wParam);
    }

  private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
      auto *self = reinterpret_cast<OverlayWindow *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

      switch (msg)
      {
      case WM_TIMER:
      case RenderMessage:
        if (self && self->m_paintCallback)
          self->m_paintCallback();
        return 0;
      case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
      }
      return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    HWND m_hwnd{nullptr};
    PaintCallback m_paintCallback;
    UINT_PTR m_refreshTimerId{0};
    UINT m_refreshIntervalMs{0};
  };

} // namespace overlayx
