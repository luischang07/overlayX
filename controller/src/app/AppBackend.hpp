#pragma once

#include "entities/OverlayConfig.hpp"
#include "interfaces/IConfigRepository.hpp"
#include "interfaces/IIPCServer.hpp"
#include <QObject>
#include <QDebug>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
#include <QColor>
#include <random>
#include <sstream>
#include <iomanip>
#include <memory>
#include <windows.h> // For GetTickCount64, GetAsyncKeyState
#include <chrono>
#include <set>

namespace overlayx
{

  /// QObject backend bridging OverlayConfig to QML.
  /// Exposes all config properties, manages IPC + persistence.
  class AppBackend : public QObject
  {
    Q_OBJECT

    // ─── Scalar Properties ─────────────────────────────────
    Q_PROPERTY(bool visible READ visible WRITE setVisible NOTIFY configChanged)
    Q_PROPERTY(bool editMode READ editMode WRITE setEditMode NOTIFY configChanged)
    Q_PROPERTY(float posX READ posX WRITE setPosX NOTIFY configChanged)
    Q_PROPERTY(float posY READ posY WRITE setPosY NOTIFY configChanged)
    Q_PROPERTY(int hotkey READ hotkey WRITE setHotkey NOTIFY configChanged)
    Q_PROPERTY(int hotkeyModifiers READ hotkeyModifiers WRITE setHotkeyModifiers NOTIFY configChanged)
    Q_PROPERTY(QString globalHotkeyString READ globalHotkeyString NOTIFY configChanged)
    Q_PROPERTY(bool engineConnected READ engineConnected NOTIFY engineStatusChanged)
    Q_PROPERTY(int selectedLayerIndex READ selectedLayerIndex WRITE setSelectedLayerIndex NOTIFY selectedLayerIndexChanged)
    Q_PROPERTY(bool isListeningForHotkey READ isListeningForHotkey NOTIFY isListeningForHotkeyChanged)
    Q_PROPERTY(QString listeningInstanceId READ listeningInstanceId NOTIFY isListeningForHotkeyChanged)
    Q_PROPERTY(int configUpdateTick READ configUpdateTick NOTIFY configChanged)

    // ─── Collections ───────────────────────────────────────
    Q_PROPERTY(QVariantList layers READ layers NOTIFY configChanged)
    Q_PROPERTY(QVariantList savedPositions READ savedPositions NOTIFY configChanged)
    Q_PROPERTY(QVariantList savedPresets READ savedPresets NOTIFY configChanged)
    Q_PROPERTY(QVariantList activePresetInstances READ activePresetInstances NOTIFY configChanged)
    Q_PROPERTY(int layerCount READ layerCount NOTIFY configChanged)

    // ─── Countdown Properties ──────────────────────────────
    Q_PROPERTY(bool countdownEnabled READ countdownEnabled WRITE setCountdownEnabled NOTIFY configChanged)
    Q_PROPERTY(int countdownDuration READ countdownDuration WRITE setCountdownDuration NOTIFY configChanged)
    Q_PROPERTY(float countdownPosX READ countdownPosX WRITE setCountdownPosX NOTIFY configChanged)
    Q_PROPERTY(float countdownPosY READ countdownPosY WRITE setCountdownPosY NOTIFY configChanged)
    Q_PROPERTY(int countdownFontSize READ countdownFontSize WRITE setCountdownFontSize NOTIFY configChanged)
    Q_PROPERTY(QString countdownColorHex READ countdownColorHex WRITE setCountdownColorHex NOTIFY configChanged)
    Q_PROPERTY(QString countdownStartHotkey READ countdownStartHotkey NOTIFY configChanged)
    Q_PROPERTY(QString countdownStopHotkey READ countdownStopHotkey NOTIFY configChanged)
    Q_PROPERTY(QString countdownResetHotkey READ countdownResetHotkey NOTIFY configChanged)

  public:
    explicit AppBackend(std::unique_ptr<IConfigRepository> repo,
                        std::unique_ptr<IIPCServer> ipc,
                        QObject *parent = nullptr)
        : QObject(parent), m_repo(std::move(repo)), m_ipc(std::move(ipc))
    {
      auto loaded = m_repo->load();
      if (loaded)
        m_config = *loaded;

      m_ipc->start();

      // Auto-save every 5 seconds if dirty
      m_saveTimer.setInterval(5000);
      connect(&m_saveTimer, &QTimer::timeout, this, [this]()
              {
            if (m_dirty) {
                m_repo->save(m_config);
                m_dirty = false;
            } });
      m_saveTimer.start();

      // Poll engine status every 500ms
      m_statusTimer.setInterval(500);
      connect(&m_statusTimer, &QTimer::timeout, this, [this]()
              {
            bool connected = m_ipc->isClientConnected();
            if (connected != m_lastEngineStatus) {
                m_lastEngineStatus = connected;
                emit engineStatusChanged();
            } });
      m_statusTimer.start();

      // Poll for global hotkeys (runs on GUI thread, non-blocking)
      m_hotkeyTimer.setInterval(20);
      connect(&m_hotkeyTimer, &QTimer::timeout, this, &AppBackend::pollHotkeys);
      m_hotkeyTimer.start();
    }

    ~AppBackend() override
    {
      m_repo->save(m_config);
      m_config.visible = false;
      m_config.shouldExit = true;
      m_ipc->sendConfig(m_config);
      m_ipc->stop();
    }

