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
#include <windows.h> // For GetTickCount64

namespace overlayx {

/// QObject backend bridging OverlayConfig to QML.
/// Exposes all config properties, manages IPC + persistence.
class AppBackend : public QObject {
    Q_OBJECT

    // ─── Scalar Properties ─────────────────────────────────
    Q_PROPERTY(bool visible    READ visible    WRITE setVisible    NOTIFY configChanged)
    Q_PROPERTY(bool editMode   READ editMode   WRITE setEditMode   NOTIFY configChanged)
    Q_PROPERTY(float posX      READ posX       WRITE setPosX       NOTIFY configChanged)
    Q_PROPERTY(float posY      READ posY       WRITE setPosY       NOTIFY configChanged)
    Q_PROPERTY(int   hotkey    READ hotkey     WRITE setHotkey     NOTIFY configChanged)
    Q_PROPERTY(bool  engineConnected READ engineConnected NOTIFY engineStatusChanged)
    Q_PROPERTY(int   selectedLayerIndex READ selectedLayerIndex WRITE setSelectedLayerIndex NOTIFY selectedLayerIndexChanged)

    // ─── Collections ───────────────────────────────────────
    Q_PROPERTY(QVariantList layers         READ layers         NOTIFY configChanged)
    Q_PROPERTY(QVariantList savedPositions READ savedPositions NOTIFY configChanged)
    Q_PROPERTY(QVariantList savedPresets   READ savedPresets   NOTIFY configChanged)
    Q_PROPERTY(QVariantList activePresetInstances READ activePresetInstances NOTIFY configChanged)
    Q_PROPERTY(int layerCount READ layerCount NOTIFY configChanged)

public:
    explicit AppBackend(std::unique_ptr<IConfigRepository> repo,
                        std::unique_ptr<IIPCServer> ipc,
                        QObject* parent = nullptr)
        : QObject(parent)
        , m_repo(std::move(repo))
        , m_ipc(std::move(ipc))
    {
        auto loaded = m_repo->load();
        if (loaded) m_config = *loaded;

        m_ipc->start();

        // Auto-save every 5 seconds if dirty
        m_saveTimer.setInterval(5000);
        connect(&m_saveTimer, &QTimer::timeout, this, [this]() {
            if (m_dirty) {
                m_repo->save(m_config);
                m_dirty = false;
            }
        });
        m_saveTimer.start();

        // Poll engine status every 500ms
        m_statusTimer.setInterval(500);
        connect(&m_statusTimer, &QTimer::timeout, this, [this]() {
            bool connected = m_ipc->isClientConnected();
            if (connected != m_lastEngineStatus) {
                m_lastEngineStatus = connected;
                emit engineStatusChanged();
            }
        });
        m_statusTimer.start();
    }

    ~AppBackend() override {
        m_repo->save(m_config);
        m_config.visible = false;
        m_config.shouldExit = true;
        m_ipc->sendConfig(m_config);
        m_ipc->stop();
    }

    // ─── Property Accessors ────────────────────────────────
    bool  visible()  const { return m_config.visible; }
    bool  editMode() const { return m_config.editMode; }
    float posX()     const { return m_config.posX; }
    float posY()     const { return m_config.posY; }
    int   hotkey()   const { return m_config.hotkey; }
    bool  engineConnected() const { return m_ipc->isClientConnected(); }
    int   layerCount() const { return (int)m_config.layers.size(); }

    void setVisible(bool v)   { if (m_config.visible  != v) { m_config.visible  = v; markDirty(); } }
    void setEditMode(bool v)  { if (m_config.editMode != v) { m_config.editMode = v; markDirty(); } }
    void setPosX(float v)     { if (m_config.posX     != v) { m_config.posX     = v; markDirty(); } }
    void setPosY(float v)     { if (m_config.posY     != v) { m_config.posY     = v; markDirty(); } }
    void setHotkey(int v)     { if (m_config.hotkey   != v) { m_config.hotkey   = v; markDirty(); } }

    int selectedLayerIndex() const { return m_selectedLayerIndex; }
    void setSelectedLayerIndex(int i) {
        if (m_selectedLayerIndex != i) {
            m_selectedLayerIndex = i;
            emit selectedLayerIndexChanged();
        }
    }

    // ─── Layer Collection ──────────────────────────────────
    QVariantList layers() const {
        QVariantList list;
        for (int i = 0; i < (int)m_config.layers.size(); ++i) {
            list.append(layerToMap(m_config.layers[i], i));
        }
        return list;
    }

