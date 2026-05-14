import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

ColumnLayout {
    spacing: 16

    Text {
        text: "Settings"
        font.pixelSize: 24
        font.bold: true
        color: "#f0f0f2"
    }

    Item { Layout.preferredHeight: 4 }

    Text { text: "VISIBILITY"; font.pixelSize: 12; font.letterSpacing: 1; color: "#6b6b70" }

    Switch {
        text: "Master Visibility"
        checked: backend ? backend.visible : true
        onToggled: if(backend) backend.visible = checked
        Material.accent: "#618FF0"
    }

    Item { Layout.preferredHeight: 8 }

    Text { text: "EDIT MODE"; font.pixelSize: 12; font.letterSpacing: 1; color: "#6b6b70" }

    Switch {
        text: "Edit Mode (Drag Overlay)"
        checked: backend ? backend.editMode : false
        onToggled: if(backend) backend.editMode = checked
        Material.accent: "#618FF0"
    }

    Text {
        text: "Enable to manually drag the crosshair to a new position."
        font.pixelSize: 13
        color: "#6b6b70"
        Layout.leftMargin: 4
    }
}
