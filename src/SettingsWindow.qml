import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import "settings"

// The configuration window: an ordinary window, unlike the dock.
ApplicationWindow {
    id: win
    required property var app
    property string page: "appearance"
    property bool helpVisible: false

    width: 980
    height: 720
    minimumWidth: 760
    minimumHeight: 520
    title: qsTr("kai Settings")
    color: "#0e0e10"
    Material.theme: Material.Dark
    Material.accent: app.accent
    Material.roundedScale: app.radius

    readonly property var pages: [
        { id: "appearance", title: qsTr("Appearance"), glyph: "◧" },
        { id: "themes", title: qsTr("Themes"), glyph: "◐" },
        { id: "applets", title: qsTr("Applets"), glyph: "▦" },
        { id: "launchers", title: qsTr("Launchers"), glyph: "★" },
        { id: "behavior", title: qsTr("Behavior"), glyph: "⟳" },
        { id: "system", title: qsTr("System"), glyph: "⚙" }
    ]
    readonly property int pageIndex: Math.max(0, pages.findIndex(p => p.id === page))

    Shortcut { sequence: "?"; context: Qt.WindowShortcut; onActivated: win.helpVisible = !win.helpVisible }
    Shortcut { sequences: ["Esc", "Q", StandardKey.Close]; context: Qt.WindowShortcut; onActivated: { if (win.helpVisible) win.helpVisible = false; else win.close() } }
    Shortcut { sequence: "Ctrl+Q"; context: Qt.WindowShortcut; onActivated: Qt.quit() }
    Shortcut { sequence: "Ctrl+Tab"; context: Qt.WindowShortcut; onActivated: win.page = win.pages[(win.pageIndex + 1) % win.pages.length].id }
    Shortcut { sequence: "Ctrl+Shift+Tab"; context: Qt.WindowShortcut; onActivated: win.page = win.pages[(win.pageIndex + win.pages.length - 1) % win.pages.length].id }
    Repeater {
        model: win.pages.length
        Item {
            required property int index
            Shortcut { sequence: "Ctrl+" + (index + 1); context: Qt.WindowShortcut; onActivated: win.page = win.pages[index].id }
        }
    }

    Item {
        anchors.fill: parent
        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        LayoutMirroring.childrenInherit: true

        Rectangle {
            id: sidebar
            width: 210
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            color: "#141417"

            Column {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 2
                Item {
                    width: parent.width
                    height: 58
                    Row {
                        anchors.verticalCenter: parent.verticalCenter
                        x: 6
                        spacing: 10
                        Rectangle {
                            width: 30; height: 30
                            radius: Math.min(8, win.app.radius + 4)
                            color: win.app.accent
                            Text { anchors.centerIn: parent; text: "k"; color: omarchy.accentForeground; font.pixelSize: 18; font.weight: Font.Bold }
                        }
                        Column {
                            Label { text: "kai"; font.pixelSize: 17; font.weight: Font.DemiBold }
                            Label { text: qsTr("Dock settings"); font.pixelSize: 11; opacity: 0.55 }
                        }
                    }
                }
                Repeater {
                    model: win.pages
                    delegate: ItemDelegate {
                        required property var modelData
                        required property int index
                        width: parent.width
                        height: 40
                        highlighted: win.page === modelData.id
                        text: modelData.glyph + "   " + modelData.title
                        onClicked: win.page = modelData.id
                        background: Rectangle {
                            radius: Math.min(6, win.app.radius + 2)
                            color: parent.highlighted ? Qt.rgba(1, 1, 1, 0.08) : parent.hovered ? Qt.rgba(1, 1, 1, 0.04) : "transparent"
                            Rectangle {
                                visible: parent.parent.highlighted
                                width: 3; height: parent.height - 16
                                anchors.verticalCenter: parent.verticalCenter
                                radius: 1.5
                                color: win.app.accent
                            }
                        }
                    }
                }
            }
            Label {
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.margins: 18
                text: qsTr("? for shortcuts")
                opacity: 0.4
                font.pixelSize: 11
            }
        }

        StackLayout {
            anchors.left: sidebar.right
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            currentIndex: win.pageIndex
            AppearancePage { app: win.app }
            ThemesPage { app: win.app }
            AppletsPage { app: win.app }
            LaunchersPage { app: win.app }
            BehaviorPage { app: win.app }
            SystemPage { app: win.app }
        }

        // Every shortcut, on "?".
        Rectangle {
            anchors.fill: parent
            visible: win.helpVisible
            color: Qt.rgba(0, 0, 0, 0.7)
            MouseArea { anchors.fill: parent; onClicked: win.helpVisible = false }
            Rectangle {
                anchors.centerIn: parent
                width: 440
                height: helpColumn.implicitHeight + 40
                radius: Math.min(10, win.app.radius + 4)
                color: "#18181c"
                border.color: Qt.rgba(1, 1, 1, 0.1)
                Column {
                    id: helpColumn
                    anchors.fill: parent
                    anchors.margins: 20
                    spacing: 8
                    Label { text: qsTr("Keyboard shortcuts"); font.pixelSize: 18; font.weight: Font.DemiBold }
                    Repeater {
                        model: [
                            ["Ctrl+1 … 6", qsTr("Go to a page")],
                            ["Ctrl+Tab", qsTr("Next page")],
                            ["Esc, Q", qsTr("Close settings")],
                            ["Ctrl+Q", qsTr("Quit the dock")],
                            ["?", qsTr("Show or hide this help")],
                            [qsTr("Click"), qsTr("Open, focus, or minimize an app")],
                            [qsTr("Middle-click"), qsTr("Open a new window")],
                            [qsTr("Scroll"), qsTr("Cycle an app's windows")],
                            [qsTr("Right-click"), qsTr("Windows, pinning, applet options")],
                            [qsTr("Drag"), qsTr("Reorder pinned apps; drag off to unpin")]
                        ]
                        RowLayout {
                            required property var modelData
                            width: parent.width
                            Label { Layout.preferredWidth: 150; text: modelData[0]; font.family: "monospace"; color: win.app.accent }
                            Label { Layout.fillWidth: true; text: modelData[1]; wrapMode: Text.Wrap }
                        }
                    }
                }
            }
        }
    }
}
