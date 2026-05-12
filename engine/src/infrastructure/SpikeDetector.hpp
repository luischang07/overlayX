#pragma once

#include <windows.h>
#include <memory>
#include <atomic>
#include <iomanip>

#ifdef SPIKE_DETECTION_ENABLED
#include <opencv2/opencv.hpp>
#include <d3d11.h>
#include <dxgi1_2.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#endif

namespace overlayx
{
  struct DetectionRegion
  {
    int x{0};
    int y{0};
    int w{0};
    int h{0};
  };

  /// Detects spike/defuse icons on screen using template matching with automatic resolution scaling
  class SpikeDetector
  {
  public:
    /// Set overlay window offset (top-left) in screen coordinates
    static void setOverlayOffset(int x, int y)
    {
      m_overlayOffsetX = x;
      m_overlayOffsetY = y;
    }

    static void getOverlayOffset(int &outX, int &outY)
    {
      outX = m_overlayOffsetX;
      outY = m_overlayOffsetY;
    }
    SpikeDetector() = default;
    ~SpikeDetector() = default;

    /// Initialize detector with template image path
    /// @param templatePath Path to the spike icon template PNG
    /// @return true if initialization successful
    bool initialize(const std::string &templatePath)
    {
#ifdef SPIKE_DETECTION_ENABLED
      m_template = cv::imread(templatePath);
      return true;
#else
      return false; // Spike detection disabled (OpenCV not available)
#endif
    }

    /// Detect spike icon on current screen
    /// @return true if spike icon detected with confidence > threshold
    bool detectSpike()
    {
#ifdef SPIKE_DETECTION_ENABLED
      // Capture screen
      cv::Mat screen = captureScreen();
      if (screen.empty())
        return false;

      // Calculate scaling factors based on current resolution
      int currentWidth = screen.cols;
      int currentHeight = screen.rows;
      

      DetectionRegion region = computeDetectionRegion(currentWidth, currentHeight);

      int x = region.x;
      int y = region.y;
      int w = region.w;
      int h = region.h;

      float scaleX = static_cast<float>(currentWidth) / CALIB_WIDTH;
      float scaleY = static_cast<float>(currentHeight) / CALIB_HEIGHT;

      // Validate bounds
      if (x < 0 || y < 0 || x + w > currentWidth || y + h > currentHeight)
        return false;
      if (w <= 0 || h <= 0)
        return false;

      // Extract region of interest
      cv::Rect roiRect(x, y, w, h);
      cv::Mat screenRegion = screen(roiRect);

      // Resize template to match current resolution (natural icon size, not ROI size)
      int targetW = static_cast<int>(CALIB_WIDTH_ICON * scaleX);
      int targetH = static_cast<int>(CALIB_HEIGHT_ICON * scaleY);
      
      cv::Mat templateScaled;
      if (m_template.empty()) return false;
      cv::resize(m_template, templateScaled, cv::Size(targetW, targetH), 0, 0, cv::INTER_LINEAR);

      // Pre-process: Pure Redness Channel
      // This isolates the red icon and filters out white/gray/black UI (timers) perfectly.
      // We also apply a threshold to suppress "weak" background redness (like bricks).
      // Pre-process: HSV Saturated Red Filter
      // This targets the specific "UI Red" and rejects both neutral and bright/washed backgrounds.
      auto getSaturatedRed = [](const cv::Mat& bgr) {
        if (bgr.empty() || bgr.channels() < 3) return bgr;
        
        cv::Mat hsv, mask1, mask2, mask;
        cv::cvtColor(bgr, hsv, cv::COLOR_BGR2HSV);
        
        // Red hue spans the 0 and 180 boundary in OpenCV HSV
        // Saturation > 100 kills white/gray. Value < 180 kills bright pink/white.
        cv::inRange(hsv, cv::Scalar(0, 100, 40), cv::Scalar(10, 255, 180), mask1);
        cv::inRange(hsv, cv::Scalar(170, 100, 40), cv::Scalar(180, 255, 180), mask2);
        cv::bitwise_or(mask1, mask2, mask);
        
        // Return Value channel masked to preserve icon structure
        std::vector<cv::Mat> hsvChannels;
        cv::split(hsv, hsvChannels);
        cv::Mat result = cv::Mat::zeros(bgr.size(), CV_8UC1);
        hsvChannels[2].copyTo(result, mask);
        return result;
      };

      cv::Mat processedROI = getSaturatedRed(screenRegion);
      cv::Mat processedTemplate = getSaturatedRed(templateScaled);

      // Smooth to reduce noise
      cv::GaussianBlur(processedROI, processedROI, cv::Size(3, 3), 0);
      cv::GaussianBlur(processedTemplate, processedTemplate, cv::Size(3, 3), 0);

      // Perform template matching using TM_CCOEFF_NORMED
      cv::Mat result;
      cv::matchTemplate(processedROI, processedTemplate, result, cv::TM_CCOEFF_NORMED);

      double minVal, maxVal;
      cv::minMaxLoc(result, &minVal, &maxVal, nullptr, nullptr);

      // Convert to confidence (0-100 where 100 = perfect match)
      double confidence = std::max(0.0, maxVal * 100.0);
      
      // Log detection percentage (live update on same line)
      // Log detection percentage (live update on same line)
      std::cout << "\r[" << currentWidth << "x" << currentHeight << "] Detection: " << std::fixed << std::setprecision(1) << confidence << "%    " << std::flush;
      
      // Save ROI to file for debugging periodically (every 2 seconds)
      static auto lastSaveTime = std::chrono::steady_clock::now();
      auto now = std::chrono::steady_clock::now();
      if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastSaveTime).count() > 2000)
      {
        cv::imwrite("debug_roi_screen.png", screenRegion);
        // Also save processed channel for debugging
        cv::imwrite("debug_roi_processed.png", processedROI);
        lastSaveTime = now;
      }

