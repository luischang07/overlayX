#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <cstdint>

namespace overlayx
{

  // ─── Shape Types ───────────────────────────────────────────
  enum class ShapeType : int
  {
    Cross = 0,
    Dot = 1,
    Circle = 2
  };

  NLOHMANN_JSON_SERIALIZE_ENUM(ShapeType, {
                                              {ShapeType::Cross, "cross"},
                                              {ShapeType::Dot, "dot"},
                                              {ShapeType::Circle, "circle"},
                                          })

  // ─── Color ─────────────────────────────────────────────────
  struct Color
  {
    int r = 255;
    int g = 255;
    int b = 255;
    int a = 255;
  };

  inline void to_json(nlohmann::json &j, const Color &c)
  {
    j = nlohmann::json{{"r", c.r}, {"g", c.g}, {"b", c.b}, {"a", c.a}};
  }

  inline void from_json(const nlohmann::json &j, Color &c)
  {
    j.at("r").get_to(c.r);
    j.at("g").get_to(c.g);
    j.at("b").get_to(c.b);
    j.at("a").get_to(c.a);
  }

  // ─── Overlay Configuration (Domain Entity) ─────────────────
  struct SavedPosition
  {
    std::string name;
    float x;
    float y;
  };

  inline void to_json(nlohmann::json &j, const SavedPosition &p)
  {
    j = nlohmann::json{{"name", p.name}, {"x", p.x}, {"y", p.y}};
  }

  inline void from_json(const nlohmann::json &j, SavedPosition &p)
  {
    j.at("name").get_to(p.name);
    j.at("x").get_to(p.x);
    j.at("y").get_to(p.y);
  }

  struct Layer
  {
    std::string name = "New Layer";
    ShapeType shape = ShapeType::Cross;
    Color color = {255, 255, 255, 255};
    float thickness = 2.0f;
    float width = 20.0f;
    float height = 20.0f;
    float gap = 4.0f;
    float posX = 0.5f;
    float posY = 0.5f;
    float rotation = 0.0f;
    bool enabled = true;
    bool outlineEnabled = false;
    Color outlineColor = {0, 0, 0, 255};
    float outlineThickness = 1.0f;
    std::string presetId = ""; // Group layers together for movement
    int hotkeyVk = 0;
    int hotkeyModifiers = 0;
  };

  inline void to_json(nlohmann::json &j, const Layer &l)
  {
    j = nlohmann::json{
        {"name", l.name}, {"shape", l.shape}, {"color", l.color}, {"thickness", l.thickness}, {"width", l.width}, {"height", l.height}, {"gap", l.gap}, {"posX", l.posX}, {"posY", l.posY}, {"rotation", l.rotation}, {"enabled", l.enabled}, {"outlineEnabled", l.outlineEnabled}, {"outlineColor", l.outlineColor}, {"outlineThickness", l.outlineThickness}, {"presetId", l.presetId}, {"hotkeyVk", l.hotkeyVk}, {"hotkeyModifiers", l.hotkeyModifiers}};
  }

  inline void from_json(const nlohmann::json &j, Layer &l)
  {
    l.name = j.value("name", l.name);
    l.shape = j.value("shape", l.shape);
    if (j.contains("color"))
      from_json(j.at("color"), l.color);
    l.thickness = j.value("thickness", l.thickness);

    // Support migration from old 'size' field if it exists
    if (j.contains("size"))
    {
      float s = j.at("size").get<float>();
      l.width = s;
      l.height = s;
    }
    else
    {
      l.width = j.value("width", l.width);
      l.height = j.value("height", l.height);
    }

    l.gap = j.value("gap", l.gap);
    l.posX = j.value("posX", l.posX);
    l.posY = j.value("posY", l.posY);
    l.rotation = j.value("rotation", l.rotation);
    l.enabled = j.value("enabled", l.enabled);
    l.outlineEnabled = j.value("outlineEnabled", l.outlineEnabled);
    if (j.contains("outlineColor"))
      from_json(j.at("outlineColor"), l.outlineColor);
    l.outlineThickness = j.value("outlineThickness", l.outlineThickness);
    l.presetId = j.value("presetId", l.presetId);
    l.hotkeyVk = j.value("hotkeyVk", l.hotkeyVk);
    l.hotkeyModifiers = j.value("hotkeyModifiers", l.hotkeyModifiers);
  }

  struct CrosshairPreset
  {
    std::string name;
    std::string uid;
    float posX = 0.5f;
    float posY = 0.5f;
    std::vector<Layer> layers;
  };

  inline void to_json(nlohmann::json &j, const CrosshairPreset &p)
  {
    j = nlohmann::json{
        {"name", p.name},
        {"uid", p.uid},
        {"posX", p.posX},
        {"posY", p.posY},
        {"layers", p.layers}};
  }

  inline void from_json(const nlohmann::json &j, CrosshairPreset &p)
  {
    j.at("name").get_to(p.name);
    if (j.contains("uid"))
      j.at("uid").get_to(p.uid);
    if (j.contains("posX"))
      j.at("posX").get_to(p.posX);
    if (j.contains("posY"))
      j.at("posY").get_to(p.posY);
    j.at("layers").get_to(p.layers);
  }

  // ─── Countdown Configuration ────────────────────────────────
  struct CountdownConfig
  {
    bool enabled = false;
    int duration = 45; // Initial duration in seconds
    float posX = 0.5f; // Position on screen
    float posY = 0.5f;
    int fontSize = 48; // Font size in pixels
    Color color = {255, 255, 255, 255};
    int hotkeyStartVk = 0; // VK for start
    int hotkeyStartMods = 0;
    int hotkeyStopVk = 0; // VK for stop
    int hotkeyStopMods = 0;
    int hotkeyResetVk = 0; // VK for reset
    int hotkeyResetMods = 0;
  };

