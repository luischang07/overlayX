import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Dialogs

ColumnLayout {
    id: countdownRoot
    spacing: 24
    Layout.margins: 24
    Layout.fillHeight: true
    Layout.fillWidth: true

    Text {
        text: "Countdown Timer"
        font.pixelSize: 28
        font.weight: Font.Bold
        color: "#f0f0f2"
    }

    // ─── Enable/Disable + Control Buttons ──────────────────
    ColumnLayout {
        spacing: 12
        Layout.fillWidth: true

        Text { 
            text: "SETTINGS"
            font.pixelSize: 12
            font.letterSpacing: 1.2
            font.weight: Font.DemiBold
            color: "#6b6b70" 
        }

        RowLayout {
            spacing: 12
            Layout.fillWidth: true

            Switch {
                text: "Enable Countdown Timer"
                checked: backend ? backend.countdownEnabled : false
                onToggled: if(backend) backend.countdownEnabled = checked
                Material.accent: "#618FF0"
            }

            Switch {
                text: "Screen Detection"
                checked: backend ? backend.countdownDetectionEnabled : false
                onToggled: if(backend) backend.countdownDetectionEnabled = checked
                Material.accent: "#618FF0"
                ToolTip.visible: hovered
                ToolTip.text: "Automatically start countdown when spike is detected on screen"
            }

            Item { Layout.fillWidth: true }

            Button {
                text: "Start"
                Layout.preferredWidth: 88
                Layout.preferredHeight: 36
                Material.background: "#4CAF50"
                Material.foreground: "#ffffff"
                onClicked: backend.startCountdown()
            }

            Button {
                text: "Stop"
                Layout.preferredWidth: 88
                Layout.preferredHeight: 36
                Material.background: "#f44336"
                Material.foreground: "#ffffff"
                onClicked: backend.stopCountdown()
            }

            Button {
                text: "Reset"
                Layout.preferredWidth: 88
                Layout.preferredHeight: 36
                Material.background: "#2196F3"
                Material.foreground: "#ffffff"
                onClicked: backend.resetCountdown()
            }
        }
    }

    // ─── Duration ─────────────────────────────────────────
    ColumnLayout {
        spacing: 12
        Layout.fillWidth: true

        Text { 
            text: "Duration (seconds):"
            color: "#f0f0f2"
            font.pixelSize: 13
            font.weight: Font.DemiBold
        }

        RowLayout {
            spacing: 16
            Layout.fillWidth: true

            SpinBox {
                from: 1
                to: 3600
                value: backend ? backend.countdownDuration : 45
                onValueModified: if(backend) backend.countdownDuration = value
                Material.accent: "#618FF0"
                Layout.preferredWidth: 164
                font.pixelSize: 16
            }

            TextField {
                text: backend ? backend.countdownDuration.toString() : "45"
                Layout.preferredWidth: 100
                implicitWidth: 100
                function commitValue() {
                    let val = parseInt(text)
                    if (!isNaN(val) && val >= 1 && val <= 3600) {
                        backend.countdownDuration = val
                    }
                }
                onEditingFinished: commitValue()
                onActiveFocusChanged: {
                    if (!activeFocus) commitValue()
                }
                validator: IntValidator { bottom: 1; top: 3600 }
                Material.accent: "#618FF0"
                font.pixelSize: 16
                placeholderText: "Enter seconds"
            }

            Text {
                text: "max: 3600"
                color: "#6b6b70"
                font.pixelSize: 12
            }

            Item { Layout.fillWidth: true }
        }
    }

    // ─── Position ─────────────────────────────────────────
    ColumnLayout {
        spacing: 12
        Layout.fillWidth: true

        Text { 
            text: "Position on Screen"
            color: "#f0f0f2"
            font.pixelSize: 13
            font.weight: Font.DemiBold
        }

        RowLayout {
            spacing: 12
            Layout.fillWidth: true

            Text { 
                text: "X:"
                color: "#a0a0a6"
                font.pixelSize: 12
                Layout.preferredWidth: 25
            }

            Slider {
                from: 0.0
                to: 1.0
                value: backend ? backend.countdownPosX : 0.5
                onValueChanged: if(backend) backend.countdownPosX = value
                Layout.preferredWidth: 150
                Material.accent: "#618FF0"
            }

            TextField {
                text: backend ? (backend.countdownPosX * 100).toFixed(1) : "50.0"
                function commitValue() {
                    let val = parseFloat(text)
                    if (!isNaN(val) && val >= 0 && val <= 100) {
                        backend.countdownPosX = val / 100.0
                    }
                }
                onEditingFinished: commitValue()
                onActiveFocusChanged: {
                    if (!activeFocus) commitValue()
                }
                validator: DoubleValidator { bottom: 0; top: 100 }
                Material.accent: "#618FF0"
                Layout.preferredWidth: 70
                placeholderText: "%"
            }

            Item { Layout.fillWidth: true }
        }

        RowLayout {
            spacing: 12
            Layout.fillWidth: true

            Text { 
                text: "Y:"
                color: "#a0a0a6"
                font.pixelSize: 12
                Layout.preferredWidth: 25
            }

            Slider {
                from: 0.0
                to: 1.0
                value: backend ? backend.countdownPosY : 0.5
                onValueChanged: if(backend) backend.countdownPosY = value
                Layout.preferredWidth: 150
                Material.accent: "#618FF0"
            }

            TextField {
                text: backend ? (backend.countdownPosY * 100).toFixed(1) : "50.0"
                function commitValue() {
                    let val = parseFloat(text)
                    if (!isNaN(val) && val >= 0 && val <= 100) {
                        backend.countdownPosY = val / 100.0
                    }
                }
                onEditingFinished: commitValue()
                onActiveFocusChanged: {
                    if (!activeFocus) commitValue()
                }
                validator: DoubleValidator { bottom: 0; top: 100 }
                Material.accent: "#618FF0"
                Layout.preferredWidth: 70
                placeholderText: "%"
            }

            Item { Layout.fillWidth: true }
        }
    }

    // ─── Font Size ────────────────────────────────────────
    ColumnLayout {
        spacing: 12
        Layout.fillWidth: true

        Text { 
            text: "Font Size (pixels):"
            color: "#f0f0f2"
            font.pixelSize: 13
            font.weight: Font.DemiBold
        }

        RowLayout {
            spacing: 16
            Layout.fillWidth: true

            SpinBox {
                from: 8
                to: 200
                value: backend ? backend.countdownFontSize : 48
                onValueModified: if(backend) backend.countdownFontSize = value
                Material.accent: "#618FF0"
                Layout.preferredWidth: 164
                font.pixelSize: 16
            }

            TextField {
                text: backend ? backend.countdownFontSize.toString() : "48"
                Layout.preferredWidth: 100
                implicitWidth: 100
                function commitValue() {
                    let val = parseInt(text)
                    if (!isNaN(val) && val >= 8 && val <= 200) {
                        backend.countdownFontSize = val
                    }
                }
                onEditingFinished: commitValue()
                onActiveFocusChanged: {
                    if (!activeFocus) commitValue()
                }
                validator: IntValidator { bottom: 8; top: 200 }
                Material.accent: "#618FF0"
                font.pixelSize: 16
                placeholderText: "Enter pixels"
            }

            Text {
                text: "8-200 px"
                color: "#6b6b70"
                font.pixelSize: 12
            }

            Item { Layout.fillWidth: true }
        }

        // Preview removed per UX request
    }

    // ─── Color ────────────────────────────────────────────
    ColumnLayout {
        spacing: 12
        Layout.fillWidth: true

        Text { 
            text: "Color:"
            color: "#f0f0f2"
            font.pixelSize: 13
            font.weight: Font.DemiBold
        }

        RowLayout {
            spacing: 16

            Rectangle {
                Layout.preferredWidth: 72
                Layout.preferredHeight: 40
                radius: 6
                color: backend ? backend.countdownColorHex : "#ffffff"
                border.color: "#2a2a2e"
                border.width: 2

                MouseArea {
                    anchors.fill: parent
                    onClicked: colorDialog.open()
                }
            }

            TextField {
                text: backend ? backend.countdownColorHex : "#ffffff"
                function commitValue() {
                    let hex = text.trim()
                    if (/^#[0-9a-fA-F]{8}$/.test(hex)) {
                        backend.countdownColorHex = hex
                    }
                }
                onEditingFinished: commitValue()
                onActiveFocusChanged: {
                    if (!activeFocus) commitValue()
                }
                Material.accent: "#618FF0"
                Layout.preferredWidth: 140
                placeholderText: "#ffffffff"
                font.family: "Consolas"
            }

            Button {
                text: "Pick"
                Layout.preferredWidth: 80
                Layout.preferredHeight: 36
                Material.background: "#2a2a2e"
                onClicked: colorDialog.open()
            }

            Item { Layout.fillWidth: true }
        }
    }

    Item { Layout.fillHeight: true }

    ColorDialog {
        id: colorDialog
        options: ColorDialog.ShowAlphaChannel
        selectedColor: backend ? backend.countdownColorHex : "#ffffff"
        onAccepted: backend.countdownColorHex = selectedColor.toString()
    }
}