    // ─── Property Accessors ────────────────────────────────
    bool visible() const { return m_config.visible; }
    bool editMode() const { return m_config.editMode; }
    float posX() const { return m_config.posX; }
    float posY() const { return m_config.posY; }
    int hotkey() const { return m_config.hotkey; }
    int hotkeyModifiers() const { return m_config.hotkeyModifiers; }
    QString globalHotkeyString() const { return formatHotkeyString(m_config.hotkey, m_config.hotkeyModifiers); }
    bool engineConnected() const { return m_ipc->isClientConnected(); }
    int layerCount() const { return (int)m_config.layers.size(); }

    void setVisible(bool v)
    {
      if (m_config.visible != v)
      {
        m_config.visible = v;
        markDirty();
      }
    }
    void setEditMode(bool v)
    {
      if (m_config.editMode != v)
      {
        m_config.editMode = v;
        markDirty();
      }
    }
    void setPosX(float v)
    {
      if (m_config.posX != v)
      {
        m_config.posX = v;
        markDirty();
      }
    }
    void setPosY(float v)
    {
      if (m_config.posY != v)
      {
        m_config.posY = v;
        markDirty();
      }
    }
    void setHotkey(int v)
    {
      if (m_config.hotkey != v)
      {
        m_config.hotkey = v;
        markDirty();
      }
    }
    void setHotkeyModifiers(int v)
    {
      if (m_config.hotkeyModifiers != v)
      {
        m_config.hotkeyModifiers = v;
        markDirty();
      }
    }

    int configUpdateTick() const { return m_configUpdateTick; }

    Q_INVOKABLE void clearGlobalHotkey()
    {
      setInstanceHotkey("global", 0, 0);
    }

    // ─── Countdown Accessors ───────────────────────────────
    bool countdownEnabled() const { return m_config.countdown.enabled; }
    int countdownDuration() const { return m_config.countdown.duration; }
    float countdownPosX() const { return m_config.countdown.posX; }
    float countdownPosY() const { return m_config.countdown.posY; }
    int countdownFontSize() const { return m_config.countdown.fontSize; }
    QString countdownColorHex() const
    {
      auto &c = m_config.countdown.color;
      return QString("#%1%2%3%4")
          .arg(c.a, 2, 16, QChar('0'))
          .arg(c.r, 2, 16, QChar('0'))
          .arg(c.g, 2, 16, QChar('0'))
          .arg(c.b, 2, 16, QChar('0'));
    }
    QString countdownStartHotkey() const
    {
      return formatHotkeyString(m_config.countdown.hotkeyStartVk, m_config.countdown.hotkeyStartMods);
    }
    QString countdownStopHotkey() const
    {
      return formatHotkeyString(m_config.countdown.hotkeyStopVk, m_config.countdown.hotkeyStopMods);
    }
    QString countdownResetHotkey() const
    {
      return formatHotkeyString(m_config.countdown.hotkeyResetVk, m_config.countdown.hotkeyResetMods);
    }

    void setCountdownEnabled(bool v)
    {
      if (m_config.countdown.enabled != v)
      {
        m_config.countdown.enabled = v;
        markDirty();
      }
    }
    void setCountdownDuration(int v)
    {
      if (m_config.countdown.duration != v)
      {
        m_config.countdown.duration = std::max(1, v);
        markDirty();
      }
    }
    void setCountdownPosX(float v)
    {
      if (m_config.countdown.posX != v)
      {
        m_config.countdown.posX = v;
        markDirty();
      }
    }
    void setCountdownPosY(float v)
    {
      if (m_config.countdown.posY != v)
      {
        m_config.countdown.posY = v;
        markDirty();
      }
    }
    void setCountdownFontSize(int v)
    {
      if (m_config.countdown.fontSize != v)
      {
        m_config.countdown.fontSize = std::max(8, v);
        markDirty();
      }
    }
    void setCountdownColorHex(const QString &hex)
    {
      QColor qc(hex);
      if (qc.isValid())
      {
        m_config.countdown.color.r = qc.red();
        m_config.countdown.color.g = qc.green();
        m_config.countdown.color.b = qc.blue();
        m_config.countdown.color.a = qc.alpha();
        markDirty();
      }
    }

    Q_INVOKABLE void startListeningForCountdownHotkey(const QString &hotkeyType)
    {
      m_listeningInstanceId = ("countdown_" + hotkeyType).toStdString();
      m_isListening = true;
      m_listeningTimer = 10;
      emit isListeningForHotkeyChanged();
    }

    Q_INVOKABLE void clearCountdownHotkey(const QString &hotkeyType)
    {
      if (hotkeyType == "start")
      {
        m_config.countdown.hotkeyStartVk = 0;
        m_config.countdown.hotkeyStartMods = 0;
      }
      else if (hotkeyType == "stop")
      {
        m_config.countdown.hotkeyStopVk = 0;
        m_config.countdown.hotkeyStopMods = 0;
      }
      else if (hotkeyType == "reset")
      {
        m_config.countdown.hotkeyResetVk = 0;
        m_config.countdown.hotkeyResetMods = 0;
      }
      markDirty();
    }

    // ─── Countdown Controls ───────────────────────────────
    Q_INVOKABLE void startCountdown()
    {
      using namespace std::chrono;
      auto now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();

      long long durationMs = static_cast<long long>(m_config.countdown.duration) * 1000LL;
      if (m_config.countdownRuntime.pausedRemainingMs >= 0)
      {
        long long remaining = m_config.countdownRuntime.pausedRemainingMs;
        m_config.countdownRuntime.startTimestampMs = now - (durationMs - remaining);
        m_config.countdownRuntime.pausedRemainingMs = -1;
      }
      else
      {
        m_config.countdownRuntime.startTimestampMs = now;
      }
      // Ensure countdown is visible when starting and runtime is marked running
      m_config.countdown.enabled = true;
      m_config.countdownRuntime.enabled = true;
      markDirty();
    }