  inline void to_json(nlohmann::json &j, const CountdownConfig &c)
  {
    j = nlohmann::json{
        {"enabled", c.enabled}, {"duration", c.duration}, {"posX", c.posX}, {"posY", c.posY}, {"fontSize", c.fontSize}, {"color", c.color}, {"hotkeyStartVk", c.hotkeyStartVk}, {"hotkeyStartMods", c.hotkeyStartMods}, {"hotkeyStopVk", c.hotkeyStopVk}, {"hotkeyStopMods", c.hotkeyStopMods}, {"hotkeyResetVk", c.hotkeyResetVk}, {"hotkeyResetMods", c.hotkeyResetMods}};
  }

  inline void from_json(const nlohmann::json &j, CountdownConfig &c)
  {
    c.enabled = j.value("enabled", c.enabled);
    c.duration = j.value("duration", c.duration);
    c.posX = j.value("posX", c.posX);
    c.posY = j.value("posY", c.posY);
    c.fontSize = j.value("fontSize", c.fontSize);
    if (j.contains("color"))
      from_json(j.at("color"), c.color);
    c.hotkeyStartVk = j.value("hotkeyStartVk", c.hotkeyStartVk);
    c.hotkeyStartMods = j.value("hotkeyStartMods", c.hotkeyStartMods);
    c.hotkeyStopVk = j.value("hotkeyStopVk", c.hotkeyStopVk);
    c.hotkeyStopMods = j.value("hotkeyStopMods", c.hotkeyStopMods);
    c.hotkeyResetVk = j.value("hotkeyResetVk", c.hotkeyResetVk);
    c.hotkeyResetMods = j.value("hotkeyResetMods", c.hotkeyResetMods);
  }

  // Extended countdown runtime control fields
  struct CountdownRuntime
  {
    bool enabled = false; // whether countdown is currently active (running)
    // epoch timestamp in milliseconds when countdown was started
    long long startTimestampMs = 0;
    // when paused, remaining milliseconds stored here (>=0). -1 means not paused.
    long long pausedRemainingMs = -1;
  };

  inline void to_json(nlohmann::json &j, const CountdownRuntime &r)
  {
    j = nlohmann::json{{"enabled", r.enabled}, {"startTimestampMs", r.startTimestampMs}, {"pausedRemainingMs", r.pausedRemainingMs}};
  }

  inline void from_json(const nlohmann::json &j, CountdownRuntime &r)
  {
    r.enabled = j.value("enabled", r.enabled);
    r.startTimestampMs = j.value("startTimestampMs", r.startTimestampMs);
    r.pausedRemainingMs = j.value("pausedRemainingMs", r.pausedRemainingMs);
  }

  struct OverlayConfig
  {
    float posX = 0.5f; // Global offset/base X
    float posY = 0.5f; // Global offset/base Y
    int hotkey = 0x75; // VK_F6
    int hotkeyModifiers = 0;
    bool visible = true;
    bool editMode = false;

    std::vector<Layer> layers;
    std::vector<SavedPosition> savedPositions;
    std::vector<CrosshairPreset> savedPresets;
    CountdownConfig countdown;
    CountdownRuntime countdownRuntime;

    // Transient signals (not saved to disk necessarily, but sent via IPC)
    bool shouldExit = false;

    OverlayConfig()
    {
      // Add a default layer
      layers.push_back(Layer{"Default", ShapeType::Cross, {255, 255, 255, 255}, 2.0f, 20.0f, 20.0f, 4.0f});
    }
  };

  inline void to_json(nlohmann::json &j, const OverlayConfig &c)
  {
    j = nlohmann::json{
        {"posX", c.posX}, {"posY", c.posY}, {"hotkey", c.hotkey}, {"hotkeyModifiers", c.hotkeyModifiers}, {"visible", c.visible}, {"editMode", c.editMode}, {"layers", c.layers}, {"savedPositions", c.savedPositions}, {"savedPresets", c.savedPresets}, {"countdown", c.countdown}, {"countdownRuntime", c.countdownRuntime}, {"shouldExit", c.shouldExit}};
  }

  inline void from_json(const nlohmann::json &j, OverlayConfig &c)
  {
    if (j.contains("posX"))
      j.at("posX").get_to(c.posX);
    if (j.contains("posY"))
      j.at("posY").get_to(c.posY);
    j.at("hotkey").get_to(c.hotkey);
    c.hotkeyModifiers = j.value("hotkeyModifiers", 0);
    j.at("visible").get_to(c.visible);
    j.at("editMode").get_to(c.editMode);

    if (j.contains("layers"))
    {
      j.at("layers").get_to(c.layers);
    }
    if (j.contains("savedPositions"))
    {
      j.at("savedPositions").get_to(c.savedPositions);
    }
    if (j.contains("savedPresets"))
    {
      j.at("savedPresets").get_to(c.savedPresets);
    }
    if (j.contains("countdown"))
    {
      j.at("countdown").get_to(c.countdown);
    }
    if (j.contains("countdownRuntime"))
    {
      j.at("countdownRuntime").get_to(c.countdownRuntime);
    }
    if (j.contains("shouldExit"))
    {
      j.at("shouldExit").get_to(c.shouldExit);
    }
  }

} // namespace overlayx
