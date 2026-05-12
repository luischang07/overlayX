#pragma once

#include "interfaces/IRenderer.hpp"
#include <d2d1.h>
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
      D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
          D2D1_RENDER_TARGET_TYPE_DEFAULT,
          D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
      hr = m_factory->CreateDCRenderTarget(&props, &m_renderTarget);
      return SUCCEEDED(hr);
    }

    void render(const OverlayConfig &config) override
    {
      if (!m_renderTarget || !m_hwnd)
        return;

      int screenW = GetSystemMetrics(SM_CXSCREEN);
      int screenH = GetSystemMetrics(SM_CYSCREEN);

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
      m_renderTarget->Clear(D2D1::ColorF(0, 0, 0, 0)); // fully transparent

      if (config.visible)
      {
        for (const auto &layer : config.layers)
        {
          if (!layer.enabled)
            continue;

          // Create brush with layer color
          ID2D1SolidColorBrush *brush = nullptr;
          m_renderTarget->CreateSolidColorBrush(
              D2D1::ColorF(
                  layer.color.r / 255.0f,
                  layer.color.g / 255.0f,
                  layer.color.b / 255.0f,
                  layer.color.a / 255.0f),
              &brush);

          ID2D1SolidColorBrush *outlineBrush = nullptr;
          if (layer.outlineEnabled)
          {
            m_renderTarget->CreateSolidColorBrush(
                D2D1::ColorF(
                    layer.outlineColor.r / 255.0f,
                    layer.outlineColor.g / 255.0f,
                    layer.outlineColor.b / 255.0f,
                    layer.outlineColor.a / 255.0f),
                &outlineBrush);
          }

          if (brush)
          {
            float cx = (layer.posX * screenW);
            float cy = (layer.posY * screenH);

            if (layer.rotation != 0.0f)
            {
              m_renderTarget->SetTransform(
                  D2D1::Matrix3x2F::Rotation(layer.rotation, D2D1::Point2F(cx, cy)));
            }
            else
            {
              m_renderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
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
            m_renderTarget->SetTransform(D2D1::Matrix3x2F::Identity());

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

      D2D1_RECT_F bars[4] = {
          D2D1::RectF(cx - halfT, cy - halfH, cx + halfT, cy - gap), // Top
          D2D1::RectF(cx - halfT, cy + gap, cx + halfT, cy + halfH), // Bottom
          D2D1::RectF(cx - halfW, cy - halfT, cx - gap, cy + halfT), // Left
          D2D1::RectF(cx + gap, cy - halfT, cx + halfW, cy + halfT)  // Right
      };

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
      D2D1_ELLIPSE ellipse = D2D1::Ellipse(D2D1::Point2F(cx, cy), rx, ry);

      if (outlineBrush && outlineThickness > 0)
      {
        D2D1_ELLIPSE outlineEllipse = D2D1::Ellipse(D2D1::Point2F(cx, cy), rx + outlineThickness, ry + outlineThickness);
        m_renderTarget->FillEllipse(outlineEllipse, outlineBrush);
      }

      m_renderTarget->FillEllipse(ellipse, brush);
    }

    void drawCircle(float cx, float cy, float w, float h, float thickness,
                    ID2D1SolidColorBrush *brush, ID2D1SolidColorBrush *outlineBrush = nullptr, float outlineThickness = 0.0f)
    {
      float rx = w / 2.0f;
      float ry = h / 2.0f;
      D2D1_ELLIPSE ellipse = D2D1::Ellipse(D2D1::Point2F(cx, cy), rx, ry);

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

        FLOAT msFontSize = std::max<FLOAT>(8.0f, static_cast<FLOAT>(config.fontSize) * 0.42f);
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
        long long nowMs = duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count();
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

      wchar_t secondsBuffer[32];
      wchar_t millisecondsBuffer[32];
      swprintf_s(secondsBuffer, L"%02d:%02d", remainingSeconds / 60, remainingSeconds % 60);
      swprintf_s(millisecondsBuffer, L".%02d", remainingCentiseconds);

      ID2D1SolidColorBrush *brush = nullptr;
      m_renderTarget->CreateSolidColorBrush(
          D2D1::ColorF(
              config.color.r / 255.0f,
              config.color.g / 255.0f,
              config.color.b / 255.0f,
              config.color.a / 255.0f),
          &brush);

      if (brush && m_textFormat && m_msTextFormat)
      {
        float x = config.posX * screenW;
        float y = config.posY * screenH;
        float mainWidth = static_cast<float>(std::max(120, config.fontSize * 4));
        float mainHeight = static_cast<float>(std::max(64, config.fontSize * 2));
        float msWidth = static_cast<float>(std::max(60, config.fontSize * 2));
        float msHeight = static_cast<float>(std::max(28, config.fontSize));

        D2D1_RECT_F secondsRect = D2D1::RectF(
            x - (mainWidth * 0.5f),
            y - (mainHeight * 0.5f),
            x + (mainWidth * 0.15f),
            y + (mainHeight * 0.5f));

        D2D1_RECT_F msRect = D2D1::RectF(
            x + (mainWidth * 0.12f),
            y - (msHeight * 0.25f),
            x + (mainWidth * 0.12f) + msWidth,
            y + (msHeight * 0.75f));

        m_renderTarget->DrawText(
            secondsBuffer,
            wcslen(secondsBuffer),
            m_textFormat,
            secondsRect,
            brush);

        m_renderTarget->DrawText(
            millisecondsBuffer,
            wcslen(millisecondsBuffer),
            m_msTextFormat,
            msRect,
            brush);

        brush->Release();
      }
    }

    HWND m_hwnd{nullptr};
    ID2D1Factory *m_factory{nullptr};
    ID2D1DCRenderTarget *m_renderTarget{nullptr};
    IDWriteFactory *m_dwriteFactory{nullptr};
    IDWriteTextFormat *m_textFormat{nullptr};
    IDWriteTextFormat *m_msTextFormat{nullptr};
    float m_lastFontSize = 0.0f;
  };

} // namespace overlayx