    Q_INVOKABLE void stopCountdown()
    {
      using namespace std::chrono;
      auto now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
      long long durationMs = static_cast<long long>(m_config.countdown.duration) * 1000LL;
      if (m_config.countdownRuntime.enabled && m_config.countdownRuntime.startTimestampMs > 0)
      {
        long long elapsed = now - m_config.countdownRuntime.startTimestampMs;
        long long remaining = durationMs - elapsed;
        if (remaining < 0)
          remaining = 0;
        m_config.countdownRuntime.pausedRemainingMs = remaining;
      }
      // Pause runtime but keep visual enabled flag as-is
      m_config.countdownRuntime.enabled = false;
      markDirty();
    }

    Q_INVOKABLE void resetCountdown()
    {
      long long durationMs = static_cast<long long>(m_config.countdown.duration) * 1000LL;
      m_config.countdownRuntime.pausedRemainingMs = durationMs;
      m_config.countdownRuntime.startTimestampMs = 0;
      m_config.countdownRuntime.enabled = false;
      markDirty();
    }

    int selectedLayerIndex() const { return m_selectedLayerIndex; }
    void setSelectedLayerIndex(int i)
    {
      if (m_selectedLayerIndex != i)
      {
        m_selectedLayerIndex = i;
        emit selectedLayerIndexChanged();
      }
    }

    bool isListeningForHotkey() const { return m_isListening; }
    QString listeningInstanceId() const { return m_isListening ? QString::fromStdString(m_listeningInstanceId) : ""; }

    // ─── Layer Collection ──────────────────────────────────
    QVariantList layers() const
    {
      QVariantList list;
      for (int i = 0; i < (int)m_config.layers.size(); ++i)
      {
        list.append(layerToMap(m_config.layers[i], i));
      }
      return list;
    }

    // ─── Saved Positions ───────────────────────────────────
    QVariantList savedPositions() const
    {
      QVariantList list;
      for (auto &p : m_config.savedPositions)
      {
        QVariantMap m;
        m["name"] = QString::fromStdString(p.name);
        m["x"] = p.x;
        m["y"] = p.y;
        list.append(m);
      }
      return list;
    }

    QVariantList savedPresets() const
    {
      QVariantList list;
      for (auto &pr : m_config.savedPresets)
      {
        QVariantMap m;
        m["name"] = QString::fromStdString(pr.name);
        m["uid"] = QString::fromStdString(pr.uid);
        m["posX"] = pr.posX;
        m["posY"] = pr.posY;
        m["layerCount"] = (int)pr.layers.size();
        list.append(m);
      }
      return list;
    }

    QVariantList activePresetInstances() const
    {
      QVariantList list;
      std::vector<std::string> seenIds;

      for (const auto &l : m_config.layers)
      {
        if (l.presetId.empty())
          continue;

        if (std::find(seenIds.begin(), seenIds.end(), l.presetId) == seenIds.end())
        {
          seenIds.push_back(l.presetId);

          QVariantMap m;
          m["presetId"] = QString::fromStdString(l.presetId);

          // Find preset name from UID (prefix of instanceId)
          QString name = "Unknown Preset";
          size_t underscorePos = l.presetId.find('_');
          if (underscorePos != std::string::npos)
          {
            std::string uid = l.presetId.substr(0, underscorePos);
            for (const auto &pr : m_config.savedPresets)
            {
              if (pr.uid == uid)
              {
                name = QString::fromStdString(pr.name);
                break;
              }
            }
          }

          m["name"] = name;
          m["posX"] = l.posX;
          m["posY"] = l.posY;
          m["hotkey"] = formatHotkeyString(l.hotkeyVk, l.hotkeyModifiers);
          list.append(m);
        }
      }
      return list;
    }

    // ═══════════════════════════════════════════════════════
    // Invokable methods (called from QML)
    // ═══════════════════════════════════════════════════════

    Q_INVOKABLE void centerCrosshair()
    {
      if (validLayer(m_selectedLayerIndex))
      {
        updateLayerPosition(m_selectedLayerIndex, 0.5f, false); // Center X
        updateLayerPosition(m_selectedLayerIndex, 0.5f, true);  // Center Y
      }
    }

    Q_INVOKABLE void enableAllLayers()
    {
      for (auto &l : m_config.layers)
        l.enabled = true;
      markDirty();
    }

    Q_INVOKABLE void disableAllLayers()
    {
      for (auto &l : m_config.layers)
        l.enabled = false;
      markDirty();
    }

    // ─── Layer CRUD ────────────────────────────────────────
    Q_INVOKABLE void addLayer()
    {
      m_config.layers.push_back(
          Layer{"Layer " + std::to_string(m_config.layers.size())});
      setSelectedLayerIndex((int)m_config.layers.size() - 1);
      markDirty();
    }

    Q_INVOKABLE void removeLayer(int index)
    {
      if (index >= 0 && index < (int)m_config.layers.size())
      {
        m_config.layers.erase(m_config.layers.begin() + index);
        if (m_selectedLayerIndex >= (int)m_config.layers.size())
        {
          setSelectedLayerIndex(std::max(0, (int)m_config.layers.size() - 1));
        }
        markDirty();
      }
    }

    Q_INVOKABLE void duplicateLayer(int index)
    {
      if (index >= 0 && index < (int)m_config.layers.size())
      {
        m_config.layers.push_back(m_config.layers[index]);
        markDirty();
      }
    }

