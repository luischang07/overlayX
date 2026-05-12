import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

Rectangle {
    id: sidebar
    color: "#161618"

    property int currentTab: 0
    property bool engineConnected: false
    property bool editMode: false

    signal tabSelected(int tab)

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ─── Header ────────────────────────────────────────
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 80
            Layout.leftMargin: 24
            Layout.topMargin: 8

            Column {
                anchors.verticalCenter: parent.verticalCenter
                spacing: 2

                Text {
                    text: "OverlayX"
                    font.pixelSize: 22
                    font.bold: true
                    color: "#618FF0"
                }
                Text {
                    text: "v2.0"
                    font.pixelSize: 12
                    color: "#6b6b70"
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            color: "#2a2a2e"
        }

        Item { Layout.preferredHeight: 8 }

        // ─── Navigation Items ──────────────────────────────
        Repeater {
            model: [
                { label: "Overview",           tab: 0 },
                { label: "Settings",           tab: 1 },
                { label: "Position & Presets", tab: 2 },
                { label: "Hotkeys",            tab: 3 },
                { label: "Countdown Timer",    tab: 4 },
                { label: "Crosshair Designer", tab: 5 }
            ]

            delegate: Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 44
                Layout.leftMargin: 8
                Layout.rightMargin: 8

                property bool isActive: sidebar.currentTab === modelData.tab

                // Active indicator bar
                Rectangle {
                    visible: isActive
                    width: 3
                    height: 24
                    radius: 2
                    color: "#618FF0"
                    anchors.left: parent.left
                    anchors.leftMargin: 4
                    anchors.verticalCenter: parent.verticalCenter
                }

                // Background highlight
                Rectangle {
                    anchors.fill: parent
                    anchors.leftMargin: 4
                    radius: 6
                    color: isActive ? "#67696c" : hoverArea.containsMouse ? "#3A3A3E" : "transparent"

                    Behavior on color { ColorAnimation { duration: 150 } }
                }

                Text {
                    text: modelData.label
                    anchors.left: parent.left
                    anchors.leftMargin: 20
                    anchors.verticalCenter: parent.verticalCenter
                    font.pixelSize: 14
                    color: isActive ? "#f0f0f2" : "#a0a0a6"
                }

                MouseArea {
                    id: hoverArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: sidebar.tabSelected(modelData.tab)
                }
            }
        }

        // ─── Spacer ────────────────────────────────────────
        Item { Layout.fillHeight: true }

        // ─── Status ────────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            color: "#2a2a2e"
        }

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            Layout.leftMargin: 24

            Column {
                anchors.verticalCenter: parent.verticalCenter
                spacing: 4

                Text {
                    text: sidebar.engineConnected ? "Engine Connected" : "Engine Offline"
                    font.pixelSize: 13
                    color: sidebar.engineConnected ? "#59D499" : "#D95959"
                }
                Text {
                    text: sidebar.editMode ? "Drag Mode Active" : "Ready"
                    font.pixelSize: 12
                    color: "#6b6b70"
                }
            }
        }
    }
}