      // For intensity-based CCOEFF_NORMED, 0.45-0.50 is a solid threshold
      return maxVal > DETECTION_THRESHOLD;
#else
      return false; // Spike detection disabled (OpenCV not available)
#endif
    }

    static DetectionRegion computeDetectionRegion(int currentWidth, int currentHeight)
    {
      DetectionRegion region;

#ifdef SPIKE_DETECTION_ENABLED
      float scaleX = static_cast<float>(currentWidth) / CALIB_WIDTH;
      float scaleY = static_cast<float>(currentHeight) / CALIB_HEIGHT;

      region.x = static_cast<int>(CALIB_X * scaleX) + m_overlayOffsetX;
      region.y = static_cast<int>(CALIB_Y * scaleY) + m_overlayOffsetY;
      region.w = static_cast<int>(CALIB_WIDTH_ICON * scaleX);
      region.h = static_cast<int>(CALIB_HEIGHT_ICON * scaleY);

      constexpr int X_PADDING = 5;
      region.x = std::max(0, region.x - X_PADDING);
      region.w += X_PADDING * 2;
#else
      (void)currentWidth;
      (void)currentHeight;
#endif

      return region;
    }

  private:
#ifdef SPIKE_DETECTION_ENABLED
    // Calibrated coordinates for capture resolution (2560 x 1440)
    static constexpr int CALIB_WIDTH = 2560;
    static constexpr int CALIB_HEIGHT = 1440;

    // Spike icon position (top-left corner) from user capture
    static constexpr int CALIB_X = 1222;
    static constexpr int CALIB_Y = 16;

    // Spike icon dimensions (computed from top-left and bottom-right)
    static constexpr int CALIB_WIDTH_ICON = 117;  // 1339 - 1222
    static constexpr int CALIB_HEIGHT_ICON = 110; // 126 - 16

    // Detection threshold (tuned for Spikeness matching)
    static constexpr float DETECTION_THRESHOLD = 0.45f;

    /// Capture current screen to OpenCV Mat
    cv::Mat captureScreen()
    {
      // GDI BitBlt-based screen capture (simpler, avoids DXGI complexity)
      // Get actual monitor resolution (not DPI-scaled virtual coordinates)
      int screenWidth = 0, screenHeight = 0;
      // Enumerate monitor to get physical dimensions (respects actual monitor resolution)
      HDC hdc = GetDC(NULL);
      screenWidth = GetDeviceCaps(hdc, DESKTOPHORZRES);
      screenHeight = GetDeviceCaps(hdc, DESKTOPVERTRES);
      ReleaseDC(NULL, hdc);
      
      // Fallback to virtual screen metrics if GetDeviceCaps fails (unlikely)
      if (screenWidth <= 0 || screenHeight <= 0)
      {
        screenWidth = GetSystemMetrics(SM_CXSCREEN);
        screenHeight = GetSystemMetrics(SM_CYSCREEN);
      }

      HDC hScreenDC = GetDC(nullptr);
      HDC hMemDC = CreateCompatibleDC(hScreenDC);
      HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, screenWidth, screenHeight);
      HGDIOBJ hOld = SelectObject(hMemDC, hBitmap);

      // Include layered windows in capture when possible
      BitBlt(hMemDC, 0, 0, screenWidth, screenHeight, hScreenDC, 0, 0, SRCCOPY | CAPTUREBLT);

      BITMAP bmp;
      GetObject(hBitmap, sizeof(BITMAP), &bmp);

      BITMAPINFOHEADER bi;
      ZeroMemory(&bi, sizeof(bi));
      bi.biSize = sizeof(BITMAPINFOHEADER);
      bi.biWidth = screenWidth;
      bi.biHeight = -screenHeight; // top-down
      bi.biPlanes = 1;
      bi.biBitCount = 32;
      bi.biCompression = BI_RGB;

      std::vector<BYTE> buffer(screenWidth * screenHeight * 4);
      BITMAPINFO binfo;
      ZeroMemory(&binfo, sizeof(binfo));
      binfo.bmiHeader = bi;

      GetDIBits(hScreenDC, hBitmap, 0, screenHeight, buffer.data(), &binfo, DIB_RGB_COLORS);

      // Build OpenCV Mat from buffer (BGRA) and convert to BGR
      cv::Mat mat(screenHeight, screenWidth, CV_8UC4, buffer.data());
      cv::Mat matBGR;
      cv::cvtColor(mat, matBGR, cv::COLOR_BGRA2BGR);

      // Cleanup GDI
      SelectObject(hMemDC, hOld);
      DeleteObject(hBitmap);
      DeleteDC(hMemDC);
      ReleaseDC(nullptr, hScreenDC);

      return matBGR.clone();
    }

    cv::Mat m_template;
  #if defined(SPIKE_DETECTION_ENABLED)
    static inline int m_overlayOffsetX = 0;
    static inline int m_overlayOffsetY = 0;
  #endif
#endif
  };

} // namespace overlayx
