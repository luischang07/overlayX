#pragma once

#include "interfaces/IRenderer.hpp"
#include "infrastructure/SpikeDetector.hpp"
#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <cmath>
#include <chrono>
#include <iomanip>
#include <sstream>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

namespace overlayx
{

  /// Direct2D renderer for drawing crosshair shapes onto a layered window.
  class Direct2DRenderer final : public IRenderer
  {
  public:
    ~Direct2DRenderer() override { cleanup(); }

    bool initialize(void *windowHandle) override
    {
      m_hwnd = static_cast<HWND>(windowHandle);
      HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_factory);
      if (FAILED(hr))
        return false;

      // Create DWrite factory for text rendering
      hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown **)&m_dwriteFactory);
      if (FAILED(hr))
        return false;

      // DC render target for use with UpdateLayeredWindow
      D2D1_RENDER_TARGET_PROPERTIES props;
      props.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
      props.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
      props.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
      props.dpiX = 0;
      props.dpiY = 0;
      props.usage = D2D1_RENDER_TARGET_USAGE_NONE;
      props.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;

      hr = m_factory->CreateDCRenderTarget(&props, &m_renderTarget);
      return SUCCEEDED(hr);
    }

    void render(const OverlayConfig &config) override
    {
      if (!m_renderTarget || !m_hwnd)
        return;

      HDC hdc_screen = GetDC(NULL);
      int screenW = GetDeviceCaps(hdc_screen, DESKTOPHORZRES);
      int screenH = GetDeviceCaps(hdc_screen, DESKTOPVERTRES);
      ReleaseDC(NULL, hdc_screen);

      // Update SpikeDetector with this overlay window's top-left screen offset
      if (m_hwnd)
      {
        RECT wr = {};
        if (GetWindowRect(m_hwnd, &wr))
        {

          // Try to get DPI for the window (Windows 10+)
          UINT dpi = 0;
          HMODULE shcore = LoadLibraryA("Shcore.dll");
          if (shcore)
          {
            typedef HRESULT(WINAPI *GetDpiForMonitor_t)(HMONITOR, int, UINT*, UINT*);
            GetDpiForMonitor_t f = (GetDpiForMonitor_t)GetProcAddress(shcore, "GetDpiForMonitor");
            if (f)
            {
              HMONITOR hm = MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTONEAREST);
              UINT dpiX = 0, dpiY = 0;
              if (SUCCEEDED(f(hm, 0 /*MDT_EFFECTIVE_DPI*/, &dpiX, &dpiY)))
                dpi = dpiX;
            }
            FreeLibrary(shcore);
          }
          if (dpi == 0)
          {
            HDC dc = GetDC(m_hwnd);
            dpi = GetDeviceCaps(dc, LOGPIXELSX);
            ReleaseDC(m_hwnd, dc);
          }

          // setOverlayOffset expects screen coords of window top-left
          SpikeDetector::setOverlayOffset(wr.left, wr.top);
        }
      }

      // Create a memory DC and 32-bit ARGB DIB
      HDC screenDC = GetDC(nullptr);
      HDC memDC = CreateCompatibleDC(screenDC);

      BITMAPINFO bmi = {};
      bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
      bmi.bmiHeader.biWidth = screenW;
      bmi.bmiHeader.biHeight = -screenH; // top-down
      bmi.bmiHeader.biPlanes = 1;
      bmi.bmiHeader.biBitCount = 32;
      bmi.bmiHeader.biCompression = BI_RGB;

      void *bits = nullptr;
      HBITMAP hBitmap = CreateDIBSection(memDC, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
      HGDIOBJ oldBitmap = SelectObject(memDC, hBitmap);

      // Bind D2D to the memory DC
      RECT rc = {0, 0, screenW, screenH};
      m_renderTarget->BindDC(memDC, &rc);

      m_renderTarget->BeginDraw();
      D2D1_COLOR_F clearColor;
      clearColor.r = 0.0f;
      clearColor.g = 0.0f;
      clearColor.b = 0.0f;
      clearColor.a = 0.0f;
      m_renderTarget->Clear(clearColor); // fully transparent

      if (config.visible)
      {
        for (const auto &layer : config.layers)
        {
          if (!layer.enabled)
            continue;

          // Create brush with layer color
          ID2D1SolidColorBrush *brush = nullptr;
          D2D1_COLOR_F layerColor;
          layerColor.r = layer.color.r / 255.0f;
          layerColor.g = layer.color.g / 255.0f;
          layerColor.b = layer.color.b / 255.0f;
          layerColor.a = layer.color.a / 255.0f;
          m_renderTarget->CreateSolidColorBrush(layerColor, &brush);

          ID2D1SolidColorBrush *outlineBrush = nullptr;
          if (layer.outlineEnabled)
          {
            D2D1_COLOR_F outlineColor;
            outlineColor.r = layer.outlineColor.r / 255.0f;
            outlineColor.g = layer.outlineColor.g / 255.0f;
            outlineColor.b = layer.outlineColor.b / 255.0f;
            outlineColor.a = layer.outlineColor.a / 255.0f;
            m_renderTarget->CreateSolidColorBrush(outlineColor, &outlineBrush);
          }

          if (brush)
          {
            float cx = (layer.posX * screenW);
            float cy = (layer.posY * screenH);

            if (layer.rotation != 0.0f)
            {
              D2D1_POINT_2F rotationCenter;
              rotationCenter.x = cx;
              rotationCenter.y = cy;
              D2D1_MATRIX_3X2_F rotationMatrix = D2D1::Matrix3x2F::Rotation(layer.rotation, rotationCenter);
              m_renderTarget->SetTransform(rotationMatrix);
            }
            else
            {
              D2D1_MATRIX_3X2_F identityMatrix = D2D1::Matrix3x2F::Identity();
              m_renderTarget->SetTransform(identityMatrix);
            }

            switch (layer.shape)
            {
            case ShapeType::Cross:
              drawCross(cx, cy, layer.width, layer.height, layer.thickness, layer.gap,
                        brush, outlineBrush, layer.outlineThickness);
              break;
            case ShapeType::Dot:
              drawDot(cx, cy, layer.width, layer.height,
                      brush, outlineBrush, layer.outlineThickness);
              break;
            case ShapeType::Circle:
              drawCircle(cx, cy, layer.width, layer.height, layer.thickness,
                         brush, outlineBrush, layer.outlineThickness);
              break;
            }

            // Reset transform
            D2D1_MATRIX_3X2_F identityMatrix = D2D1::Matrix3x2F::Identity();
            m_renderTarget->SetTransform(identityMatrix);

            brush->Release();
            if (outlineBrush)
              outlineBrush->Release();
          }
        }
      }

      // Render countdown if visible and DWrite available
      if (config.countdown.enabled && m_dwriteFactory)
      {
        drawCountdown(screenW, screenH, config.countdown, config.countdownRuntime);
      }

#ifdef SPIKE_DETECTION_ENABLED
      drawDetectionRegion(screenW, screenH);
#endif

      m_renderTarget->EndDraw();

      // Update the layered window with per-pixel alpha
      POINT ptSrc = {0, 0};
      POINT ptDst = {0, 0};
      SIZE sz = {screenW, screenH};
      BLENDFUNCTION blend = {};
      blend.BlendOp = AC_SRC_OVER;
      blend.SourceConstantAlpha = 255;
      blend.AlphaFormat = AC_SRC_ALPHA;

      UpdateLayeredWindow(m_hwnd, screenDC, &ptDst, &sz, memDC, &ptSrc, 0, &blend, ULW_ALPHA);

      // Cleanup GDI resources
      SelectObject(memDC, oldBitmap);
      DeleteObject(hBitmap);
      DeleteDC(memDC);
      ReleaseDC(nullptr, screenDC);
    }

    void resize(int, int) override
    {
      // Full-screen overlay — resize handled dynamically in render()
    }

    void cleanup() override
    {
      if (m_renderTarget)
      {
        m_renderTarget->Release();
        m_renderTarget = nullptr;
      }
      if (m_dwriteFactory)
      {
        m_dwriteFactory->Release();
        m_dwriteFactory = nullptr;
      }
      if (m_textFormat)
      {
        m_textFormat->Release();
        m_textFormat = nullptr;
      }
      if (m_msTextFormat)
      {
        m_msTextFormat->Release();
        m_msTextFormat = nullptr;
      }
      if (m_factory)
      {
        m_factory->Release();
        m_factory = nullptr;
      }
    }

  private:
    void drawCross(float cx, float cy, float w, float h, float thickness, float gap,
                   ID2D1SolidColorBrush *brush, ID2D1SolidColorBrush *outlineBrush = nullptr, float outlineThickness = 0.0f)
    {
      float halfW = w / 2.0f;
      float halfH = h / 2.0f;
      float halfT = thickness / 2.0f;

      D2D1_RECT_F bars[4];
      bars[0].left = cx - halfT;
      bars[0].top = cy - halfH;
      bars[0].right = cx + halfT;
      bars[0].bottom = cy - gap;

      bars[1].left = cx - halfT;
      bars[1].top = cy + gap;
      bars[1].right = cx + halfT;
      bars[1].bottom = cy + halfH;

      bars[2].left = cx - halfW;
      bars[2].top = cy - halfT;
      bars[2].right = cx - gap;
      bars[2].bottom = cy + halfT;

      bars[3].left = cx + gap;
      bars[3].top = cy - halfT;
      bars[3].right = cx + halfW;
      bars[3].bottom = cy + halfT;

      if (outlineBrush && outlineThickness > 0)
      {
        for (int i = 0; i < 4; ++i)
        {
          D2D1_RECT_F outlineRect = bars[i];
          outlineRect.left -= outlineThickness;
          outlineRect.top -= outlineThickness;
          outlineRect.right += outlineThickness;
          outlineRect.bottom += outlineThickness;
          m_renderTarget->FillRectangle(outlineRect, outlineBrush);
        }
      }

      for (int i = 0; i < 4; ++i)
      {
        m_renderTarget->FillRectangle(bars[i], brush);
      }
    }

    void drawDot(float cx, float cy, float w, float h, ID2D1SolidColorBrush *brush,
                 ID2D1SolidColorBrush *outlineBrush = nullptr, float outlineThickness = 0.0f)
    {
      float rx = w / 2.0f;
      float ry = h / 2.0f;

      D2D1_ELLIPSE ellipse;
      ellipse.point.x = cx;
      ellipse.point.y = cy;
      ellipse.radiusX = rx;
      ellipse.radiusY = ry;

      if (outlineBrush && outlineThickness > 0)
      {
        D2D1_ELLIPSE outlineEllipse;
        outlineEllipse.point.x = cx;
        outlineEllipse.point.y = cy;
        outlineEllipse.radiusX = rx + outlineThickness;
        outlineEllipse.radiusY = ry + outlineThickness;
        m_renderTarget->FillEllipse(outlineEllipse, outlineBrush);
      }

      m_renderTarget->FillEllipse(ellipse, brush);
    }

    void drawCircle(float cx, float cy, float w, float h, float thickness,
                    ID2D1SolidColorBrush *brush, ID2D1SolidColorBrush *outlineBrush = nullptr, float outlineThickness = 0.0f)
    {
      float rx = w / 2.0f;
      float ry = h / 2.0f;

      D2D1_ELLIPSE ellipse;
      ellipse.point.x = cx;
      ellipse.point.y = cy;
      ellipse.radiusX = rx;
      ellipse.radiusY = ry;

      if (outlineBrush && outlineThickness > 0)
      {
        // Outline on a circle (outline of a stroke)
        // We draw a slightly thicker circle behind or just two concentric strokes.
        // Best is to draw the outline with (thickness + 2*outlineThickness)
        m_renderTarget->DrawEllipse(ellipse, outlineBrush, thickness + (outlineThickness * 2.0f));
      }

      m_renderTarget->DrawEllipse(ellipse, brush, thickness);
    }

    void drawCountdown(int screenW, int screenH, const CountdownConfig &config, const CountdownRuntime &runtime)
    {
      if (!m_dwriteFactory)
        return;

      // Recreate text format if font size changed
      if (m_lastFontSize != static_cast<float>(config.fontSize))
      {
        if (m_textFormat)
        {
          m_textFormat->Release();
          m_textFormat = nullptr;
        }
        if (m_msTextFormat)
        {
          m_msTextFormat->Release();
          m_msTextFormat = nullptr;
        }
        HRESULT hr = m_dwriteFactory->CreateTextFormat(
            L"Segoe UI",
            nullptr,
            DWRITE_FONT_WEIGHT_BOLD,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            static_cast<FLOAT>(config.fontSize),
            L"en-us",
            &m_textFormat);
        if (SUCCEEDED(hr) && m_textFormat)
        {
          m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
          m_textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }

        FLOAT msFontSize = std::max<FLOAT>(8.0f, static_cast<FLOAT>(config.fontSize) * 0.5f);
        hr = m_dwriteFactory->CreateTextFormat(
            L"Segoe UI",
            nullptr,
            DWRITE_FONT_WEIGHT_BOLD,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            msFontSize,
            L"en-us",
            &m_msTextFormat);
        if (SUCCEEDED(hr) && m_msTextFormat)
        {
          m_msTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
          m_msTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }

        m_lastFontSize = static_cast<float>(config.fontSize);
      }

      // Determine remaining time based on runtime control fields
      long long remainingMs = static_cast<long long>(config.duration) * 1000LL;
      if (runtime.enabled && runtime.startTimestampMs > 0)
      {
        using namespace std::chrono;
        long long nowMs = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        long long elapsed = nowMs - runtime.startTimestampMs;
        remainingMs = static_cast<long long>(config.duration) * 1000LL - elapsed;
        if (remainingMs < 0)
          remainingMs = 0;
      }
      else if (runtime.pausedRemainingMs >= 0)
      {
        remainingMs = runtime.pausedRemainingMs;
      }
      int remainingSeconds = static_cast<int>((remainingMs + 999) / 1000); // Round up
      int remainingCentiseconds = static_cast<int>((remainingMs % 1000) / 10);
      int remainingMinutes = remainingSeconds / 60;
      int remainingWholeSeconds = remainingSeconds % 60;

      wchar_t secondsBuffer[32];
      wchar_t msBuffer[8];
      swprintf_s(secondsBuffer, L"%02d:%02d", remainingMinutes, remainingWholeSeconds);
      swprintf_s(msBuffer, L".%02d", remainingCentiseconds);

      ID2D1SolidColorBrush *brush = nullptr;
      D2D1_COLOR_F color;
      color.r = config.color.r / 255.0f;
      color.g = config.color.g / 255.0f;
      color.b = config.color.b / 255.0f;
      color.a = config.color.a / 255.0f;
      m_renderTarget->CreateSolidColorBrush(color, &brush);

      if (brush && m_textFormat)
      {
        float x = config.posX * screenW;
        float y = config.posY * screenH;
        float textWidth = static_cast<float>(std::max<int>(160, config.fontSize * 6));
        float textHeight = static_cast<float>(std::max<int>(72, config.fontSize * 2));

        D2D1_RECT_F secondsRect;
        secondsRect.left = x - (textWidth * 0.5f);
        secondsRect.top = y - (textHeight * 0.5f);
        secondsRect.right = x + (textWidth * 0.5f);
        secondsRect.bottom = y + (textHeight * 0.5f);

        // Draw main seconds
        m_renderTarget->DrawText(
            secondsBuffer,
            wcslen(secondsBuffer),
            m_textFormat,
            secondsRect,
            brush);

        // Draw milliseconds smaller next to seconds
        if (m_msTextFormat)
        {
          D2D1_RECT_F msRect;
          msRect.left = x + (textWidth * 0.3f);
          msRect.top = y + (textHeight * 0.2f);
          msRect.right = x + (textWidth * 0.5f);
          msRect.bottom = y + (textHeight * 0.5f);

          m_renderTarget->DrawText(
              msBuffer,
              wcslen(msBuffer),
              m_msTextFormat,
              msRect,
              brush);
        }

        brush->Release();
      }
    }