    Q_INVOKABLE void moveLayer(int from, int to)
    {
      if (from >= 0 && from < (int)m_config.layers.size() &&
          to >= 0 && to < (int)m_config.layers.size() && from != to)
      {
        auto item = m_config.layers[from];
        m_config.layers.erase(m_config.layers.begin() + from);
        m_config.layers.insert(m_config.layers.begin() + to, item);
        markDirty();
      }
    }

    // ─── Layer Property Setters ────────────────────────────
    Q_INVOKABLE void setLayerName(int i, const QString &name)
    {
      if (validLayer(i))
      {
        m_config.layers[i].name = name.toStdString();
        markDirty();
      }
    }
    Q_INVOKABLE void setLayerEnabled(int i, bool v)
    {
      if (validLayer(i))
      {
        m_config.layers[i].enabled = v;
        markDirty();
      }
    }
    Q_INVOKABLE void setLayerShape(int i, int shape)
    {
      if (validLayer(i))
      {
        m_config.layers[i].shape = (ShapeType)shape;
        markDirty();
      }
    }
    Q_INVOKABLE void setLayerWidth(int i, float v)
    {
      if (validLayer(i))
      {
        m_config.layers[i].width = v;
        markDirty();
      }
    }
    Q_INVOKABLE void setLayerHeight(int i, float v)
    {
      if (validLayer(i))
      {
        m_config.layers[i].height = v;
        markDirty();
      }
    }
    Q_INVOKABLE void setLayerThickness(int i, float v)
    {
      if (validLayer(i))
      {
        m_config.layers[i].thickness = v;
        markDirty();
      }
    }
    Q_INVOKABLE void setLayerGap(int i, float v)
    {
      if (validLayer(i))
      {
        m_config.layers[i].gap = v;
        markDirty();
      }
    }
    Q_INVOKABLE void setLayerPosX(int i, float v) { updateLayerPosition(i, v, false); }
    Q_INVOKABLE void setLayerPosY(int i, float v) { updateLayerPosition(i, v, true); }
    Q_INVOKABLE void setLayerRotation(int i, float v)
    {
      if (validLayer(i))
      {
        m_config.layers[i].rotation = v;
        markDirty();
      }
    }
    Q_INVOKABLE void setLayerColor(int i, const QString &hex)
    {
      if (!validLayer(i))
        return;
      QColor qc(hex);
      m_config.layers[i].color.r = qc.red();
      m_config.layers[i].color.g = qc.green();
      m_config.layers[i].color.b = qc.blue();
      m_config.layers[i].color.a = qc.alpha();
      markDirty();
    }
    Q_INVOKABLE void setLayerOutlineEnabled(int i, bool v)
    {
      if (validLayer(i))
      {
        m_config.layers[i].outlineEnabled = v;
        markDirty();
      }
    }
    Q_INVOKABLE void setLayerOutlineColor(int i, const QString &hex)
    {
      if (!validLayer(i))
        return;
      QColor qc(hex);
      m_config.layers[i].outlineColor.r = qc.red();
      m_config.layers[i].outlineColor.g = qc.green();
      m_config.layers[i].outlineColor.b = qc.blue();
      m_config.layers[i].outlineColor.a = qc.alpha();
      markDirty();
    }
    Q_INVOKABLE void setLayerOutlineThickness(int i, float v)
    {
      if (validLayer(i))
      {
        m_config.layers[i].outlineThickness = v;
        markDirty();
      }
    }

    Q_INVOKABLE void setInstanceHotkey(const QString &id, int vk, int modifiers)
    {
      if (id == "global")
      {
        m_config.hotkey = vk;
        m_config.hotkeyModifiers = modifiers;
        markDirty();
        return;
      }

      std::string sid = id.toStdString();

      // Handle countdown hotkeys
      if (sid.find("countdown_") == 0)
      {
        std::string hotkeyType = sid.substr(10); // Remove "countdown_" prefix
        if (hotkeyType == "start")
        {
          m_config.countdown.hotkeyStartVk = vk;
          m_config.countdown.hotkeyStartMods = modifiers;
        }
        else if (hotkeyType == "stop")
        {
          m_config.countdown.hotkeyStopVk = vk;
          m_config.countdown.hotkeyStopMods = modifiers;
        }
        else if (hotkeyType == "reset")
        {
          m_config.countdown.hotkeyResetVk = vk;
          m_config.countdown.hotkeyResetMods = modifiers;
        }
        markDirty();
        return;
      }

      std::string uid = "";
      size_t underscorePos = sid.find('_');
      if (underscorePos != std::string::npos)
      {
        uid = sid.substr(0, underscorePos);
      }

      // Update all layers that belong to this specific instance OR this preset type
      for (auto &l : m_config.layers)
      {
        bool matches = (l.presetId == sid);
        if (!matches && !uid.empty() && !l.presetId.empty())
        {
          // Also match other instances of the same preset
          if (l.presetId.find(uid + "_") == 0)
            matches = true;
        }

        if (matches)
        {
          l.hotkeyVk = vk;
          l.hotkeyModifiers = modifiers;
        }
      }

      // Update the template in savedPresets so new instances inherit this hotkey
      if (!uid.empty())
      {
        for (auto &pr : m_config.savedPresets)
        {
          if (pr.uid == uid)
          {
            for (auto &pl : pr.layers)
            {
              pl.hotkeyVk = vk;
              pl.hotkeyModifiers = modifiers;
            }
            break;
          }
        }
      }

      markDirty();
    }

