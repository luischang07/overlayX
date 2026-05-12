import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

ColumnLayout {
    spacing: 16

    Text {
        text: "Hotkeys"
        font.pixelSize: 24
        font.bold: true
        color: "#f0f0f2"
    }

    Text { text: "TOGGLE HOTKEY"; font.pixelSize: 12; font.letterSpacing: 1; color: "#6b6b70" }

    Text {
        text: "Select a function key to toggle overlay visibility."
        font.pixelSize: 13
        color: "#6b6b70"
    }

    ListView {
        id: hotkeyList
        Layout.fillWidth: true
        Layout.preferredWidth: 240
        Layout.preferredHeight: 320
        clip: true
        model: ["F1","F2","F3","F4","F5","F6","F7","F8","F9","F10","F11","F12"]

        delegate: ItemDelegate {
            width: 240
            height: 40
            highlighted: (backend.hotkey - 0x70) === index

            contentItem: Text {
                text: modelData
                font.pixelSize: 14
                color: highlighted ? "#618FF0" : "#f0f0f2"
                verticalAlignment: Text.AlignVCenter
            }

            background: Rectangle {
                color: highlighted ? "#618FF01A" : (hovered ? "#ffffff0A" : "transparent")
                radius: 6
            }

            onClicked: backend.hotkey = 0x70 + index
        }
    }
}
