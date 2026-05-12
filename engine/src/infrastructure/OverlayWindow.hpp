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

    bool create()
    {
      WNDCLASSEXW wc = {};
      wc.cbSize = sizeof(wc);
      wc.lpfnWndProc = WndProc;
      wc.hInstance = GetModuleHandle(nullptr);
      wc.lpszClassName = L"OverlayXEngineWnd";
      RegisterClassExW(&wc);

      m_hwnd = CreateWindowExW(
          WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
          wc.lpszClassName,
          L"OverlayX Engine",
          WS_POPUP,
          0, 0,
          GetSystemMetrics(SM_CXSCREEN),
          GetSystemMetrics(SM_CYSCREEN),
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
      if (m_refreshTimerId != 0)
      {
        KillTimer(m_hwnd, m_refreshTimerId);
        m_refreshTimerId = 0;
      }
      m_refreshTimerId = SetTimer(m_hwnd, 1, intervalMs, nullptr);
    }

    void stopRefreshTimer()
    {
      if (m_hwnd && m_refreshTimerId != 0)
      {
        KillTimer(m_hwnd, m_refreshTimerId);
        m_refreshTimerId = 0;
      }
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
      InvalidateRect(m_hwnd, nullptr, FALSE);
      // Also post a custom message to wake up the message loop
      PostMessage(m_hwnd, WM_USER + 1, 0, 0);
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
      case WM_USER + 1: // Custom repaint trigger
        if (self && self->m_paintCallback)
          self->m_paintCallback();
        return 0;
      case WM_TIMER:
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
  };

} // namespace overlayx