    // ─── Saved Positions ───────────────────────────────────
    QVariantList savedPositions() const {
        QVariantList list;
        for (auto& p : m_config.savedPositions) {
            QVariantMap m;
            m["name"] = QString::fromStdString(p.name);
            m["x"] = p.x;
            m["y"] = p.y;
            list.append(m);
        }
        return list;
    }

    QVariantList savedPresets() const {
        QVariantList list;
        for (auto& pr : m_config.savedPresets) {
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

    QVariantList activePresetInstances() const {
        QVariantList list;
        std::vector<std::string> seenIds;
        
        for (const auto& l : m_config.layers) {
            if (l.presetId.empty()) continue;
            
            if (std::find(seenIds.begin(), seenIds.end(), l.presetId) == seenIds.end()) {
                seenIds.push_back(l.presetId);
                
                QVariantMap m;
                m["presetId"] = QString::fromStdString(l.presetId);
                
                // Find preset name from UID (prefix of instanceId)
                QString name = "Unknown Preset";
                size_t underscorePos = l.presetId.find('_');
                if (underscorePos != std::string::npos) {
                    std::string uid = l.presetId.substr(0, underscorePos);
                    for (const auto& pr : m_config.savedPresets) {
                        if (pr.uid == uid) {
                            name = QString::fromStdString(pr.name);
                            break;
                        }
                    }
                }
                
                m["name"] = name;
                m["posX"] = l.posX;
                m["posY"] = l.posY;
                list.append(m);
            }
        }
        return list;
    }

    // ═══════════════════════════════════════════════════════
    // Invokable methods (called from QML)
    // ═══════════════════════════════════════════════════════

    Q_INVOKABLE void centerCrosshair() {
        if (validLayer(m_selectedLayerIndex)) {
            updateLayerPosition(m_selectedLayerIndex, 0.5f, false); // Center X
            updateLayerPosition(m_selectedLayerIndex, 0.5f, true);  // Center Y
        }
    }

    Q_INVOKABLE void enableAllLayers() {
        for (auto& l : m_config.layers) l.enabled = true;
        markDirty();
    }

    Q_INVOKABLE void disableAllLayers() {
        for (auto& l : m_config.layers) l.enabled = false;
        markDirty();
    }

    // ─── Layer CRUD ────────────────────────────────────────
    Q_INVOKABLE void addLayer() {
        m_config.layers.push_back(
            Layer{"Layer " + std::to_string(m_config.layers.size())});
        setSelectedLayerIndex((int)m_config.layers.size() - 1);
        markDirty();
    }

    Q_INVOKABLE void removeLayer(int index) {
        if (index >= 0 && index < (int)m_config.layers.size()) {
            m_config.layers.erase(m_config.layers.begin() + index);
            if (m_selectedLayerIndex >= (int)m_config.layers.size()) {
                setSelectedLayerIndex(std::max(0, (int)m_config.layers.size() - 1));
            }
            markDirty();
        }
    }

    Q_INVOKABLE void duplicateLayer(int index) {
        if (index >= 0 && index < (int)m_config.layers.size()) {
            m_config.layers.push_back(m_config.layers[index]);
            markDirty();
        }
    }

    Q_INVOKABLE void moveLayer(int from, int to) {
        if (from >= 0 && from < (int)m_config.layers.size() &&
            to >= 0 && to < (int)m_config.layers.size() && from != to) {
            auto item = m_config.layers[from];
            m_config.layers.erase(m_config.layers.begin() + from);
            m_config.layers.insert(m_config.layers.begin() + to, item);
            markDirty();
        }
    }

    // ─── Layer Property Setters ────────────────────────────
    Q_INVOKABLE void setLayerName(int i, const QString& name) {
        if (validLayer(i)) { m_config.layers[i].name = name.toStdString(); markDirty(); }
    }
    Q_INVOKABLE void setLayerEnabled(int i, bool v) {
        if (validLayer(i)) { m_config.layers[i].enabled = v; markDirty(); }
    }
    Q_INVOKABLE void setLayerShape(int i, int shape) {
        if (validLayer(i)) { m_config.layers[i].shape = (ShapeType)shape; markDirty(); }
    }
    Q_INVOKABLE void setLayerWidth(int i, float v) {
        if (validLayer(i)) { m_config.layers[i].width = v; markDirty(); }
    }
    Q_INVOKABLE void setLayerHeight(int i, float v) {
        if (validLayer(i)) { m_config.layers[i].height = v; markDirty(); }
    }
    Q_INVOKABLE void setLayerThickness(int i, float v) {
        if (validLayer(i)) { m_config.layers[i].thickness = v; markDirty(); }
    }
    Q_INVOKABLE void setLayerGap(int i, float v) {
        if (validLayer(i)) { m_config.layers[i].gap = v; markDirty(); }
    }
    Q_INVOKABLE void setLayerPosX(int i, float v) { updateLayerPosition(i, v, false); }
    Q_INVOKABLE void setLayerPosY(int i, float v) { updateLayerPosition(i, v, true); }
    Q_INVOKABLE void setLayerRotation(int i, float v) {
        if (validLayer(i)) { m_config.layers[i].rotation = v; markDirty(); }
    }
    Q_INVOKABLE void setLayerColor(int i, const QString& hex) {
        if (!validLayer(i)) return;
        QColor qc = QColor::fromString(hex);
        m_config.layers[i].color.r = qc.red();
        m_config.layers[i].color.g = qc.green();
        m_config.layers[i].color.b = qc.blue();
        m_config.layers[i].color.a = qc.alpha();
        markDirty();
    }
    Q_INVOKABLE void setLayerOutlineEnabled(int i, bool v) {
        if (validLayer(i)) { m_config.layers[i].outlineEnabled = v; markDirty(); }
    }
    Q_INVOKABLE void setLayerOutlineColor(int i, const QString& hex) {
        if (!validLayer(i)) return;
        QColor qc = QColor::fromString(hex);
        m_config.layers[i].outlineColor.r = qc.red();
        m_config.layers[i].outlineColor.g = qc.green();
        m_config.layers[i].outlineColor.b = qc.blue();
        m_config.layers[i].outlineColor.a = qc.alpha();
        markDirty();
    }
    Q_INVOKABLE void setLayerOutlineThickness(int i, float v) {
        if (validLayer(i)) { m_config.layers[i].outlineThickness = v; markDirty(); }
    }

    Q_INVOKABLE QVariantMap getLayer(int i) const {
        if (i >= 0 && i < (int)m_config.layers.size())
            return layerToMap(m_config.layers[i], i);
        return {};
    }

    // ─── Position Management ───────────────────────────────
    Q_INVOKABLE void savePosition(const QString& name, int layerIndex) {
        if (name.isEmpty() || !validLayer(layerIndex)) return;
        m_config.savedPositions.push_back(
            {name.toStdString(), m_config.layers[layerIndex].posX, m_config.layers[layerIndex].posY});
        markDirty();
    }
    Q_INVOKABLE void loadPosition(int index, int layerIndex) {
        if (index >= 0 && index < (int)m_config.savedPositions.size() && validLayer(layerIndex)) {
            updateLayerPosition(layerIndex, m_config.savedPositions[index].x, false);
            updateLayerPosition(layerIndex, m_config.savedPositions[index].y, true);
        }
    }
    Q_INVOKABLE void deletePosition(int index) {
        if (index >= 0 && index < (int)m_config.savedPositions.size()) {
            m_config.savedPositions.erase(m_config.savedPositions.begin() + index);
            markDirty();
        }
    }
    Q_INVOKABLE void renamePosition(int index, const QString& newName) {
        if (index >= 0 && index < (int)m_config.savedPositions.size() && !newName.isEmpty()) {
            m_config.savedPositions[index].name = newName.toStdString();
            markDirty();
        }
    }

    // ─── Preset Management ─────────────────────────────────
    Q_INVOKABLE void savePreset(const QString& name) {
        if (name.isEmpty()) return;
        CrosshairPreset p;
        p.name = name.toStdString();
        p.uid = generateUid();
        p.layers = m_config.layers;
        // Strip instance-specific IDs when saving as a template
        for (auto& l : p.layers) l.presetId = "";
        
        // Use the position of the first layer as the preset's reference position
        if (!p.layers.empty()) {
            p.posX = p.layers[0].posX;
            p.posY = p.layers[0].posY;
        } else {
            p.posX = 0.5f;
            p.posY = 0.5f;
        }

        m_config.savedPresets.push_back(p);
        markDirty();
    }
    Q_INVOKABLE void loadPreset(int index) {
        if (index >= 0 && index < (int)m_config.savedPresets.size()) {
            m_config.layers = m_config.savedPresets[index].layers;
            // When loading, treat all layers as a fresh group
            std::string instanceId = generateInstanceId(m_config.savedPresets[index].uid);
            for (auto& l : m_config.layers) l.presetId = instanceId;
            markDirty();
        }
    }
    Q_INVOKABLE void deletePreset(int index) {
        if (index >= 0 && index < (int)m_config.savedPresets.size()) {
            m_config.savedPresets.erase(m_config.savedPresets.begin() + index);
            markDirty();
        }
    }

    Q_INVOKABLE void updatePreset(int index) {
        if (index >= 0 && index < (int)m_config.savedPresets.size()) {
            m_config.savedPresets[index].layers = m_config.layers;
            markDirty();
        }
    }

    Q_INVOKABLE void setPresetPosX(int index, float x) {
        if (index >= 0 && index < (int)m_config.savedPresets.size()) {
            m_config.savedPresets[index].posX = x;
            markDirty();
        }
    }

    Q_INVOKABLE void setPresetPosY(int index, float y) {
        if (index >= 0 && index < (int)m_config.savedPresets.size()) {
            m_config.savedPresets[index].posY = y;
            markDirty();
        }
    }

    Q_INVOKABLE void setInstancePosX(const QString& id, float x) {
        std::string sid = id.toStdString();
        float currentX = 0;
        bool found = false;
        for (const auto& l : m_config.layers) {
            if (l.presetId == sid) { currentX = l.posX; found = true; break; }
        }
        if (found) {
            float delta = x - currentX;
            for (auto& l : m_config.layers) {
                if (l.presetId == sid) l.posX += delta;
            }
            markDirty();
        }
    }

    // Live variant: updates engine via IPC but does NOT emit configChanged.
    // Use this during slider drag to avoid destroying the ListView delegate mid-gesture.
    Q_INVOKABLE void setInstancePosXLive(const QString& id, float x) {
        std::string sid = id.toStdString();
        float currentX = 0;
        bool found = false;
        for (const auto& l : m_config.layers) {
            if (l.presetId == sid) { currentX = l.posX; found = true; break; }
        }
        if (found) {
            float delta = x - currentX;
            for (auto& l : m_config.layers) {
                if (l.presetId == sid) l.posX += delta;
            }
            m_dirty = true;
            m_ipc->sendConfig(m_config);
        }
    }

    Q_INVOKABLE void setInstancePosY(const QString& id, float y) {
        std::string sid = id.toStdString();
        float currentY = 0;
        bool found = false;
        for (const auto& l : m_config.layers) {
            if (l.presetId == sid) { currentY = l.posY; found = true; break; }
        }
        if (found) {
            float delta = y - currentY;
            for (auto& l : m_config.layers) {
                if (l.presetId == sid) l.posY += delta;
            }
            markDirty();
        }
    }

    // Live variant: updates engine via IPC but does NOT emit configChanged.
    Q_INVOKABLE void setInstancePosYLive(const QString& id, float y) {
        std::string sid = id.toStdString();
        float currentY = 0;
        bool found = false;
        for (const auto& l : m_config.layers) {
            if (l.presetId == sid) { currentY = l.posY; found = true; break; }
        }
        if (found) {
            float delta = y - currentY;
            for (auto& l : m_config.layers) {
                if (l.presetId == sid) l.posY += delta;
            }
            m_dirty = true;
            m_ipc->sendConfig(m_config);
        }
    }

    Q_INVOKABLE void removeInstance(const QString& id) {
        std::string sid = id.toStdString();
        auto it = std::remove_if(m_config.layers.begin(), m_config.layers.end(),
            [&](const Layer& l) { return l.presetId == sid; });
        if (it != m_config.layers.end()) {
            m_config.layers.erase(it, m_config.layers.end());
            markDirty();
        }
    }

    Q_INVOKABLE void appendPreset(int index) {
        if (index >= 0 && index < (int)m_config.savedPresets.size()) {
            auto& preset = m_config.savedPresets[index];
            if (preset.uid.empty()) preset.uid = generateUid();

            std::string instanceId = generateInstanceId(preset.uid);
            
            // Calculate delta to move all layers relative to the preset's base position
            float deltaX = 0, deltaY = 0;
            if (!preset.layers.empty()) {
                deltaX = preset.posX - preset.layers[0].posX;
                deltaY = preset.posY - preset.layers[0].posY;
            }

            for (const auto& l : preset.layers) {
                Layer nl = l;
                nl.presetId = instanceId;
                nl.posX += deltaX;
                nl.posY += deltaY;
                m_config.layers.push_back(nl);
            }
            markDirty();
        }
    }

    Q_INVOKABLE void removePresetLayers(int index) {
        if (index < 0 || index >= (int)m_config.savedPresets.size()) return;
        
        const auto& presetLayers = m_config.savedPresets[index].layers;
        bool changed = false;

        for (const auto& pLayer : presetLayers) {
            // Find a matching layer in the current config and remove it
            for (auto it = m_config.layers.begin(); it != m_config.layers.end(); ++it) {
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

        if (changed) markDirty();
    }
    Q_INVOKABLE void renamePreset(int index, const QString& newName) {
        if (index >= 0 && index < (int)m_config.savedPresets.size() && !newName.isEmpty()) {
            m_config.savedPresets[index].name = newName.toStdString();
            markDirty();
        }
    }

signals:
    void configChanged();
    void engineStatusChanged();
    void selectedLayerIndexChanged();

private:
    void updateLayerPosition(int i, float val, bool isY) {
        if (!validLayer(i)) return;
        float currentVal = isY ? m_config.layers[i].posY : m_config.layers[i].posX;
        float delta = val - currentVal;
        std::string pid = m_config.layers[i].presetId;

        if (!pid.empty()) {
            // Move entire preset group together when changing base position
            for (auto& l : m_config.layers) {
                if (l.presetId == pid) {
                    if (isY) l.posY += delta; else l.posX += delta;
                }
            }
        } else {
            // Move ONLY the specific layer if it has no presetId
            if (isY) m_config.layers[i].posY = val; else m_config.layers[i].posX = val;
        }
        markDirty();
    }

    std::string generateInstanceId(const std::string& base) {
        static uint64_t counter = 0;
        return base + "_" + std::to_string(GetTickCount64()) + "_" + std::to_string(++counter);
    }

    void markDirty() {
        m_dirty = true;
        m_ipc->sendConfig(m_config);
        emit configChanged();
    }

    bool validLayer(int i) const {
        return i >= 0 && i < (int)m_config.layers.size();
    }

    QVariantMap layerToMap(const Layer& l, int index) const {
        QVariantMap m;
        m["index"]     = index;
        m["name"]      = QString::fromStdString(l.name);
        m["enabled"]   = l.enabled;
        m["shape"]     = (int)l.shape;
        m["width"]     = l.width;
        m["height"]    = l.height;
        m["thickness"] = l.thickness;
        m["gap"]       = l.gap;
        m["posX"]      = l.posX;
        m["posY"]      = l.posY;
        m["rotation"]  = l.rotation;
        m["colorR"]    = l.color.r;
        m["colorG"]    = l.color.g;
        m["colorB"]    = l.color.b;
        m["colorA"]    = l.color.a;
        m["color"]     = QString("#%1%2%3%4")
            .arg(l.color.a, 2, 16, QChar('0'))
            .arg(l.color.r, 2, 16, QChar('0'))
            .arg(l.color.g, 2, 16, QChar('0'))
            .arg(l.color.b, 2, 16, QChar('0'));
        m["outlineEnabled"] = l.outlineEnabled;
        m["outlineColor"]   = QString("#%1%2%3%4")
            .arg(l.outlineColor.a, 2, 16, QChar('0'))
            .arg(l.outlineColor.r, 2, 16, QChar('0'))
            .arg(l.outlineColor.g, 2, 16, QChar('0'))
            .arg(l.outlineColor.b, 2, 16, QChar('0'));
        m["outlineThickness"] = l.outlineThickness;
        m["presetId"]         = QString::fromStdString(l.presetId);
        return m;
    }

    std::string generateUid() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dis(0, 15);
        std::stringstream ss;
        ss << std::hex;
        for (int i = 0; i < 16; i++) ss << dis(gen);
        return ss.str();
    }

    OverlayConfig                    m_config;
    std::unique_ptr<IConfigRepository> m_repo;
    std::unique_ptr<IIPCServer>      m_ipc;
    bool                             m_dirty{false};
    bool                             m_lastEngineStatus{false};
    QTimer                           m_saveTimer;
    QTimer                           m_statusTimer;
    int                              m_selectedLayerIndex{0};
};

} // namespace overlayx
