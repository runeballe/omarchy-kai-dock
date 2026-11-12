import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

SettingsPage {
    id: page
    required property var app
    heading: qsTr("Launchers")
    subheading: qsTr("Apps pinned to the dock, in order. You can also drag icons along the dock to reorder them, or off it to unpin.")

    readonly property var pinned: config.values.launchers

    SectionTitle { text: qsTr("Pinned") }
    Label {
        visible: page.pinned.length === 0
        text: qsTr("Nothing pinned yet. Add apps below.")
        opacity: 0.6
        topPadding: 6
    }
    Repeater {
        model: page.pinned
        delegate: Item {
            required property string modelData
            required property int index
            readonly property var entry: apps.entry(modelData)
            width: parent ? parent.width : 400
            height: 46
            Rectangle {
                anchors.fill: parent
                anchors.bottomMargin: 2
                radius: Math.min(6, page.app.radius)
                color: Qt.rgba(1, 1, 1, 0.04)
            }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 6
                spacing: 10
                Image {
                    Layout.preferredWidth: 28
                    Layout.preferredHeight: 28
                    sourceSize: Qt.size(56, 56)
                    source: "image://icon/" + encodeURIComponent(entry.icon || modelData)
                }
                Label {
                    Layout.fillWidth: true
                    text: entry.name || modelData
                    elide: Text.ElideRight
                }
                Label {
                    visible: !entry.name
                    text: qsTr("not installed")
                    opacity: 0.5
                }
                ToolButton { text: "↑"; enabled: index > 0; onClicked: config.moveLauncher(index, index - 1) }
                ToolButton { text: "↓"; enabled: index < page.pinned.length - 1; onClicked: config.moveLauncher(index, index + 1) }
                ToolButton { text: qsTr("Unpin"); onClicked: config.unpin(modelData) }
            }
        }
    }

    SectionTitle { text: qsTr("Add an app") }
    TextField {
        id: search
        width: parent.width
        placeholderText: qsTr("Search installed apps")
        selectByMouse: true
    }
    Item { width: 1; height: 8 }
    Repeater {
        model: { apps.apps; return apps.search(search.text).filter(id => page.pinned.indexOf(id) < 0).slice(0, 60) }
        delegate: Item {
            required property string modelData
            readonly property var entry: apps.entry(modelData)
            width: parent ? parent.width : 400
            height: 42
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 6
                spacing: 10
                Image {
                    Layout.preferredWidth: 24
                    Layout.preferredHeight: 24
                    sourceSize: Qt.size(48, 48)
                    source: "image://icon/" + encodeURIComponent(entry.icon || modelData)
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0
                    Label { Layout.fillWidth: true; text: entry.name; elide: Text.ElideRight }
                    Label { Layout.fillWidth: true; text: entry.comment || ""; visible: text !== ""; opacity: 0.5; font.pixelSize: 11; elide: Text.ElideRight }
                }
                Button { text: qsTr("Pin"); flat: true; onClicked: config.pin(modelData) }
            }
        }
    }
}