    Q_INVOKABLE void startListeningForInstanceHotkey(const QString &id)
    {
      m_listeningInstanceId = id.toStdString();
      m_isListening = true;
      m_listeningTimer = 10;
      emit isListeningForHotkeyChanged();
    }

    Q_INVOKABLE void clearInstanceHotkey(const QString &id)
    {
      setInstanceHotkey(id, 0, 0);
    }

    Q_INVOKABLE QString getInstanceHotkeyString(const QString &id) const
    {
      std::string sid = id.toStdString();
      int vk = 0;
      int mods = 0;

      for (const auto &l : m_config.layers)
      {
        if (l.presetId == sid)
        {
          vk = l.hotkeyVk;
          mods = l.hotkeyModifiers;
          break;
        }
      }

      if (vk == 0)
        return "None";
      return formatHotkeyString(vk, mods);
    }

    Q_INVOKABLE QVariantMap getLayer(int i) const
    {
      if (i >= 0 && i < (int)m_config.layers.size())
        return layerToMap(m_config.layers[i], i);
      return {};
    }

    // ─── Position Management ───────────────────────────────
    Q_INVOKABLE void savePosition(const QString &name, int layerIndex)
    {
      if (name.isEmpty() || !validLayer(layerIndex))
        return;
      m_config.savedPositions.push_back(
          {name.toStdString(), m_config.layers[layerIndex].posX, m_config.layers[layerIndex].posY});
      markDirty();
    }
    Q_INVOKABLE void loadPosition(int index, int layerIndex)
    {
      if (index >= 0 && index < (int)m_config.savedPositions.size() && validLayer(layerIndex))
      {
        updateLayerPosition(layerIndex, m_config.savedPositions[index].x, false);
        updateLayerPosition(layerIndex, m_config.savedPositions[index].y, true);
      }
    }
    Q_INVOKABLE void deletePosition(int index)
    {
      if (index >= 0 && index < (int)m_config.savedPositions.size())
      {
        m_config.savedPositions.erase(m_config.savedPositions.begin() + index);
        markDirty();
      }
    }
    Q_INVOKABLE void renamePosition(int index, const QString &newName)
    {
      if (index >= 0 && index < (int)m_config.savedPositions.size() && !newName.isEmpty())
      {
        m_config.savedPositions[index].name = newName.toStdString();
        markDirty();
      }
    }

    // ─── Preset Management ─────────────────────────────────
    Q_INVOKABLE void savePreset(const QString &name)
    {
      if (name.isEmpty())
        return;
      CrosshairPreset p;
      p.name = name.toStdString();
      p.uid = generateUid();
      p.layers = m_config.layers;
      // Strip instance-specific IDs when saving as a template
      for (auto &l : p.layers)
        l.presetId = "";

      // Use the position of the first layer as the preset's reference position
      if (!p.layers.empty())
      {
        p.posX = p.layers[0].posX;
        p.posY = p.layers[0].posY;
      }
      else
      {
        p.posX = 0.5f;
        p.posY = 0.5f;
      }

      m_config.savedPresets.push_back(p);
      markDirty();
    }
    Q_INVOKABLE void loadPreset(int index)
    {
      if (index >= 0 && index < (int)m_config.savedPresets.size())
      {
        m_config.layers = m_config.savedPresets[index].layers;
        // When loading, treat all layers as a fresh group
        std::string instanceId = generateInstanceId(m_config.savedPresets[index].uid);
        for (auto &l : m_config.layers)
          l.presetId = instanceId;
        markDirty();
      }
    }
    Q_INVOKABLE void deletePreset(int index)
    {
      if (index >= 0 && index < (int)m_config.savedPresets.size())
      {
        m_config.savedPresets.erase(m_config.savedPresets.begin() + index);
        markDirty();
      }
    }

    Q_INVOKABLE void updatePreset(int index)
    {
      if (index >= 0 && index < (int)m_config.savedPresets.size())
      {
        m_config.savedPresets[index].layers = m_config.layers;
        markDirty();
      }
    }

    Q_INVOKABLE void setPresetPosX(int index, float x)
    {
      if (index >= 0 && index < (int)m_config.savedPresets.size())
      {
        m_config.savedPresets[index].posX = x;
        markDirty();
      }
    }

    Q_INVOKABLE void setPresetPosY(int index, float y)
    {
      if (index >= 0 && index < (int)m_config.savedPresets.size())
      {
        m_config.savedPresets[index].posY = y;
        markDirty();
      }
    }

    Q_INVOKABLE void setInstancePosX(const QString &id, float x)
    {
      std::string sid = id.toStdString();
      float currentX = 0;
      bool found = false;
      for (const auto &l : m_config.layers)
      {
        if (l.presetId == sid)
        {
          currentX = l.posX;
          found = true;
          break;
        }
      }
      if (found)
      {
        float delta = x - currentX;
        for (auto &l : m_config.layers)
        {
          if (l.presetId == sid)
            l.posX += delta;
        }
        markDirty();
      }
    }

    // Live variant: updates engine via IPC but does NOT emit configChanged.
    // Use this during slider drag to avoid destroying the ListView delegate mid-gesture.
    Q_INVOKABLE void setInstancePosXLive(const QString &id, float x)
    {
      std::string sid = id.toStdString();
      float currentX = 0;
      bool found = false;
      for (const auto &l : m_config.layers)
      {
        if (l.presetId == sid)
        {
          currentX = l.posX;
          found = true;
          break;
        }
      }
      if (found)
      {
        float delta = x - currentX;
        for (auto &l : m_config.layers)
        {
          if (l.presetId == sid)
            l.posX += delta;
        }
        m_dirty = true;
        m_ipc->sendConfig(m_config);
      }
    }

