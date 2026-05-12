#pragma once

#include "interfaces/IRenderer.hpp"
#include <d2d1.h>
#include <cmath>

#pragma comment(lib, "d2d1.lib")

namespace overlayx {

/// Direct2D renderer for drawing crosshair shapes onto a layered window.
class Direct2DRenderer final : public IRenderer {
public:
    ~Direct2DRenderer() override { cleanup(); }

    bool initialize(void* windowHandle) override {
        m_hwnd = static_cast<HWND>(windowHandle);
        HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_factory);
        if (FAILED(hr)) return false;

        // DC render target for use with UpdateLayeredWindow
        D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
            D2D1_RENDER_TARGET_TYPE_DEFAULT,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
        );
        hr = m_factory->CreateDCRenderTarget(&props, &m_renderTarget);
        return SUCCEEDED(hr);
    }

    void render(const OverlayConfig& config) override {
        if (!m_renderTarget || !m_hwnd) return;

        int screenW = GetSystemMetrics(SM_CXSCREEN);
        int screenH = GetSystemMetrics(SM_CYSCREEN);

        // Create a memory DC and 32-bit ARGB DIB
        HDC screenDC = GetDC(nullptr);
        HDC memDC = CreateCompatibleDC(screenDC);

        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = screenW;
        bmi.bmiHeader.biHeight = -screenH;  // top-down
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        void* bits = nullptr;
        HBITMAP hBitmap = CreateDIBSection(memDC, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
        HGDIOBJ oldBitmap = SelectObject(memDC, hBitmap);

        // Bind D2D to the memory DC
        RECT rc = {0, 0, screenW, screenH};
        m_renderTarget->BindDC(memDC, &rc);

        m_renderTarget->BeginDraw();
        m_renderTarget->Clear(D2D1::ColorF(0, 0, 0, 0));  // fully transparent

        if (config.visible) {
            for (const auto& layer : config.layers) {
                if (!layer.enabled) continue;

                // Create brush with layer color
                ID2D1SolidColorBrush* brush = nullptr;
                m_renderTarget->CreateSolidColorBrush(
                    D2D1::ColorF(
                        layer.color.r / 255.0f,
                        layer.color.g / 255.0f,
                        layer.color.b / 255.0f,
                        layer.color.a / 255.0f
                    ),
                    &brush
                );

                ID2D1SolidColorBrush* outlineBrush = nullptr;
                if (layer.outlineEnabled) {
                    m_renderTarget->CreateSolidColorBrush(
                        D2D1::ColorF(
                            layer.outlineColor.r / 255.0f,
                            layer.outlineColor.g / 255.0f,
                            layer.outlineColor.b / 255.0f,
                            layer.outlineColor.a / 255.0f
                        ),
                        &outlineBrush
                    );
                }

                if (brush) {
                    float cx = (layer.posX * screenW);
                    float cy = (layer.posY * screenH);

                    if (layer.rotation != 0.0f) {
                        m_renderTarget->SetTransform(
                            D2D1::Matrix3x2F::Rotation(layer.rotation, D2D1::Point2F(cx, cy))
                        );
                    } else {
                        m_renderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
                    }

                    switch (layer.shape) {
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
                    if (outlineBrush) outlineBrush->Release();
                }
            }
        }

        m_renderTarget->EndDraw();

        // Update the layered window with per-pixel alpha
        POINT ptSrc = {0, 0};
        POINT ptDst = {0, 0};
        SIZE  sz    = {screenW, screenH};
        BLENDFUNCTION blend = {};
        blend.BlendOp             = AC_SRC_OVER;
        blend.SourceConstantAlpha = 255;
        blend.AlphaFormat         = AC_SRC_ALPHA;

        UpdateLayeredWindow(m_hwnd, screenDC, &ptDst, &sz, memDC, &ptSrc, 0, &blend, ULW_ALPHA);

        // Cleanup GDI resources
        SelectObject(memDC, oldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(memDC);
        ReleaseDC(nullptr, screenDC);
    }

    void resize(int, int) override {
        // Full-screen overlay — resize handled dynamically in render()
    }

    void cleanup() override {
        if (m_renderTarget) { m_renderTarget->Release(); m_renderTarget = nullptr; }
        if (m_factory)      { m_factory->Release();      m_factory = nullptr; }
    }

private:
    void drawCross(float cx, float cy, float w, float h, float thickness, float gap,
                   ID2D1SolidColorBrush* brush, ID2D1SolidColorBrush* outlineBrush = nullptr, float outlineThickness = 0.0f) {
        float halfW = w / 2.0f;
        float halfH = h / 2.0f;
        float halfT = thickness / 2.0f;

        D2D1_RECT_F bars[4] = {
            D2D1::RectF(cx - halfT, cy - halfH, cx + halfT, cy - gap), // Top
            D2D1::RectF(cx - halfT, cy + gap, cx + halfT, cy + halfH), // Bottom
            D2D1::RectF(cx - halfW, cy - halfT, cx - gap, cy + halfT), // Left
            D2D1::RectF(cx + gap, cy - halfT, cx + halfW, cy + halfT)  // Right
        };

        if (outlineBrush && outlineThickness > 0) {
            for (int i = 0; i < 4; ++i) {
                D2D1_RECT_F outlineRect = bars[i];
                outlineRect.left -= outlineThickness;
                outlineRect.top -= outlineThickness;
                outlineRect.right += outlineThickness;
                outlineRect.bottom += outlineThickness;
                m_renderTarget->FillRectangle(outlineRect, outlineBrush);
            }
        }

        for (int i = 0; i < 4; ++i) {
            m_renderTarget->FillRectangle(bars[i], brush);
        }
    }

    void drawDot(float cx, float cy, float w, float h, ID2D1SolidColorBrush* brush, 
                 ID2D1SolidColorBrush* outlineBrush = nullptr, float outlineThickness = 0.0f) {
        float rx = w / 2.0f;
        float ry = h / 2.0f;
        D2D1_ELLIPSE ellipse = D2D1::Ellipse(D2D1::Point2F(cx, cy), rx, ry);
        
        if (outlineBrush && outlineThickness > 0) {
            D2D1_ELLIPSE outlineEllipse = D2D1::Ellipse(D2D1::Point2F(cx, cy), rx + outlineThickness, ry + outlineThickness);
            m_renderTarget->FillEllipse(outlineEllipse, outlineBrush);
        }
        
        m_renderTarget->FillEllipse(ellipse, brush);
    }

    void drawCircle(float cx, float cy, float w, float h, float thickness,
                    ID2D1SolidColorBrush* brush, ID2D1SolidColorBrush* outlineBrush = nullptr, float outlineThickness = 0.0f) {
        float rx = w / 2.0f;
        float ry = h / 2.0f;
        D2D1_ELLIPSE ellipse = D2D1::Ellipse(D2D1::Point2F(cx, cy), rx, ry);
        
        if (outlineBrush && outlineThickness > 0) {
            // Outline on a circle (outline of a stroke)
            // We draw a slightly thicker circle behind or just two concentric strokes.
            // Best is to draw the outline with (thickness + 2*outlineThickness)
            m_renderTarget->DrawEllipse(ellipse, outlineBrush, thickness + (outlineThickness * 2.0f));
        }
        
        m_renderTarget->DrawEllipse(ellipse, brush, thickness);
    }

    HWND                  m_hwnd{nullptr};
    ID2D1Factory*         m_factory{nullptr};
    ID2D1DCRenderTarget*  m_renderTarget{nullptr};
};

} // namespace overlayx