#ifdef SPIKE_DETECTION_ENABLED
    void drawDetectionRegion(int screenW, int screenH)
    {
      if (!m_renderTarget)
        return;

      DetectionRegion region = SpikeDetector::computeDetectionRegion(screenW, screenH);
      if (region.w <= 0 || region.h <= 0)
        return;

      ID2D1SolidColorBrush *boxBrush = nullptr;
      D2D1_COLOR_F color;
      color.r = 0.95f;
      color.g = 0.85f;
      color.b = 0.15f;
      color.a = 0.85f;
      if (FAILED(m_renderTarget->CreateSolidColorBrush(color, &boxBrush)) || !boxBrush)
        return;

      D2D1_RECT_F rect;
      int offX = 0, offY = 0;
      SpikeDetector::getOverlayOffset(offX, offY);
      // Convert screen coords to overlay-local coords for drawing
      rect.left = static_cast<float>(region.x - offX);
      rect.top = static_cast<float>(region.y - offY);
      rect.right = static_cast<float>(region.x + region.w - offX);
      rect.bottom = static_cast<float>(region.y + region.h - offY);


      m_renderTarget->DrawRectangle(rect, boxBrush, 2.0f);
      boxBrush->Release();
    }
#endif

    HWND m_hwnd{nullptr};
    ID2D1Factory *m_factory{nullptr};
    ID2D1DCRenderTarget *m_renderTarget{nullptr};
    IDWriteFactory *m_dwriteFactory{nullptr};
    IDWriteTextFormat *m_textFormat{nullptr};
    IDWriteTextFormat *m_msTextFormat{nullptr};
    float m_lastFontSize = 0.0f;
  };

} // namespace overlayx