    Q_INVOKABLE void setInstancePosY(const QString &id, float y)
    {
      std::string sid = id.toStdString();
      float currentY = 0;
      bool found = false;
      for (const auto &l : m_config.layers)
      {
        if (l.presetId == sid)
        {
          currentY = l.posY;
          found = true;
          break;
        }
      }
      if (found)
      {
        float delta = y - currentY;
        for (auto &l : m_config.layers)
        {
          if (l.presetId == sid)
            l.posY += delta;
        }
        markDirty();
      }
    }

    // Live variant: updates engine via IPC but does NOT emit configChanged.
    Q_INVOKABLE void setInstancePosYLive(const QString &id, float y)
    {
      std::string sid = id.toStdString();
      float currentY = 0;
      bool found = false;
      for (const auto &l : m_config.layers)
      {
        if (l.presetId == sid)
        {
          currentY = l.posY;
          found = true;
          break;
        }
      }
      if (found)
      {
        float delta = y - currentY;
        for (auto &l : m_config.layers)
        {
          if (l.presetId == sid)
            l.posY += delta;
        }
        m_dirty = true;
        m_ipc->sendConfig(m_config);
      }
    }

    Q_INVOKABLE void removeInstance(const QString &id)
    {
      std::string sid = id.toStdString();
      auto it = std::remove_if(m_config.layers.begin(), m_config.layers.end(),
                               [&](const Layer &l)
                               { return l.presetId == sid; });
      if (it != m_config.layers.end())
      {
        m_config.layers.erase(it, m_config.layers.end());
        markDirty();
      }
    }

    Q_INVOKABLE void appendPreset(int index)
    {
      if (index >= 0 && index < (int)m_config.savedPresets.size())
      {
        auto &preset = m_config.savedPresets[index];
        if (preset.uid.empty())
          preset.uid = generateUid();

        std::string instanceId = generateInstanceId(preset.uid);

        // Calculate delta to move all layers relative to the preset's base position
        float deltaX = 0, deltaY = 0;
        if (!preset.layers.empty())
        {
          deltaX = preset.posX - preset.layers[0].posX;
          deltaY = preset.posY - preset.layers[0].posY;
        }

        for (const auto &l : preset.layers)
        {
          Layer nl = l;
          nl.presetId = instanceId;
          nl.posX += deltaX;
          nl.posY += deltaY;
          m_config.layers.push_back(nl);
        }
        markDirty();
      }
    }

    Q_INVOKABLE void removePresetLayers(int index)
    {
      if (index < 0 || index >= (int)m_config.savedPresets.size())
        return;

      const auto &presetLayers = m_config.savedPresets[index].layers;
      bool changed = false;

      for (const auto &pLayer : presetLayers)
      {
        // Find a matching layer in the current config and remove it
        for (auto it = m_config.layers.begin(); it != m_config.layers.end(); ++it)
        {
          // Match by basic properties
          if (it->name == pLayer.name &&
              it->shape == pLayer.shape &&
              it->width == pLayer.width &&
              it->height == pLayer.height &&
              it->thickness == pLayer.thickness &&
              it->color.r == pLayer.color.r &&
              it->color.g == pLayer.color.g &&
              it->color.b == pLayer.color.b &&
              it->color.a == pLayer.color.a)
          {
            m_config.layers.erase(it);
            changed = true;
            break;
          }
        }
      }

      if (changed)
        markDirty();
    }
    Q_INVOKABLE void renamePreset(int index, const QString &newName)
    {
      if (index >= 0 && index < (int)m_config.savedPresets.size() && !newName.isEmpty())
      {
        m_config.savedPresets[index].name = newName.toStdString();
        markDirty();
      }
    }

  signals:
    void configChanged();
    void engineStatusChanged();
    void selectedLayerIndexChanged();
    void isListeningForHotkeyChanged();

  private:
    void updateLayerPosition(int i, float val, bool isY)
    {
      if (!validLayer(i))
        return;
      float currentVal = isY ? m_config.layers[i].posY : m_config.layers[i].posX;
      float delta = val - currentVal;
      std::string pid = m_config.layers[i].presetId;

      if (!pid.empty())
      {
        // Move entire preset group together when changing base position
        for (auto &l : m_config.layers)
        {
          if (l.presetId == pid)
          {
            if (isY)
              l.posY += delta;
            else
              l.posX += delta;
          }
        }
      }
      else
      {
        // Move ONLY the specific layer if it has no presetId
        if (isY)
          m_config.layers[i].posY = val;
        else
          m_config.layers[i].posX = val;
      }
      markDirty();
    }

    std::string generateInstanceId(const std::string &base)
    {
      static uint64_t counter = 0;
      return base + "_" + std::to_string(GetTickCount64()) + "_" + std::to_string(++counter);
    }

    void markDirty()
    {
      m_dirty = true;
      m_configUpdateTick++;
      m_ipc->sendConfig(m_config);
      emit configChanged();
    }

