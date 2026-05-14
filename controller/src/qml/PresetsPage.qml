import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

ScrollView {
    clip: true

    ColumnLayout {
        spacing: 16
        width: parent ? parent.width : 600

        Text {
            text: "Position & Presets"
            font.pixelSize: 24
            font.bold: true
            color: "#f0f0f2"
        }

        Item { Layout.preferredHeight: 8 }

        // ─── Fine Tuning ───────────────────────────────────
        Text { text: "FINE TUNING"; font.pixelSize: 12; font.letterSpacing: 1; color: "#6b6b70" }

        GridLayout {
            columns: 4
            columnSpacing: 16
            rowSpacing: 12
            Layout.fillWidth: true

            property var activeLayer: backend ? ((backend.selectedLayerIndex >= 0 && backend.selectedLayerIndex < backend.layers.length)
                                      ? backend.layers[backend.selectedLayerIndex] : null) : null

            Text { text: "Horizontal %"; font.pixelSize: 14; color: "#a0a0a6"; Layout.preferredWidth: 100 }
            Slider {
                from: 0; to: 1; stepSize: 0.0001
                value: parent.activeLayer ? parent.activeLayer.posX : 0.5
                onMoved: backend.setLayerPosX(backend.selectedLayerIndex, value)
                Layout.fillWidth: true
                Material.accent: "#618FF0"
            }
            TextField {
                text: (parent.activeLayer ? parent.activeLayer.posX : 0.5).toFixed(4)
                Layout.preferredWidth: 100
                implicitHeight: 34
                padding: 10
                verticalAlignment: TextInput.AlignVCenter
                horizontalAlignment: TextInput.AlignRight
                onEditingFinished: {
                    let val = parseFloat(text);
                    if (!isNaN(val)) backend.setLayerPosX(backend.selectedLayerIndex, Math.max(0, Math.min(1, val)));
                }
            }
            Button {
                text: "Reset"
                Layout.preferredWidth: 160; Layout.preferredHeight: 34
                implicitWidth: 160; implicitHeight: 34
                padding: 0
                background: Rectangle { color: parent.hovered ? "#3A3A3E" : "#2a2a2e"; radius: 6 }
                onClicked: backend.setLayerPosX(backend.selectedLayerIndex, 0.5)
                contentItem: Text { text: "Reset"; color: "#a0a0a6"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }

            Text { text: "Vertical %"; font.pixelSize: 14; color: "#a0a0a6"; Layout.preferredWidth: 100 }
            Slider {
                from: 0; to: 1; stepSize: 0.0001
                value: parent.activeLayer ? parent.activeLayer.posY : 0.5
                onMoved: backend.setLayerPosY(backend.selectedLayerIndex, value)
                Layout.fillWidth: true
                Material.accent: "#618FF0"
            }
            TextField {
                text: (parent.activeLayer ? parent.activeLayer.posY : 0.5).toFixed(4)
                Layout.preferredWidth: 100
                implicitHeight: 34
                padding: 10
                verticalAlignment: TextInput.AlignVCenter
                horizontalAlignment: TextInput.AlignRight
                onEditingFinished: {
                    let val = parseFloat(text);
                    if (!isNaN(val)) backend.setLayerPosY(backend.selectedLayerIndex, Math.max(0, Math.min(1, val)));
                }
            }
            Button {
                text: "Reset"
                Layout.preferredWidth: 160; Layout.preferredHeight: 34
                implicitWidth: 160; implicitHeight: 34
                padding: 0
                background: Rectangle { color: parent.hovered ? "#3A3A3E" : "#2a2a2e"; radius: 6 }
                onClicked: backend.setLayerPosY(backend.selectedLayerIndex, 0.5)
                contentItem: Text { text: "Reset"; color: "#a0a0a6"; font.pixelSize: 12; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
            }
        }

        Button {
            text: "Center Crosshair"
            Layout.preferredWidth: 200
            implicitWidth: 200
            implicitHeight: 34
            padding: 0
            background: Rectangle { color: "#618FF0"; radius: 6 }
            contentItem: Text {
                text: parent.text
                color: "#ffffff"
                font.pixelSize: 12
                font.bold: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            onClicked: backend.centerCrosshair()
        }

        Item { Layout.preferredHeight: 16 }

        // ─── Active Presets ────────────────────────────────
        Text { text: "ACTIVE PRESETS (Independent Movement)"; font.pixelSize: 12; font.letterSpacing: 1; color: "#6b6b70" }

        ListView {
            id: activeInstancesList
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(contentHeight, 300)
            clip: true
            model: backend ? backend.activePresetInstances : []

            delegate: Item {
                id: instanceDelegate
                width: activeInstancesList.width
                height: 100

                // Hover highlight — plain Rectangle so mouse events pass through to child controls
                Rectangle {
                    anchors.fill: parent
                    radius: 6
                    color: hoverArea.containsMouse ? "#0AFFFFFF" : "transparent"
                    MouseArea {
                        id: hoverArea
                        anchors.fill: parent
                        hoverEnabled: true
                        // Don't accept press so child Sliders and Buttons get their events
                        onPressed: mouse.accepted = false
                    }
                }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 8

                    RowLayout {
                        spacing: 12
                        Layout.fillWidth: true

                        Text {
                            text: modelData.name
                            font.pixelSize: 14
                            font.bold: true
                            color: "#f0f0f2"
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                            Layout.alignment: Qt.AlignVCenter
                        }

                        Button {
                            text: "Remove Instance"
                            Layout.preferredWidth: 180; Layout.preferredHeight: 34
                            implicitWidth: 180; implicitHeight: 34
                            padding: 0
                            onClicked: backend.removeInstance(modelData.presetId)
                            background: Rectangle { color: parent.hovered ? "#3A3A3E" : "#2a2a2e"; radius: 6 }
                            contentItem: Text { 
                                text: "Remove Instance"; 
                                color: "#f44336"; font.pixelSize: 11; font.bold: true; 
                                horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter 
                            }
                        }
                    }

                    GridLayout {
                        columns: 6
                        columnSpacing: 16
                        Layout.fillWidth: true
                        
                        Text { text: "X:"; font.pixelSize: 11; color: "#6b6b70"; Layout.preferredWidth: 20 }
                        Slider {
                            id: xSlider
                            from: 0; to: 1; stepSize: 0.001
                            value: modelData.posX
                            // During drag: push to engine only (no configChanged → no delegate recreation)
                            onMoved: backend.setInstancePosXLive(modelData.presetId, value)
                            // On drag release: do a full commit so model + text field refresh
                            onPressedChanged: {
                                if (!pressed) backend.setInstancePosX(modelData.presetId, value)
                            }
                            Layout.fillWidth: true
                            Material.accent: "#618FF0"
                        }
                        TextField {
                            text: modelData.posX.toFixed(4)
                            Layout.preferredWidth: 120
                            implicitHeight: 30
                            verticalAlignment: TextInput.AlignVCenter
                            horizontalAlignment: TextInput.AlignRight
                            onEditingFinished: {
                                let val = parseFloat(text);
                                if (!isNaN(val)) backend.setInstancePosX(modelData.presetId, val);
                            }
                        }

                        Text { text: "Y:"; font.pixelSize: 11; color: "#6b6b70"; Layout.preferredWidth: 20 }
                        Slider {
                            id: ySlider
                            from: 0; to: 1; stepSize: 0.001
                            value: modelData.posY
                            // During drag: push to engine only (no configChanged → no delegate recreation)
                            onMoved: backend.setInstancePosYLive(modelData.presetId, value)
                            // On drag release: do a full commit so model + text field refresh
                            onPressedChanged: {
                                if (!pressed) backend.setInstancePosY(modelData.presetId, value)
                            }
                            Layout.fillWidth: true
                            Material.accent: "#618FF0"
                        }
                        TextField {
                            text: modelData.posY.toFixed(4)
                            Layout.preferredWidth: 120
                            implicitHeight: 30
                            verticalAlignment: TextInput.AlignVCenter
                            horizontalAlignment: TextInput.AlignRight
                            onEditingFinished: {
                                let val = parseFloat(text);
                                if (!isNaN(val)) backend.setInstancePosY(modelData.presetId, val);
                            }
                        }
                    }
                }
            }
        }

        Item { Layout.preferredHeight: 16 }

        // ─── Saved Positions ───────────────────────────────
        Text { text: "SAVED POSITIONS"; font.pixelSize: 12; font.letterSpacing: 1; color: "#6b6b70" }

        RowLayout {
            spacing: 8
            Layout.fillWidth: true
            TextField {
                id: posNameField
                placeholderText: "Position name..."
                Layout.fillWidth: true
                implicitWidth: 400
                implicitHeight: 34
                padding: 10
                verticalAlignment: TextInput.AlignVCenter
            }
            Button {
                text: "Save Position"
                implicitHeight: 34
                implicitWidth: 200
                Layout.preferredWidth: 200
                Layout.preferredHeight: 34
                padding: 0
                background: Rectangle { color: "#618FF0"; radius: 6 }
                contentItem: Text {
                    text: "Save Position"
                    color: "#ffffff"
                    font.pixelSize: 12
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: {
                    backend.savePosition(posNameField.text, backend.selectedLayerIndex)
                    posNameField.text = ""
                }
            }
        }

        // Header for Positions
        GridLayout {
            columns: 3
            columnSpacing: 12
            Layout.fillWidth: true
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            
            Text { 
                text: "Name"; 
                font.pixelSize: 11; font.bold: true; color: "#6b6b70"; 
                Layout.fillWidth: true 
            }
            Text { 
                text: "Load"; 
                font.pixelSize: 11; font.bold: true; color: "#6b6b70"; 
                Layout.preferredWidth: 100; horizontalAlignment: Text.AlignHCenter 
            }
            Item { Layout.preferredWidth: 32 } // Spacer for Delete button
        }

        ListView {
            id: positionsList
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(contentHeight, 160)
            clip: true
            model: backend ? backend.savedPositions : []

            delegate: ItemDelegate {
                id: posDelegate
                width: positionsList.width
                height: 48
                padding: 0
                
                background: Rectangle {
                    color: posDelegate.down ? "#1AFFFFFF" : (posDelegate.hovered ? "#0AFFFFFF" : "transparent")
                    radius: 6
                }

                contentItem: GridLayout {
                    columns: 3
                    columnSpacing: 12
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12

                    Text {
                        text: modelData.name
                        font.pixelSize: 14
                        color: "#f0f0f2"
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                        Layout.alignment: Qt.AlignVCenter
                        
                        MouseArea {
                            anchors.fill: parent
                            onDoubleClicked: {
                                renameDialog.targetIndex = index
                                renameDialog.targetType = "position"
                                renameDialog.currentName = modelData.name
                                renameDialog.open()
                            }
                        }
                    }
                    Button {
                        text: "Load"
                        Layout.preferredWidth: 100; Layout.preferredHeight: 32
                        Layout.alignment: Qt.AlignVCenter
                        onClicked: backend.loadPosition(index, backend.selectedLayerIndex)
                        background: Rectangle { color: parent.hovered ? "#3A3A3E" : "#2a2a2e"; radius: 4 }
                        contentItem: Text { text: "Load"; color: "#618FF0"; font.pixelSize: 11; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    }
                    Button {
                        Layout.preferredWidth: 32; Layout.preferredHeight: 32
                        Layout.alignment: Qt.AlignVCenter
                        flat: true
                        padding: 0
                        background: Rectangle { color: parent.hovered ? "#d32f2f" : "transparent"; radius: 4 }
                        contentItem: Text {
                            text: "\u00D7"
                            color: "#f0f0f2"
                            font.pixelSize: 22
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                        onClicked: backend.deletePosition(index)
                    }
                }
            }
        }

        Item { Layout.preferredHeight: 8 }

        // ─── Crosshair Presets ──────────────────────────────
        Text { text: "CROSSHAIR PRESETS"; font.pixelSize: 12; font.letterSpacing: 1; color: "#6b6b70" }

        RowLayout {
            spacing: 8
            Layout.fillWidth: true
            TextField {
                id: presetNameField
                placeholderText: "Preset name..."
                Layout.fillWidth: true
                implicitWidth: 300
                implicitHeight: 34
                padding: 10
                verticalAlignment: TextInput.AlignVCenter
            }
            Button {
                id: mainSavePresetBtn
                text: "Save Preset"
                implicitHeight: 34
                implicitWidth: 160
                Layout.preferredWidth: 160
                Layout.preferredHeight: 34
                padding: 0
                background: Rectangle { color: "#618FF0"; radius: 6 }
                contentItem: Text {
                    text: "Save Preset"
                    color: "#ffffff"
                    font.pixelSize: 12
                    font.bold: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                onClicked: {
                    backend.savePreset(presetNameField.text)
                    presetNameField.text = ""
                }
            }
        }

        // Header for Presets
        GridLayout {
            columns: 5
            columnSpacing: 12
            Layout.fillWidth: true
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            
            Text { 
                text: "Name / Info"; 
                font.pixelSize: 11; font.bold: true; color: "#6b6b70"; 
                Layout.fillWidth: true 
            }
            Text { 
                text: "Load"; 
                font.pixelSize: 11; font.bold: true; color: "#6b6b70"; 
                Layout.preferredWidth: 100; horizontalAlignment: Text.AlignHCenter 
            }
            Text { 
                text: "Add"; 
                font.pixelSize: 11; font.bold: true; color: "#6b6b70"; 
                Layout.preferredWidth: 120; horizontalAlignment: Text.AlignHCenter 
            }
            Text { 
                text: "Remove"; 
                font.pixelSize: 11; font.bold: true; color: "#6b6b70"; 
                Layout.preferredWidth: 100; horizontalAlignment: Text.AlignHCenter 
            }
            Item { Layout.preferredWidth: 32 } // Delete button column
        }

        ListView {
            id: presetsList
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(contentHeight, 300)
            clip: true
            model: backend ? backend.savedPresets : []

            delegate: ItemDelegate {
                id: presetDelegate
                width: presetsList.width
                height: 110
                padding: 0
                
                background: Rectangle {
                    color: presetDelegate.down ? "#1AFFFFFF" : (presetDelegate.hovered ? "#0AFFFFFF" : "transparent")
                    radius: 6
                }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 8

                    RowLayout {
                        spacing: 12
                        Layout.fillWidth: true

                        ColumnLayout {
                            spacing: 0
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignVCenter
                            Text {
                                text: modelData.name
                                font.pixelSize: 14
                                font.bold: true
                                color: "#f0f0f2"
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                                
                                MouseArea {
                                    anchors.fill: parent
                                    onDoubleClicked: {
                                        renameDialog.targetIndex = index
                                        renameDialog.targetType = "preset"
                                        renameDialog.currentName = modelData.name
                                        renameDialog.open()
                                    }
                                }
                            }
                            Text {
                                text: modelData.layerCount + " layers"
                                font.pixelSize: 11
                                color: "#6b6b70"
                            }
                        }

                        Button {
                            text: "Load"
                            Layout.preferredWidth: 100; Layout.preferredHeight: 32
                            Layout.alignment: Qt.AlignVCenter
                            onClicked: backend.loadPreset(index)
                            background: Rectangle { color: parent.hovered ? "#3A3A3E" : "#2a2a2e"; radius: 4 }
                            contentItem: Text { text: "Load"; color: "#618FF0"; font.pixelSize: 11; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        }

                        Button {
                            text: "Add"
                            Layout.preferredWidth: 120; Layout.preferredHeight: 32
                            Layout.alignment: Qt.AlignVCenter
                            onClicked: backend.appendPreset(index)
                            background: Rectangle { color: parent.hovered ? "#3A3A3E" : "#2a2a2e"; radius: 4 }
                            contentItem: Text { text: "Add New Instance"; color: "#4CAF50"; font.pixelSize: 11; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        }

                        Button {
                            text: "Remove All"
                            Layout.preferredWidth: 100; Layout.preferredHeight: 32
                            Layout.alignment: Qt.AlignVCenter
                            onClicked: backend.removePresetLayers(index)
                            background: Rectangle { color: parent.hovered ? "#3A3A3E" : "#2a2a2e"; radius: 4 }
                            contentItem: Text { text: "Remove All"; color: "#f44336"; font.pixelSize: 11; font.bold: true; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                        }

                        Button {
                            Layout.preferredWidth: 32; Layout.preferredHeight: 32
                            Layout.alignment: Qt.AlignVCenter
                            flat: true
                            padding: 0
                            background: Rectangle { color: parent.hovered ? "#d32f2f" : "transparent"; radius: 4 }
                            contentItem: Text {
                                text: "\u2715"
                                color: "#f0f0f2"
                                font.pixelSize: 18
                                font.bold: true
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                            onClicked: backend.deletePreset(index)
                        }
                    }

                    GridLayout {
                        columns: 4
                        columnSpacing: 16
                        Layout.fillWidth: true
                        
                        Text { text: "X:"; font.pixelSize: 11; color: "#6b6b70"; Layout.preferredWidth: 20 }
                        Slider {
                            from: 0; to: 1; stepSize: 0.001
                            value: modelData.posX
                            onMoved: backend.setPresetPosX(index, value)
                            Layout.fillWidth: true
                            Material.accent: "#618FF0"
                        }
                        
                        Text { text: "Y:"; font.pixelSize: 11; color: "#6b6b70"; Layout.preferredWidth: 20 }
                        Slider {
                            from: 0; to: 1; stepSize: 0.001
                            value: modelData.posY
                            onMoved: backend.setPresetPosY(index, value)
                            Layout.fillWidth: true
                            Material.accent: "#618FF0"
                        }
                    }
                }
            }
        }

    }

    Dialog {
        id: renameDialog
        title: "Rename"
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2
        modal: true
        padding: 20

        property int targetIndex: -1
        property string targetType: ""
        property string currentName: ""

        ColumnLayout {
            spacing: 16
            width: 300
            Text {
                text: "Enter new name:"
                color: "#a0a0a6"
                font.pixelSize: 14
            }
            TextField {
                id: renameField
                text: renameDialog.currentName
                Layout.fillWidth: true
                placeholderText: "New name..."
                onAccepted: renameDialog.accept()
            }
        }

        footer: RowLayout {
            spacing: 12
            Layout.margins: 16
            Item { Layout.fillWidth: true }
            Button {
                text: "Cancel"
                onClicked: renameDialog.reject()
                implicitWidth: 100
                background: Rectangle { color: "#2a2a2e"; radius: 6; border.color: "#3a3a3e" }
                contentItem: Text { text: "Cancel"; color: "#f0f0f2"; horizontalAlignment: Text.AlignHCenter }
            }
            Button {
                text: "OK"
                onClicked: renameDialog.accept()
                implicitWidth: 100
                background: Rectangle { color: "#618FF0"; radius: 6 }
                contentItem: Text { text: "OK"; color: "#ffffff"; font.bold: true; horizontalAlignment: Text.AlignHCenter }
            }
        }

        onOpened: {
            renameField.text = currentName
            renameField.forceActiveFocus()
        }

        onAccepted: {
            if (targetType === "position") {
                backend.renamePosition(targetIndex, renameField.text)
            } else if (targetType === "preset") {
                backend.renamePreset(targetIndex, renameField.text)
            }
        }
    }
}
