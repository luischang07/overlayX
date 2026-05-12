import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Dialogs

Item {
    id: pageRoot

    RowLayout {
        id: designerRoot
        anchors.fill: parent
        spacing: 0

        // Force refresh when config changes
        property var layerData: backend.layers
        property int selectedIndex: backend.selectedLayerIndex
        property var selectedLayer: selectedIndex >= 0 && selectedIndex < layerData.length ? layerData[selectedIndex] : null

        // ─── Left: Layer Stack ─────────────────────────────────
        ColumnLayout {
            Layout.preferredWidth: 240
            Layout.fillHeight: true
            Layout.rightMargin: 16
            spacing: 8

            Text {
                text: "LAYER STACK"
                font.pixelSize: 12
                font.letterSpacing: 1
                color: "#6b6b70"
            }

            Button {
                text: "+ Add Layer"
                Material.background: "#618FF0"
                Material.foreground: "#ffffff"
                Layout.fillWidth: true
                implicitHeight: 64
                font.bold: true
                onClicked: backend.addLayer()
            }

            ListView {
                id: layerListView
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: designerRoot.layerData

                delegate: ItemDelegate {
                    id: layerDelegate
                    width: layerListView.width
                    height: 48
                    padding: 0
                    highlighted: designerRoot.selectedIndex === index

                    background: Rectangle {
                        color: layerDelegate.highlighted ? "#1A618FF0" : (layerDelegate.hovered ? "#0AFFFFFF" : "transparent")
                        radius: 6
                    }

                    // Handle selection on left click
                    onClicked: backend.selectedLayerIndex = index

                    // Handle right-click for context menu
                    TapHandler {
                        acceptedButtons: Qt.RightButton
                        onTapped: contextMenu.popup()
                    }

                    contentItem: GridLayout {
                        columns: 3
                        columnSpacing: 12
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 8

                        // Color dot
                        Rectangle {
                            width: 12; height: 12; radius: 6
                            color: modelData.color
                            opacity: modelData.enabled ? 1.0 : 0.3
                            Layout.alignment: Qt.AlignVCenter
                        }

                        Text {
                            text: modelData.name + (modelData.enabled ? "" : " (off)")
                            font.pixelSize: 14
                            color: layerDelegate.highlighted ? "#f0f0f2" : "#a0a0a6"
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                            verticalAlignment: Text.AlignVCenter
                            Layout.alignment: Qt.AlignVCenter
                        }

                        Button {
                            Layout.preferredWidth: 32
                            Layout.preferredHeight: 32
                            Layout.alignment: Qt.AlignVCenter
                            flat: true
                            padding: 0
                            onClicked: {
                                backend.removeLayer(index)
                            }
                            background: Rectangle {
                                color: parent.hovered ? "#d32f2f" : "transparent"
                                radius: 4
                            }
                            contentItem: Text {
                                text: "\u2715"
                                color: "#f0f0f2"
                                font.pixelSize: 18
                                font.bold: true
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }

                    Menu {
                        id: contextMenu
                        MenuItem {
                            text: "Duplicate"
                            onTriggered: backend.duplicateLayer(index)
                        }
                        MenuItem {
                            text: "Delete"
                            onTriggered: {
                                backend.removeLayer(index)
                            }
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: 8
                Layout.bottomMargin: 8
                Button {
                    id: savePresetBtn
                    text: "Save Preset"
                    Material.background: "#618FF0"
                    Material.foreground: "#ffffff"
                    implicitHeight: 40
                    Layout.fillWidth: true
                    font.bold: true
                    onClicked: savePresetDialog.open()
                    contentItem: Text {
                        text: savePresetBtn.text
                        color: "#ffffff"
                        font.bold: true
                        font.pixelSize: 14
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }
        }

        // Separator
        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: 1
            Layout.topMargin: 8
            Layout.bottomMargin: 8
            color: "#2a2a2e"
        }

        // ─── Right: Properties ─────────────────────────────────
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 24
            Layout.maximumWidth: 600 // Reduced from 800 to fit user request
            clip: true

            ColumnLayout {
                spacing: 12
                width: parent ? parent.width - 24 : 400

                Text {
                    text: "PROPERTIES"
                    font.pixelSize: 12
                    font.letterSpacing: 1
                    color: "#6b6b70"
                    visible: designerRoot.selectedLayer !== null
                }

                // Show properties only if a layer is selected
                ColumnLayout {
                    visible: designerRoot.selectedLayer !== null
                    spacing: 10

                    // Name
                    RowLayout {
                        spacing: 12
                        Text { text: "Name"; font.pixelSize: 14; color: "#a0a0a6"; Layout.preferredWidth: 80 }
                        TextField {
                            id: nameField
                            text: designerRoot.selectedLayer ? designerRoot.selectedLayer.name : ""
                            Layout.fillWidth: true
                            implicitHeight: 34
                            padding: 10
                            verticalAlignment: TextInput.AlignVCenter
                            onEditingFinished: backend.setLayerName(backend.selectedLayerIndex, text)
                        }
                    }

                    // Enabled
                    Switch {
                        text: "Enabled"
                        checked: designerRoot.selectedLayer ? designerRoot.selectedLayer.enabled : false
                        onToggled: backend.setLayerEnabled(backend.selectedLayerIndex, checked)
                        Material.accent: "#618FF0"
                    }

                    Item { Layout.preferredHeight: 4 }
                    Text { text: "SHAPE"; font.pixelSize: 12; font.letterSpacing: 1; color: "#6b6b70" }

                    ComboBox {
                        id: shapeCombo
                        model: ["Cross", "Dot", "Circle"]
                        currentIndex: designerRoot.selectedLayer ? designerRoot.selectedLayer.shape : 0
                        Layout.preferredWidth: 160
                        implicitHeight: 34
                        onActivated: backend.setLayerShape(backend.selectedLayerIndex, currentIndex)
                    }

                    GridLayout {
                        columns: 4
                        columnSpacing: 12
                        rowSpacing: 8

                        Text { text: "Width"; font.pixelSize: 14; color: "#a0a0a6"; Layout.preferredWidth: 60 }
                        Slider {
                            from: 1; to: 300; stepSize: 1
                            value: designerRoot.selectedLayer ? designerRoot.selectedLayer.width : 20
                            onMoved: backend.setLayerWidth(backend.selectedLayerIndex, value)
                            Layout.preferredWidth: 180
                            Material.accent: "#618FF0"
                        }
                        TextField { 
                            text: Math.round(designerRoot.selectedLayer ? designerRoot.selectedLayer.width : 0).toString()
                            font.pixelSize: 12; color: "#f0f0f2"
                            Layout.preferredWidth: 70
                            implicitHeight: 34
                            padding: 8
                            verticalAlignment: TextInput.AlignVCenter
                            horizontalAlignment: TextInput.AlignRight
                            onEditingFinished: {
                                let val = parseInt(text);
                                if (!isNaN(val)) backend.setLayerWidth(backend.selectedLayerIndex, Math.max(1, Math.min(300, val)));
                            }
                        }
                        Button {
                            text: "Reset"
                            Layout.preferredWidth: 80; Layout.preferredHeight: 34
                            implicitWidth: 80; implicitHeight: 34
                            padding: 0
                            background: Rectangle { color: parent.hovered ? "#3A3A3E" : "#2a2a2e"; radius: 6 }
                            onClicked: backend.setLayerWidth(backend.selectedLayerIndex, 20)
                            contentItem: Text { text: "Reset"; color: "#a0a0a6"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        }

                        Text { text: "Height"; font.pixelSize: 14; color: "#a0a0a6"; Layout.preferredWidth: 60 }
                        Slider {
                            from: 1; to: 300; stepSize: 1
                            value: designerRoot.selectedLayer ? designerRoot.selectedLayer.height : 20
                            onMoved: backend.setLayerHeight(backend.selectedLayerIndex, value)
                            Layout.preferredWidth: 180
                            Material.accent: "#618FF0"
                        }
                        TextField { 
                            text: Math.round(designerRoot.selectedLayer ? designerRoot.selectedLayer.height : 0).toString()
                            font.pixelSize: 12; color: "#f0f0f2"
                            Layout.preferredWidth: 70
                            implicitHeight: 34
                            padding: 8
                            verticalAlignment: TextInput.AlignVCenter
                            horizontalAlignment: TextInput.AlignRight
                            onEditingFinished: {
                                let val = parseInt(text);
                                if (!isNaN(val)) backend.setLayerHeight(backend.selectedLayerIndex, Math.max(1, Math.min(300, val)));
                            }
                        }
                        Button {
                            text: "Reset"
                            Layout.preferredWidth: 80; Layout.preferredHeight: 34
                            implicitWidth: 80; implicitHeight: 34
                            padding: 0
                            background: Rectangle { color: parent.hovered ? "#3A3A3E" : "#2a2a2e"; radius: 6 }
                            onClicked: backend.setLayerHeight(backend.selectedLayerIndex, 20)
                            contentItem: Text { text: "Reset"; color: "#a0a0a6"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        }

                        Text { text: "Thickness"; font.pixelSize: 14; color: "#a0a0a6"; Layout.preferredWidth: 60 }
                        Slider {
                            from: 0.1; to: 40; stepSize: 0.1
                            value: designerRoot.selectedLayer ? designerRoot.selectedLayer.thickness : 2
                            onMoved: backend.setLayerThickness(backend.selectedLayerIndex, value)
                            Layout.preferredWidth: 180
                            Material.accent: "#618FF0"
                        }
                        TextField { 
                            text: (designerRoot.selectedLayer ? designerRoot.selectedLayer.thickness : 0).toFixed(1)
                            font.pixelSize: 12; color: "#f0f0f2"
                            Layout.preferredWidth: 70
                            implicitHeight: 34
                            padding: 8
                            verticalAlignment: TextInput.AlignVCenter
                            horizontalAlignment: TextInput.AlignRight
                            onEditingFinished: {
                                let val = parseFloat(text);
                                if (!isNaN(val)) backend.setLayerThickness(backend.selectedLayerIndex, Math.max(0.1, Math.min(40, val)));
                            }
                        }
                        Button {
                            text: "Reset"
                            Layout.preferredWidth: 80; Layout.preferredHeight: 34
                            implicitWidth: 80; implicitHeight: 34
                            padding: 0
                            background: Rectangle { color: parent.hovered ? "#3A3A3E" : "#2a2a2e"; radius: 6 }
                            onClicked: backend.setLayerThickness(backend.selectedLayerIndex, 2)
                            contentItem: Text { text: "Reset"; color: "#a0a0a6"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        }

                        // Gap — only for Cross shape
                        Text {
                            text: "Gap"
                            font.pixelSize: 14; color: "#a0a0a6"
                            Layout.preferredWidth: 60
                            visible: shapeCombo.currentIndex === 0
                        }
                        Slider {
                            from: 0; to: 200; stepSize: 1
                            value: designerRoot.selectedLayer ? designerRoot.selectedLayer.gap : 0
                            onMoved: backend.setLayerGap(backend.selectedLayerIndex, value)
                            Layout.preferredWidth: 180
                            Material.accent: "#618FF0"
                            visible: shapeCombo.currentIndex === 0
                        }
                        TextField {
                            text: Math.round(designerRoot.selectedLayer ? designerRoot.selectedLayer.gap : 0).toString()
                            font.pixelSize: 12; color: "#f0f0f2"
                            Layout.preferredWidth: 70
                            implicitHeight: 34
                            padding: 8
                            verticalAlignment: TextInput.AlignVCenter
                            horizontalAlignment: TextInput.AlignRight
                            visible: shapeCombo.currentIndex === 0
                            onEditingFinished: {
                                let val = parseInt(text);
                                if (!isNaN(val)) backend.setLayerGap(backend.selectedLayerIndex, Math.max(0, Math.min(200, val)));
                            }
                        }
                        Button {
                            text: "Reset"
                            Layout.preferredWidth: 80; Layout.preferredHeight: 34
                            implicitWidth: 80; implicitHeight: 34
                            padding: 0
                            background: Rectangle { color: parent.hovered ? "#3A3A3E" : "#2a2a2e"; radius: 6 }
                            onClicked: backend.setLayerGap(backend.selectedLayerIndex, 0)
                            visible: shapeCombo.currentIndex === 0
                            contentItem: Text { text: "Reset"; color: "#a0a0a6"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        }
                        
                        Text { text: "Rotation"; font.pixelSize: 14; color: "#a0a0a6"; Layout.preferredWidth: 60 }
                        Slider {
                            from: 0; to: 360; stepSize: 1
                            value: designerRoot.selectedLayer && designerRoot.selectedLayer.rotation !== undefined ? designerRoot.selectedLayer.rotation : 0
                            onMoved: backend.setLayerRotation(backend.selectedLayerIndex, value)
                            Layout.preferredWidth: 180
                            Material.accent: "#618FF0"
                        }
                        TextField { 
                            text: Math.round(designerRoot.selectedLayer && designerRoot.selectedLayer.rotation !== undefined ? designerRoot.selectedLayer.rotation : 0).toString()
                            font.pixelSize: 12; color: "#f0f0f2"
                            Layout.preferredWidth: 70
                            implicitHeight: 34
                            padding: 8
                            verticalAlignment: TextInput.AlignVCenter
                            horizontalAlignment: TextInput.AlignRight
                            onEditingFinished: {
                                let val = parseInt(text);
                                if (!isNaN(val)) backend.setLayerRotation(backend.selectedLayerIndex, Math.max(0, Math.min(360, val)));
                            }
                        }
                        Button {
                            text: "Reset"
                            Layout.preferredWidth: 80; Layout.preferredHeight: 34
                            implicitWidth: 80; implicitHeight: 34
                            padding: 0
                            background: Rectangle { color: parent.hovered ? "#3A3A3E" : "#2a2a2e"; radius: 6 }
                            onClicked: backend.setLayerRotation(backend.selectedLayerIndex, 0)
                            contentItem: Text { text: "Reset"; color: "#a0a0a6"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        }
                    }


                    Item { Layout.preferredHeight: 4 }
                    Text { text: "POSITION %"; font.pixelSize: 12; font.letterSpacing: 1; color: "#6b6b70" }

                    GridLayout {
                        columns: 4
                        columnSpacing: 12
                        rowSpacing: 8

                        Text { text: "Horizontal %"; font.pixelSize: 14; color: "#a0a0a6"; Layout.preferredWidth: 60 }
                        Slider {
                            from: 0; to: 1; stepSize: 0.0001
                            value: designerRoot.selectedLayer ? designerRoot.selectedLayer.posX : 0.5
                            onMoved: backend.setLayerPosX(backend.selectedLayerIndex, value)
                            Layout.preferredWidth: 180
                            Material.accent: "#618FF0"
                        }
                        TextField { 
                            text: (designerRoot.selectedLayer ? designerRoot.selectedLayer.posX : 0.5).toFixed(4)
                            font.pixelSize: 12; color: "#f0f0f2"
                            Layout.preferredWidth: 70
                            implicitHeight: 34
                            padding: 8
                            verticalAlignment: TextInput.AlignVCenter
                            horizontalAlignment: TextInput.AlignRight
                            onEditingFinished: {
                                let val = parseFloat(text);
                                if (!isNaN(val)) backend.setLayerPosX(backend.selectedLayerIndex, Math.max(0, Math.min(1, val)));
                            }
                        }
                        Button {
                            text: "Reset"
                            Layout.preferredWidth: 80; Layout.preferredHeight: 34
                            implicitWidth: 80; implicitHeight: 34
                            padding: 0
                            background: Rectangle { color: parent.hovered ? "#3A3A3E" : "#2a2a2e"; radius: 6 }
                            onClicked: backend.setLayerPosX(backend.selectedLayerIndex, 0.5)
                            contentItem: Text { text: "Reset"; color: "#a0a0a6"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        }

                        Text { text: "Vertical %"; font.pixelSize: 14; color: "#a0a0a6"; Layout.preferredWidth: 60 }
                        Slider {
                            from: 0; to: 1; stepSize: 0.0001
                            value: designerRoot.selectedLayer ? designerRoot.selectedLayer.posY : 0.5
                            onMoved: backend.setLayerPosY(backend.selectedLayerIndex, value)
                            Layout.preferredWidth: 180
                            Material.accent: "#618FF0"
                        }
                        TextField { 
                            text: (designerRoot.selectedLayer ? designerRoot.selectedLayer.posY : 0.5).toFixed(4)
                            font.pixelSize: 12; color: "#f0f0f2"
                            Layout.preferredWidth: 70
                            implicitHeight: 34
                            padding: 8
                            verticalAlignment: TextInput.AlignVCenter
                            horizontalAlignment: TextInput.AlignRight
                            onEditingFinished: {
                                let val = parseFloat(text);
                                if (!isNaN(val)) backend.setLayerPosY(backend.selectedLayerIndex, Math.max(0, Math.min(1, val)));
                            }
                        }
                        Button {
                            text: "Reset"
                            Layout.preferredWidth: 80; Layout.preferredHeight: 34
                            implicitWidth: 80; implicitHeight: 34
                            padding: 0
                            background: Rectangle { color: parent.hovered ? "#3A3A3E" : "#2a2a2e"; radius: 6 }
                            onClicked: backend.setLayerPosY(backend.selectedLayerIndex, 0.5)
                            contentItem: Text { text: "Reset"; color: "#a0a0a6"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        }
                    }



                    Item { Layout.preferredHeight: 4 }
                    Text { text: "COLOR"; font.pixelSize: 12; font.letterSpacing: 1; color: "#6b6b70" }

                    RowLayout {
                        spacing: 12

                        // Color preview swatch
                        Rectangle {
                            width: 48; height: 48; radius: 8
                            color: designerRoot.selectedLayer ? designerRoot.selectedLayer.color : "#ffffffff"
                            border.width: 1
                            border.color: "#3a3a3e"

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: colorDialog.open()
                            }
                        }

                        Text {
                            text: designerRoot.selectedLayer ? designerRoot.selectedLayer.color : "#ffffffff"
                            font.pixelSize: 14
                            color: "#a0a0a6"
                        }

                        Text {
                            text: "Click swatch to change"
                            font.pixelSize: 12
                            color: "#6b6b70"
                        }
                    }

                    Item { Layout.preferredHeight: 8 }
                    Text { text: "OUTLINE"; font.pixelSize: 12; font.letterSpacing: 1; color: "#6b6b70" }

                    RowLayout {
                        spacing: 12
                        Switch {
                            text: "Enable Outline"
                            checked: designerRoot.selectedLayer ? designerRoot.selectedLayer.outlineEnabled : false
                            onToggled: backend.setLayerOutlineEnabled(backend.selectedLayerIndex, checked)
                            Material.accent: "#618FF0"
                        }

                        Rectangle {
                            width: 32; height: 32; radius: 6
                            color: designerRoot.selectedLayer ? designerRoot.selectedLayer.outlineColor : "#000000"
                            border.width: 1
                            border.color: "#3a3a3e"
                            visible: designerRoot.selectedLayer ? designerRoot.selectedLayer.outlineEnabled : false

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: outlineColorDialog.open()
                            }
                        }
                    }

                    // Outline Thickness — only visible if enabled
                    RowLayout {
                        spacing: 12
                        visible: designerRoot.selectedLayer ? designerRoot.selectedLayer.outlineEnabled : false
                        Text { text: "Thickness"; font.pixelSize: 14; color: "#a0a0a6"; Layout.preferredWidth: 60 }
                        Slider {
                            from: 0.1; to: 10; stepSize: 0.1
                            value: designerRoot.selectedLayer && designerRoot.selectedLayer.outlineThickness !== undefined ? designerRoot.selectedLayer.outlineThickness : 1.0
                            onMoved: backend.setLayerOutlineThickness(backend.selectedLayerIndex, value)
                            Layout.preferredWidth: 180
                            Material.accent: "#618FF0"
                        }
                        TextField { 
                            text: (designerRoot.selectedLayer && designerRoot.selectedLayer.outlineThickness !== undefined ? designerRoot.selectedLayer.outlineThickness : 1.0).toFixed(1)
                            font.pixelSize: 12; color: "#f0f0f2"
                            Layout.preferredWidth: 70
                            implicitHeight: 34
                            padding: 8
                            verticalAlignment: TextInput.AlignVCenter
                            horizontalAlignment: TextInput.AlignRight
                            onEditingFinished: {
                                let val = parseFloat(text);
                                if (!isNaN(val)) backend.setLayerOutlineThickness(backend.selectedLayerIndex, Math.max(0.1, Math.min(10, val)));
                            }
                        }
                        Button {
                            text: "Reset"
                            Layout.preferredWidth: 80; Layout.preferredHeight: 34
                            implicitWidth: 80; implicitHeight: 34
                            padding: 0
                            background: Rectangle { color: parent.hovered ? "#3A3A3E" : "#2a2a2e"; radius: 6 }
                            onClicked: backend.setLayerOutlineThickness(backend.selectedLayerIndex, 1.0)
                            contentItem: Text { text: "Reset"; color: "#a0a0a6"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        }
                    }

                }

                // No layer selected
                Text {
                    visible: designerRoot.selectedLayer === null
                    text: "Add or select a layer to edit."
                    font.pixelSize: 14
                    color: "#6b6b70"
                }
            }
        }
    }

    // ─── Dialogs (Outside Layout) ─────────────────────────
    Dialog {
        id: savePresetDialog
        title: "Save Crosshair Preset"
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2
        modal: true
        padding: 20

        ColumnLayout {
            spacing: 16
            width: 300
            Text {
                text: "Enter preset name:"
                color: "#a0a0a6"
                font.pixelSize: 14
            }
            TextField {
                id: savePresetNameField
                placeholderText: "Preset name..."
                Layout.fillWidth: true
                onAccepted: savePresetDialog.accept()
            }
        }

        footer: RowLayout {
            spacing: 12
            Layout.margins: 16
            Item { Layout.fillWidth: true }
            Button {
                text: "Cancel"
                onClicked: savePresetDialog.reject()
                implicitWidth: 100
                background: Rectangle { color: "#2a2a2e"; radius: 6; border.color: "#3a3a3e" }
                contentItem: Text { text: "Cancel"; color: "#f0f0f2"; horizontalAlignment: Text.AlignHCenter }
            }
            Button {
                text: "Save"
                onClicked: savePresetDialog.accept()
                implicitWidth: 100
                background: Rectangle { color: "#618FF0"; radius: 6 }
                contentItem: Text { text: "Save"; color: "#ffffff"; font.bold: true; horizontalAlignment: Text.AlignHCenter }
            }
        }

        onOpened: {
            savePresetNameField.text = ""
            savePresetNameField.forceActiveFocus()
        }

        onAccepted: {
            let name = savePresetNameField.text.trim() === "" ? "Custom Preset" : savePresetNameField.text;
            backend.savePreset(name);
        }
    }

    ColorDialog {
        id: colorDialog
        title: "Layer Color"
        selectedColor: designerRoot.selectedLayer ? designerRoot.selectedLayer.color : "#ffffffff"
        options: ColorDialog.ShowAlphaChannel
        onAccepted: backend.setLayerColor(backend.selectedLayerIndex, selectedColor.toString())
    }

    ColorDialog {
        id: outlineColorDialog
        title: "Outline Color"
        selectedColor: designerRoot.selectedLayer ? designerRoot.selectedLayer.outlineColor : "#000000"
        options: ColorDialog.ShowAlphaChannel
        onAccepted: backend.setLayerOutlineColor(backend.selectedLayerIndex, selectedColor.toString())
    }
}