    QString formatHotkeyString(int vk, int mods) const
    {
      if (vk == 0)
        return "None";
      QString res;
      if (mods & 1)
        res += "Ctrl + ";
      if (mods & 2)
        res += "Shift + ";
      if (mods & 4)
        res += "Alt + ";

      switch (vk)
      {
      case VK_LBUTTON:
        return res + "Left Mouse";
      case VK_RBUTTON:
        return res + "Right Mouse";
      case VK_MBUTTON:
        return res + "Middle Mouse";
      case VK_XBUTTON1:
        return res + "Mouse 4";
      case VK_XBUTTON2:
        return res + "Mouse 5";
      case VK_TAB:
        return res + "Tab";
      case VK_SPACE:
        return res + "Space";
      case VK_RETURN:
        return res + "Enter";
      }

      char name[128];
      UINT scanCode = MapVirtualKeyA(vk, MAPVK_VK_TO_VSC);
      switch (vk)
      {
      case VK_LEFT:
      case VK_UP:
      case VK_RIGHT:
      case VK_DOWN:
      case VK_PRIOR:
      case VK_NEXT:
      case VK_END:
      case VK_HOME:
      case VK_INSERT:
      case VK_DELETE:
      case VK_DIVIDE:
      case VK_NUMLOCK:
        scanCode |= 0x100;
        break;
      }

      LONG lParam = (scanCode << 16);
      if (GetKeyNameTextA(lParam, name, sizeof(name)) > 0)
      {
        res += QString::fromLocal8Bit(name);
      }
      else
      {
        res += QString("Key 0x%1").arg(vk, 0, 16);
      }
      return res;
    }

    bool validLayer(int i) const
    {
      return i >= 0 && i < (int)m_config.layers.size();
    }

    QVariantMap layerToMap(const Layer &l, int index) const
    {
      QVariantMap m;
      m["index"] = index;
      m["name"] = QString::fromStdString(l.name);
      m["enabled"] = l.enabled;
      m["shape"] = (int)l.shape;
      m["width"] = l.width;
      m["height"] = l.height;
      m["thickness"] = l.thickness;
      m["gap"] = l.gap;
      m["posX"] = l.posX;
      m["posY"] = l.posY;
      m["rotation"] = l.rotation;
      m["colorR"] = l.color.r;
      m["colorG"] = l.color.g;
      m["colorB"] = l.color.b;
      m["colorA"] = l.color.a;
      m["color"] = QString("#%1%2%3%4")
                       .arg(l.color.a, 2, 16, QChar('0'))
                       .arg(l.color.r, 2, 16, QChar('0'))
                       .arg(l.color.g, 2, 16, QChar('0'))
                       .arg(l.color.b, 2, 16, QChar('0'));
      m["outlineEnabled"] = l.outlineEnabled;
      m["outlineColor"] = QString("#%1%2%3%4")
                              .arg(l.outlineColor.a, 2, 16, QChar('0'))
                              .arg(l.outlineColor.r, 2, 16, QChar('0'))
                              .arg(l.outlineColor.g, 2, 16, QChar('0'))
                              .arg(l.outlineColor.b, 2, 16, QChar('0'));
      m["outlineThickness"] = l.outlineThickness;
      m["presetId"] = QString::fromStdString(l.presetId);
      m["hotkeyVk"] = l.hotkeyVk;
      m["hotkeyModifiers"] = l.hotkeyModifiers;
      return m;
    }

    std::string generateUid()
    {
      static std::random_device rd;
      static std::mt19937 gen(rd());
      static std::uniform_int_distribution<> dis(0, 15);
      std::stringstream ss;
      ss << std::hex;
      for (int i = 0; i < 16; i++)
        ss << dis(gen);
      return ss.str();
    }

    OverlayConfig m_config;
    std::unique_ptr<IConfigRepository> m_repo;
    std::unique_ptr<IIPCServer> m_ipc;
    bool m_dirty{false};
    bool m_lastEngineStatus{false};
    QTimer m_saveTimer;
    QTimer m_statusTimer;
    QTimer m_hotkeyTimer;
    int m_selectedLayerIndex{0};
    int m_configUpdateTick{0};

    // Hotkey tracking
    std::set<std::pair<int, int>> m_pressedHotkeys;
    bool m_masterHotkeyState{false};
    bool m_countdownStartPressedState{false};
    bool m_countdownStopPressedState{false};
    bool m_countdownResetPressedState{false};

    bool m_isListening{false};
    std::string m_listeningInstanceId;
    int m_listeningTimer{0};

