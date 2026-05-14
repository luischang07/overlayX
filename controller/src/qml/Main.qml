import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts

ApplicationWindow {
    id: root
    visible: true
    width: 1280
    height: 780
    minimumWidth: 960
    minimumHeight: 600
    title: "OverlayX Controller"

    Component.onCompleted: {
        x = (Screen.width - width) / 2
        y = (Screen.height - height) / 2
    }

    // Material Dark + Blue accent
    Material.theme: Material.Dark
    Material.accent: "#618FF0"
    Material.background: "#1c1c1e"
    Material.foreground: "#f0f0f2"

    color: "#1c1c1e"

    property int currentTab: 0

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // ─── Sidebar ───────────────────────────────────────
        Sidebar {
            Layout.fillHeight: true
            Layout.preferredWidth: 250
            currentTab: root.currentTab
            engineConnected: backend ? backend.engineConnected : false
            editMode: backend ? backend.editMode : false
            onTabSelected: function(tab) { root.currentTab = tab }
        }

        // Separator line
        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: 1
            color: "#2a2a2e"
        }

        // ─── Content Area ──────────────────────────────────
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            StackLayout {
                anchors.fill: parent
                anchors.margins: 20
                currentIndex: root.currentTab

                OverviewPage {}
                SettingsPage {}
                PresetsPage {}
                HotkeysPage {}
                CountdownPage {}
                DesignerPage {}
            }
        }
    }
}
