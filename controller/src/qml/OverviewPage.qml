import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

ColumnLayout {
    spacing: 16

    Text {
        text: "Overview"
        font.pixelSize: 24
        font.bold: true
        color: "#f0f0f2"
    }

    Text {
        text: "Welcome to OverlayX. Use the Crosshair Designer to build complex, multi-layered crosshairs. Save presets to switch between configurations instantly."
        font.pixelSize: 14
        color: "#a0a0a6"
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
        Layout.rightMargin: 32
    }

    Item { Layout.preferredHeight: 8 }

    Text {
        text: "QUICK ACTIONS"
        font.pixelSize: 12
        font.letterSpacing: 1
        color: "#6b6b70"
    }

    Text {
        text: "Active Layers: " + (backend ? backend.layerCount : 0)
        font.pixelSize: 15
        color: "#f0f0f2"
    }

    RowLayout {
        spacing: 12

        Button {
            text: "Enable All Layers"
            Material.background: "#618FF0"
            Material.foreground: "#ffffff"
            implicitWidth: 180
            implicitHeight: 40
            onClicked: if(backend) backend.enableAllLayers()
        }

        Button {
            text: "Disable All Layers"
            implicitWidth: 180
            implicitHeight: 40
            onClicked: if(backend) backend.disableAllLayers()
        }
    }
}