    void pollHotkeys()
    {
      if (m_isListening)
      {
        if (m_listeningTimer > 0)
        {
          m_listeningTimer--;
          return;
        }

        int mods = 0;
        if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
          mods |= 1;
        if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
          mods |= 2;
        if (GetAsyncKeyState(VK_MENU) & 0x8000)
          mods |= 4;

        for (int vk = 1; vk < 255; ++vk)
        {
          if (vk == VK_SHIFT || vk == VK_LSHIFT || vk == VK_RSHIFT ||
              vk == VK_CONTROL || vk == VK_LCONTROL || vk == VK_RCONTROL ||
              vk == VK_MENU || vk == VK_LMENU || vk == VK_RMENU ||
              vk == VK_LWIN || vk == VK_RWIN)
          {
            continue;
          }

          if (GetAsyncKeyState(vk) & 0x8000)
          {
            // Store listening state before clearing it
            std::string targetId = m_listeningInstanceId;
            m_isListening = false;
            m_listeningInstanceId = "";
            emit isListeningForHotkeyChanged();

            if (vk == VK_ESCAPE || vk == VK_LBUTTON)
            {
              return; // Cancelled
            }

            if (!targetId.empty())
            {
              setInstanceHotkey(QString::fromStdString(targetId), vk, mods);
            }
            else
            {
              markDirty();
            }
            return;
          }
        }
        return;
      }

      std::set<std::pair<int, int>> currentlyPressed;
      std::set<std::string> toggledInstances;

      for (auto &l : m_config.layers)
      {
        if (l.hotkeyVk == 0 || l.presetId.empty())
          continue;
        if (toggledInstances.count(l.presetId))
          continue;

        bool isPressed = (GetAsyncKeyState(l.hotkeyVk) & 0x8000) != 0;
        if (isPressed && l.hotkeyModifiers != 0)
        {
          if ((l.hotkeyModifiers & 1) && !(GetAsyncKeyState(VK_CONTROL) & 0x8000))
            isPressed = false;
          if ((l.hotkeyModifiers & 2) && !(GetAsyncKeyState(VK_SHIFT) & 0x8000))
            isPressed = false;
          if ((l.hotkeyModifiers & 4) && !(GetAsyncKeyState(VK_MENU) & 0x8000))
            isPressed = false;
        }

        if (isPressed)
        {
          currentlyPressed.insert({l.hotkeyVk, l.hotkeyModifiers});
          if (m_pressedHotkeys.find({l.hotkeyVk, l.hotkeyModifiers}) == m_pressedHotkeys.end())
          {
            // Toggle all layers in this instance
            bool newState = !l.enabled;
            for (auto &l2 : m_config.layers)
            {
              if (l2.presetId == l.presetId)
                l2.enabled = newState;
            }
            toggledInstances.insert(l.presetId);
            markDirty();
          }
        }
      }

      m_pressedHotkeys = currentlyPressed;

      // Countdown hotkeys
      if (m_config.countdown.hotkeyStartVk != 0)
      {
        bool startPressed = (GetAsyncKeyState(m_config.countdown.hotkeyStartVk) & 0x8000) != 0;
        if (startPressed && m_config.countdown.hotkeyStartMods != 0)
        {
          if ((m_config.countdown.hotkeyStartMods & 1) && !(GetAsyncKeyState(VK_CONTROL) & 0x8000))
            startPressed = false;
          if ((m_config.countdown.hotkeyStartMods & 2) && !(GetAsyncKeyState(VK_SHIFT) & 0x8000))
            startPressed = false;
          if ((m_config.countdown.hotkeyStartMods & 4) && !(GetAsyncKeyState(VK_MENU) & 0x8000))
            startPressed = false;
        }

        if (startPressed && m_countdownStartPressedState == false)
        {
          startCountdown();
        }
        m_countdownStartPressedState = startPressed;
      }

      if (m_config.countdown.hotkeyStopVk != 0)
      {
        bool stopPressed = (GetAsyncKeyState(m_config.countdown.hotkeyStopVk) & 0x8000) != 0;
        if (stopPressed && m_config.countdown.hotkeyStopMods != 0)
        {
          if ((m_config.countdown.hotkeyStopMods & 1) && !(GetAsyncKeyState(VK_CONTROL) & 0x8000))
            stopPressed = false;
          if ((m_config.countdown.hotkeyStopMods & 2) && !(GetAsyncKeyState(VK_SHIFT) & 0x8000))
            stopPressed = false;
          if ((m_config.countdown.hotkeyStopMods & 4) && !(GetAsyncKeyState(VK_MENU) & 0x8000))
            stopPressed = false;
        }

        if (stopPressed && m_countdownStopPressedState == false)
        {
          stopCountdown();
        }
        m_countdownStopPressedState = stopPressed;
      }

      if (m_config.countdown.hotkeyResetVk != 0)
      {
        bool resetPressed = (GetAsyncKeyState(m_config.countdown.hotkeyResetVk) & 0x8000) != 0;
        if (resetPressed && m_config.countdown.hotkeyResetMods != 0)
        {
          if ((m_config.countdown.hotkeyResetMods & 1) && !(GetAsyncKeyState(VK_CONTROL) & 0x8000))
            resetPressed = false;
          if ((m_config.countdown.hotkeyResetMods & 2) && !(GetAsyncKeyState(VK_SHIFT) & 0x8000))
            resetPressed = false;
          if ((m_config.countdown.hotkeyResetMods & 4) && !(GetAsyncKeyState(VK_MENU) & 0x8000))
            resetPressed = false;
        }

        if (resetPressed && m_countdownResetPressedState == false)
        {
          resetCountdown();
        }
        m_countdownResetPressedState = resetPressed;
      }

      // Master hotkey toggle
      if (m_config.hotkey != 0)
      {
        bool masterPressed = (GetAsyncKeyState(m_config.hotkey) & 0x8000) != 0;
        if (masterPressed && m_config.hotkeyModifiers != 0)
        {
          if ((m_config.hotkeyModifiers & 1) && !(GetAsyncKeyState(VK_CONTROL) & 0x8000))
            masterPressed = false;
          if ((m_config.hotkeyModifiers & 2) && !(GetAsyncKeyState(VK_SHIFT) & 0x8000))
            masterPressed = false;
          if ((m_config.hotkeyModifiers & 4) && !(GetAsyncKeyState(VK_MENU) & 0x8000))
            masterPressed = false;
        }

        if (masterPressed && !m_masterHotkeyState)
        {
          bool newVisible = !m_config.visible;
          if (!newVisible)
          {
            m_config.countdown.enabled = false;
            m_config.countdownRuntime.enabled = false;
            m_config.countdownRuntime.startTimestampMs = 0;
            m_config.countdownRuntime.pausedRemainingMs = -1;
          }
          else
          {
            m_config.countdown.enabled = true;
          }
          m_config.visible = newVisible;
          markDirty();
        }
        m_masterHotkeyState = masterPressed;
      }
    }
  };

} // namespace overlayx
