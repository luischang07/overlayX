import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

ColumnLayout {
    id: hotkeysRoot
    spacing: 24
    Layout.margins: 32
    Layout.fillHeight: true
    Layout.fillWidth: true

    Text {
        text: "Hotkeys"
        font.pixelSize: 28
        font.weight: Font.Bold
        color: "#f0f0f2"
    }

    // ─── Global Toggle ──────────────────────────────────────
    ColumnLayout {
        spacing: 12
        Layout.fillWidth: true

        Text { 
            text: "GLOBAL OVERLAY TOGGLE"
            font.pixelSize: 12
            font.letterSpacing: 1.2
            font.weight: Font.DemiBold
            color: "#6b6b70" 
        }

        RowLayout {
            spacing: 16
            
            // Keybind Display
            Rectangle {
                Layout.preferredWidth: 180
                Layout.preferredHeight: 36
                color: backend ? (backend.listeningInstanceId === "global" ? "#3d2b1f" : "#0d0d0f") : "#0d0d0f"
                radius: 6
                border.color: backend ? (backend.listeningInstanceId === "global" ? "#ffb74d" : "#2a2a2e") : "#2a2a2e"

                Text {
                    anchors.centerIn: parent
                    text: backend ? (backend.listeningInstanceId === "global" ? "Listening..." : backend.globalHotkeyString) : ""
                    color: backend ? (backend.listeningInstanceId === "global" ? "#ffb74d" : "#f0f0f2") : "#f0f0f2"
                    font.pixelSize: 13
                    font.family: "Consolas"
                }
            }

            Button {
                text: "Assign"
                Layout.preferredWidth: 90
                Layout.preferredHeight: 36
                Material.background: "#2a2a2e"
                enabled: backend ? (backend.listeningInstanceId === "") : false
                onClicked: if(backend) backend.startListeningForInstanceHotkey("global")
            }

            Button {
                text: "Clear"
                Layout.preferredWidth: 90
                Layout.preferredHeight: 36
                Material.background: "#2a2a2e"
                enabled: backend ? (backend.listeningInstanceId === "" && (backend.configUpdateTick, backend.globalHotkeyString !== "None")) : false
                onClicked: if(backend) backend.clearGlobalHotkey()
            }

            Text {
                text: "Hide or show the entire overlay globally."
                font.pixelSize: 13
                color: "#6b6b70"
            }
        }
    }

    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 1
        color: "#2a2a2e"
    }

    // ─── Preset Instance Hotkeys ───────────────────────────
    ColumnLayout {
        spacing: 16
        Layout.fillWidth: true
        Layout.fillHeight: true

        Text { 
            text: "ACTIVE PRESET VISIBILITY"
            font.pixelSize: 12
            font.letterSpacing: 1.2
            font.weight: Font.DemiBold
            color: "#6b6b70" 
        }

        ListView {
            id: presetHotkeyListView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: backend ? (backend.configUpdateTick, backend.activePresetInstances) : []
            spacing: 8

            delegate: Rectangle {
                width: presetHotkeyListView.width
                height: 64
                color: "#1a1a1c"
                radius: 8
                border.color: "#2a2a2e"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 16
                    anchors.rightMargin: 16
                    spacing: 16

                    ColumnLayout {
                        spacing: 2
                        Layout.fillWidth: true
                        Text {
                            text: modelData.name
                            font.pixelSize: 15
                            font.weight: Font.Medium
                            color: "#f0f0f2"
                        }
                        Text {
                            text: "Instance ID: " + modelData.presetId
                            font.pixelSize: 11
                            color: "#6b6b70"
                            elide: Text.ElideRight
                            Layout.maximumWidth: 200
                        }
                    }

                    // Keybind Display
                    Rectangle {
                        Layout.preferredWidth: 180
                        Layout.preferredHeight: 36
                        color: backend ? (backend.listeningInstanceId === modelData.presetId ? "#3d2b1f" : "#0d0d0f") : "#0d0d0f"
                        radius: 6
                        border.color: backend ? (backend.listeningInstanceId === modelData.presetId ? "#ffb74d" : "#2a2a2e") : "#2a2a2e"

                        Text {
                            anchors.centerIn: parent
                            text: backend ? (backend.listeningInstanceId === modelData.presetId ? "Listening..." : modelData.hotkey) : ""
                            color: backend ? (backend.listeningInstanceId === modelData.presetId ? "#ffb74d" : "#f0f0f2") : "#f0f0f2"
                            font.pixelSize: 13
                            font.family: "Consolas"
                        }
                    }

                    Button {
                        text: "Assign"
                        Layout.preferredWidth: 90
                        Layout.preferredHeight: 36
                        Material.background: "#2a2a2e"
                        enabled: backend ? backend.listeningInstanceId === "" : false
                        onClicked: if(backend) backend.startListeningForInstanceHotkey(modelData.presetId)
                    }

                    Button {
                        text: "Clear"
                        Layout.preferredWidth: 90
                        Layout.preferredHeight: 36
                        enabled: backend ? (backend.listeningInstanceId === "" && (backend.configUpdateTick, modelData.hotkey !== "None")) : false
                        Material.background: "#2a2a2e"
                        onClicked: if(backend) backend.clearInstanceHotkey(modelData.presetId)
                    }
                }
            }

            footer: Text {
                visible: presetHotkeyListView.count === 0
                text: "No active presets found. Add some from the Presets tab."
                color: "#6b6b70"
                font.pixelSize: 14
                topPadding: 20
            }
        }
    }

    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 1
        color: "#2a2a2e"
    }

    // ─── Countdown Hotkeys ────────────────────────────────
    ColumnLayout {
        spacing: 16
        Layout.fillWidth: true

        Text { 
            text: "COUNTDOWN TIMER CONTROLS"
            font.pixelSize: 12
            font.letterSpacing: 1.2
            font.weight: Font.DemiBold
            color: "#6b6b70" 
        }

        // Start Hotkey
        RowLayout {
            spacing: 16
            Layout.fillWidth: true

            Text {
                text: "Start:"
                color: "#f0f0f2"
                font.pixelSize: 13
                Layout.preferredWidth: 80
            }

            Rectangle {
                Layout.preferredWidth: 180
                Layout.preferredHeight: 36
                color: backend ? (backend.listeningInstanceId === "countdown_start" ? "#3d2b1f" : "#0d0d0f") : "#0d0d0f"
                radius: 6
                border.color: backend ? (backend.listeningInstanceId === "countdown_start" ? "#ffb74d" : "#2a2a2e") : "#2a2a2e"

                Text {
                    anchors.centerIn: parent
                    text: backend ? (backend.listeningInstanceId === "countdown_start" ? "Listening..." : backend.countdownStartHotkey) : ""
                    color: backend ? (backend.listeningInstanceId === "countdown_start" ? "#ffb74d" : "#f0f0f2") : "#f0f0f2"
                    font.pixelSize: 13
                    font.family: "Consolas"
                }
            }

            Button {
                text: "Assign"
                Layout.preferredWidth: 90
                Layout.preferredHeight: 36
                Material.background: "#2a2a2e"
                enabled: backend ? (backend.listeningInstanceId === "") : false
                onClicked: if(backend) backend.startListeningForCountdownHotkey("start")
            }

            Button {
                text: "Clear"
                Layout.preferredWidth: 90
                Layout.preferredHeight: 36
                Material.background: "#2a2a2e"
                enabled: backend ? (backend.listeningInstanceId === "" && (backend.configUpdateTick, backend.countdownStartHotkey !== "None")) : false
                onClicked: if(backend) backend.clearCountdownHotkey("start")
            }

            Item { Layout.fillWidth: true }
        }

        // Stop Hotkey
        RowLayout {
            spacing: 16
            Layout.fillWidth: true

            Text {
                text: "Stop:"
                color: "#f0f0f2"
                font.pixelSize: 13
                Layout.preferredWidth: 80
            }

            Rectangle {
                Layout.preferredWidth: 180
                Layout.preferredHeight: 36
                color: backend ? (backend.listeningInstanceId === "countdown_stop" ? "#3d2b1f" : "#0d0d0f") : "#0d0d0f"
                radius: 6
                border.color: backend ? (backend.listeningInstanceId === "countdown_stop" ? "#ffb74d" : "#2a2a2e") : "#2a2a2e"

                Text {
                    anchors.centerIn: parent
                    text: backend ? (backend.listeningInstanceId === "countdown_stop" ? "Listening..." : backend.countdownStopHotkey) : ""
                    color: backend ? (backend.listeningInstanceId === "countdown_stop" ? "#ffb74d" : "#f0f0f2") : "#f0f0f2"
                    font.pixelSize: 13
                    font.family: "Consolas"
                }
            }

            Button {
                text: "Assign"
                Layout.preferredWidth: 90
                Layout.preferredHeight: 36
                Material.background: "#2a2a2e"
                enabled: backend ? (backend.listeningInstanceId === "") : false
                onClicked: if(backend) backend.startListeningForCountdownHotkey("stop")
            }

            Button {
                text: "Clear"
                Layout.preferredWidth: 90
                Layout.preferredHeight: 36
                Material.background: "#2a2a2e"
                enabled: backend ? (backend.listeningInstanceId === "" && (backend.configUpdateTick, backend.countdownStopHotkey !== "None")) : false
                onClicked: if(backend) backend.clearCountdownHotkey("stop")
            }

            Item { Layout.fillWidth: true }
        }

        // Reset Hotkey
        RowLayout {
            spacing: 16
            Layout.fillWidth: true

            Text {
                text: "Reset:"
                color: "#f0f0f2"
                font.pixelSize: 13
                Layout.preferredWidth: 80
            }

            Rectangle {
                Layout.preferredWidth: 180
                Layout.preferredHeight: 36
                color: backend ? (backend.listeningInstanceId === "countdown_reset" ? "#3d2b1f" : "#0d0d0f") : "#0d0d0f"
                radius: 6
                border.color: backend ? (backend.listeningInstanceId === "countdown_reset" ? "#ffb74d" : "#2a2a2e") : "#2a2a2e"

                Text {
                    anchors.centerIn: parent
                    text: backend ? (backend.listeningInstanceId === "countdown_reset" ? "Listening..." : backend.countdownResetHotkey) : ""
                    color: backend ? (backend.listeningInstanceId === "countdown_reset" ? "#ffb74d" : "#f0f0f2") : "#f0f0f2"
                    font.pixelSize: 13
                    font.family: "Consolas"
                }
            }

            Button {
                text: "Assign"
                Layout.preferredWidth: 90
                Layout.preferredHeight: 36
                Material.background: "#2a2a2e"
                enabled: backend ? (backend.listeningInstanceId === "") : false
                onClicked: if(backend) backend.startListeningForCountdownHotkey("reset")
            }

            Button {
                text: "Clear"
                Layout.preferredWidth: 90
                Layout.preferredHeight: 36
                Material.background: "#2a2a2e"
                enabled: backend ? (backend.listeningInstanceId === "" && (backend.configUpdateTick, backend.countdownResetHotkey !== "None")) : false
                onClicked: if(backend) backend.clearCountdownHotkey("reset")
            }

            Item { Layout.fillWidth: true }
        }

        // Toggle Detection Hotkey
        RowLayout {
            spacing: 16
            Layout.fillWidth: true

            Text {
                text: "Detection:"
                color: "#f0f0f2"
                font.pixelSize: 13
                Layout.preferredWidth: 80
            }

            Rectangle {
                Layout.preferredWidth: 180
                Layout.preferredHeight: 36
                color: backend ? (backend.listeningInstanceId === "countdown_detection" ? "#3d2b1f" : "#0d0d0f") : "#0d0d0f"
                radius: 6
                border.color: backend ? (backend.listeningInstanceId === "countdown_detection" ? "#ffb74d" : "#2a2a2e") : "#2a2a2e"

                Text {
                    anchors.centerIn: parent
                    text: backend ? (backend.listeningInstanceId === "countdown_detection" ? "Listening..." : backend.countdownDetectionHotkey) : ""
                    color: backend ? (backend.listeningInstanceId === "countdown_detection" ? "#ffb74d" : "#f0f0f2") : "#f0f0f2"
                    font.pixelSize: 13
                    font.family: "Consolas"
                }
            }

            Button {
                text: "Assign"
                Layout.preferredWidth: 90
                Layout.preferredHeight: 36
                Material.background: "#2a2a2e"
                enabled: backend ? (backend.listeningInstanceId === "") : false
                onClicked: if(backend) backend.startListeningForCountdownHotkey("detection")
            }

            Button {
                text: "Clear"
                Layout.preferredWidth: 90
                Layout.preferredHeight: 36
                Material.background: "#2a2a2e"
                enabled: backend ? (backend.listeningInstanceId === "" && (backend.configUpdateTick, backend.countdownDetectionHotkey !== "None")) : false
                onClicked: if(backend) backend.clearCountdownHotkey("detection")
            }

            Item { Layout.fillWidth: true }
        }
    }
}
