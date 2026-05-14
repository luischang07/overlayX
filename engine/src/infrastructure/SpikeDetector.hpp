#pragma once

#include <windows.h>
#include <memory>
#include <atomic>
#include <algorithm>
#include <iomanip>
#include <cstdint>
#include <vector>
#include <string>
#include "FileSystemUtils.hpp"

#ifdef SPIKE_DETECTION_ENABLED
#include <opencv2/opencv.hpp>
#include <d3d11.h>
#include <dxgi1_2.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#endif

#include <chrono>

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
      // Try high-priority paths first (Installation assets), then fall back to development paths
      std::vector<std::string> searchPaths = {
          fs::ResolveAssetPath("spike.png"), // Standard installation path
          templatePath,
          "spike.png",
          "../spike.png",
          "../../spike.png"};

      for (const auto &path : searchPaths)
      {
        m_template = cv::imread(path);
        if (!m_template.empty())
        {
          return true;
        }
      }

      return false;
#else
      return false; // Spike detection disabled (OpenCV not available)
#endif
    }

    // Suggested delay (ms) for next detection cycle based on recent activity
    int getSuggestedDelayMs() const
    {
#ifdef SPIKE_DETECTION_ENABLED
      using namespace std::chrono;
      long long nowMs = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
      int IDLE_HZ = static_cast<int>(m_detectionIdleHz);
      int ACTIVE_HZ = static_cast<int>(m_detectionActiveHz);
      constexpr long long ACTIVE_MS = 2000;

      if ((nowMs - m_lastActivityMs) < ACTIVE_MS)
        return 1000 / ACTIVE_HZ;
      return 1000 / IDLE_HZ;
#else
      return 1000; // default 1s when disabled
#endif
    }

    // Configure detection parameters at runtime (idle and active polling frequencies in Hz)
    void setDetectionParams(int idleHz, int activeHz)
    {
      m_detectionIdleHz = std::max(1, idleHz);
      m_detectionActiveHz = std::max(1, std::max(activeHz, idleHz + 1));
    }

    // Simple timing stats (aggregated, low-overhead)
    struct TimingStats
    {
      long long captureMs = 0;
      long long precheckMs = 0;
      long long matchingMs = 0;
      long long totalMs = 0;
      int cycleCount = 0;
    };

    TimingStats getTimingStats() const { return m_timingStats; }
    void resetTimingStats() { m_timingStats = TimingStats(); }

    /// Detect spike icon on current screen
    bool detectSpike()
    {
#ifdef SPIKE_DETECTION_ENABLED
      using namespace std::chrono;
      auto cycleStartMs = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();

      // Use cached resolution or update if necessary
      updateResolution();
      int currentWidth = m_cachedWidth;
      int currentHeight = m_cachedHeight;

      DetectionRegion region = computeDetectionRegion(currentWidth, currentHeight);

      float scaleX = static_cast<float>(currentWidth) / CALIB_WIDTH;
      float scaleY = static_cast<float>(currentHeight) / CALIB_HEIGHT;

      // Capture once, then derive both the cheap thumbnail hash and matching buffer from the same pixels.
      auto captureStart = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
      cv::Mat screenRegion;
      std::uint64_t thumbHash = 0;
      bool captured = captureScreen(region, &screenRegion, &thumbHash);
      auto captureEnd = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
      m_timingStats.captureMs += (captureEnd - captureStart);

      if (!captured || screenRegion.empty())
        return false;

      auto precheckEnd = captureEnd;
      m_timingStats.precheckMs += (precheckEnd - captureStart);

      long long nowMs = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();

      if (thumbHash != 0 && thumbHash == m_lastThumbHash)
      {
        // ROI is unchanged, so keep the detector cold and skip the expensive match path.
        m_timingStats.totalMs += (nowMs - cycleStartMs);
        m_timingStats.cycleCount++;
        logStatsIfNeeded();
        return false;
      }

      m_lastThumbHash = thumbHash;
      m_lastActivityMs = nowMs;

      if (m_template.empty())
        return false;

      // Match in grayscale with cached DPI-scaled template and mask. 
      // Using TM_CCORR_NORMED with a mask effectively ignores the template's background.
      // 1. Stricter color filtering (Red - Green/Blue)
      cv::Mat bgr[3];
      cv::split(screenRegion, bgr);
      cv::Mat rg, rb, redness;
      cv::subtract(bgr[2], bgr[1], rg);
      cv::subtract(bgr[2], bgr[0], rb);
      cv::min(rg, rb, redness);
      
      // 2. Edge Detection: Distinguish sharp icon from smooth backgrounds
      cv::Mat edges;
      cv::Canny(redness, edges, 50, 150);
      cv::GaussianBlur(edges, edges, cv::Size(3, 3), 0);
      cv::Mat roiGray = edges;

      int targetW = static_cast<int>(CALIB_WIDTH_ICON * scaleX);
      int targetH = static_cast<int>(CALIB_HEIGHT_ICON * scaleY);
      ensureCachedTemplate(targetW, targetH);
      if (m_cachedTemplateGray.empty() || m_cachedMask.empty())
        return false;

      // Intensity pre-check: The spike icon has significant brightness. 
      // If the average intensity in the ROI is very low, it's a false positive (e.g. black screen).
      cv::Scalar meanIntensity = cv::mean(roiGray);
      if (meanIntensity[0] < 20.0) 
        return false;

      auto matchStart = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
      cv::Mat result;
      
      // TM_CCORR_NORMED is used because it is one of the few methods in OpenCV that supports masks.
      // We combine it with an intensity pre-check to handle dark/flat backgrounds.
      cv::matchTemplate(roiGray, m_cachedTemplateGray, result, cv::TM_CCORR_NORMED, m_cachedMask);

      double minVal, maxVal;
      cv::Point maxLoc;
      cv::minMaxLoc(result, nullptr, &maxVal, nullptr, &maxLoc);
      auto matchEnd = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
      m_timingStats.matchingMs += (matchEnd - matchStart);

      auto cycleEnd = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
      m_timingStats.totalMs += (cycleEnd - cycleStartMs);
      m_timingStats.cycleCount++;
      logStatsIfNeeded();

      // With masked TM_CCORR_NORMED, 0.70-0.80 is a reliable threshold
      if (maxVal > DETECTION_THRESHOLD)
      {
          std::cout << "[SpikeDetector] MATCH! Score: " << maxVal << " at (" << maxLoc.x << ", " << maxLoc.y << ")" << std::endl;
          
          // Debug: Save what we found
          // cv::imwrite("debug_match_edges.png", roiGray);
          // cv::imwrite("debug_match_roi.png", screenRegion);

          return true;
      }
      
      // Periodic log to see it's alive
      static int tick = 0;
      if (++tick % 60 == 0) {
          std::cout << "[SpikeDetector] Scanning... Best Score: " << maxVal << "\r" << std::flush;
      }
      return false;
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
    static inline int m_overlayOffsetX = 0;
    static inline int m_overlayOffsetY = 0;
    static inline int m_cachedWidth = 0;
    static inline int m_cachedHeight = 0;
    static inline long long m_lastResUpdate = 0;

    static void updateResolution()
    {
      using namespace std::chrono;
      long long now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();

      // Update resolution every 5 seconds to minimize system calls
      if (m_cachedWidth == 0 || (now - m_lastResUpdate > 5000))
      {
        DEVMODEW dm;
        ZeroMemory(&dm, sizeof(dm));
        dm.dmSize = sizeof(dm);
        if (EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &dm))
        {
          m_cachedWidth = dm.dmPelsWidth;
          m_cachedHeight = dm.dmPelsHeight;
        }
        else
        {
          // Fallback if EnumDisplaySettings fails
          HDC hdc = GetDC(NULL);
          m_cachedWidth = GetDeviceCaps(hdc, DESKTOPHORZRES);
          m_cachedHeight = GetDeviceCaps(hdc, DESKTOPVERTRES);
          ReleaseDC(NULL, hdc);
        }
        m_lastResUpdate = now;
      }
    }

    // Log aggregated timing stats every 10 seconds
    void logStatsIfNeeded() const
    {
      using namespace std::chrono;
      static long long lastLogMs = 0;
      long long nowMs = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();

      if (lastLogMs == 0)
        lastLogMs = nowMs;

      if ((nowMs - lastLogMs) >= 10000) // Every 10 seconds
      {
        double avgCapture = m_timingStats.cycleCount > 0 ? static_cast<double>(m_timingStats.captureMs) / m_timingStats.cycleCount : 0.0;
        double avgPrecheck = m_timingStats.cycleCount > 0 ? static_cast<double>(m_timingStats.precheckMs) / m_timingStats.cycleCount : 0.0;
        double avgMatching = m_timingStats.cycleCount > 0 ? static_cast<double>(m_timingStats.matchingMs) / m_timingStats.cycleCount : 0.0;
        double avgTotal = m_timingStats.cycleCount > 0 ? static_cast<double>(m_timingStats.totalMs) / m_timingStats.cycleCount : 0.0;

        OutputDebugStringA("=== Spike Detection Timing Stats (10s aggregate) ===\n");
        char buf[256];
        snprintf(buf, sizeof(buf), "Cycles: %d | Capture: %.2fms | Precheck: %.2fms | Matching: %.2fms | Total: %.2fms\n",
                 m_timingStats.cycleCount, avgCapture, avgPrecheck, avgMatching, avgTotal);
        OutputDebugStringA(buf);

        lastLogMs = nowMs;
      }
    }

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

    // Detection threshold (tuned for masked correlation matching)
    static constexpr float DETECTION_THRESHOLD = 0.70f;

    // Thumbnail pre-check state
    static constexpr int THUMB_SIZE = 16;
    std::uint64_t m_lastThumbHash{0};
    long long m_lastActivityMs{0};
    int m_cachedTemplateWidth{0};
    int m_cachedTemplateHeight{0};
    cv::Mat m_cachedTemplateGray;
    cv::Mat m_cachedMask;

    void ensureCachedTemplate(int targetW, int targetH)
    {
      if (targetW <= 0 || targetH <= 0)
        return;

      if (m_cachedTemplateGray.empty() || m_cachedTemplateWidth != targetW || m_cachedTemplateHeight != targetH)
      {
        cv::Mat bgr[3];
        cv::split(m_template, bgr);
        cv::Mat rg, rb, templateRedness;
        cv::subtract(bgr[2], bgr[1], rg);
        cv::subtract(bgr[2], bgr[0], rb);
        cv::min(rg, rb, templateRedness);
        
        cv::Mat resizedRedness;
        cv::resize(templateRedness, resizedRedness, cv::Size(targetW, targetH), 0, 0, cv::INTER_LINEAR);
        
        // Use Canny on template to get the shape
        cv::Mat templateEdges;
        cv::Canny(resizedRedness, templateEdges, 50, 150);
        
        // Binary mask from redness (foreground only)
        cv::threshold(resizedRedness, m_cachedMask, 15, 255, cv::THRESH_BINARY);
        
        // Blur edges for robustness
        cv::GaussianBlur(templateEdges, m_cachedTemplateGray, cv::Size(3, 3), 0);

        m_cachedTemplateWidth = targetW;
        m_cachedTemplateHeight = targetH;
      }
    }

    /// Capture specific region of the screen once, then return both the hash and a BGR view if requested.
    bool captureScreen(const DetectionRegion &region, cv::Mat *outBgr, std::uint64_t *outThumbHash)
    {
      if (region.w <= 0 || region.h <= 0)
        return false;

      // Ensure persistent resources are ready
      if (!m_hScreenDC)
        m_hScreenDC = GetDC(nullptr);
      if (!m_hMemDC)
        m_hMemDC = CreateCompatibleDC(m_hScreenDC);

      // Recreate bitmap only if size changed
      if (!m_hBitmap || m_bitmapW != region.w || m_bitmapH != region.h)
      {
        if (m_hBitmap)
          DeleteObject(m_hBitmap);
        m_hBitmap = CreateCompatibleBitmap(m_hScreenDC, region.w, region.h);
        m_bitmapW = region.w;
        m_bitmapH = region.h;
      }

      HGDIOBJ hOld = SelectObject(m_hMemDC, m_hBitmap);

      // NO CAPTUREBLT = No cursor flickering. We only need the base screen content.
      BitBlt(m_hMemDC, 0, 0, region.w, region.h, m_hScreenDC, region.x, region.y, SRCCOPY);

      BITMAPINFOHEADER bi = {0};
      bi.biSize = sizeof(BITMAPINFOHEADER);
      bi.biWidth = region.w;
      bi.biHeight = -region.h; // top-down
      bi.biPlanes = 1;
      bi.biBitCount = 32;
      bi.biCompression = BI_RGB;

      // Reuse internal buffer to avoid reallocations
      if (m_captureBuffer.size() < static_cast<size_t>(region.w * region.h * 4))
      {
        m_captureBuffer.resize(region.w * region.h * 4);
      }

      BITMAPINFO binfo = {0};
      binfo.bmiHeader = bi;

      if (!GetDIBits(m_hScreenDC, m_hBitmap, 0, region.h, m_captureBuffer.data(), &binfo, DIB_RGB_COLORS))
      {
        SelectObject(m_hMemDC, hOld);
        return false;
      }

      if (outThumbHash)
      {
        const std::uint64_t FNV_OFFSET = 14695981039346656037ull;
        const std::uint64_t FNV_PRIME = 1099511628211ull;
        std::uint64_t hash = FNV_OFFSET;
        for (int ty = 0; ty < THUMB_SIZE; ++ty)
        {
          int sy = (ty * region.h) / THUMB_SIZE;
          for (int tx = 0; tx < THUMB_SIZE; ++tx)
          {
            int sx = (tx * region.w) / THUMB_SIZE;
            size_t idx = (static_cast<size_t>(sy) * region.w + sx) * 4;
            if (idx + 2 >= m_captureBuffer.size())
              continue;

            unsigned char b = m_captureBuffer[idx + 0];
            unsigned char g = m_captureBuffer[idx + 1];
            unsigned char r = m_captureBuffer[idx + 2];
            unsigned char gray = static_cast<unsigned char>((r * 77 + g * 151 + b * 28) >> 8);

            hash ^= static_cast<std::uint64_t>(gray);
            hash *= FNV_PRIME;
          }
        }
        *outThumbHash = hash;
      }

      if (outBgr)
      {
        cv::Mat mat(region.h, region.w, CV_8UC4, m_captureBuffer.data());
        cv::cvtColor(mat, *outBgr, cv::COLOR_BGRA2BGR);
      }

      SelectObject(m_hMemDC, hOld);
      return true;
    }

    // Persistent GDI resources for capture optimization
    HDC m_hScreenDC{nullptr};
    HDC m_hMemDC{nullptr};
    HBITMAP m_hBitmap{nullptr};
    int m_bitmapW{0};
    int m_bitmapH{0};
    std::vector<BYTE> m_captureBuffer;
    cv::Mat m_template;
#endif

    // Detection parameters and timing
    int m_detectionIdleHz = 3;
    int m_detectionActiveHz = 10;
    TimingStats m_timingStats;
  };

} // namespace overlayx
