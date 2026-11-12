import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

// A color as a hex value, with a swatch, a few presets, and "accent" to
// follow the Omarchy accent.
RowLayout {
    id: field
    required property var app
    property string value: ""
    property bool allowAccent: true
    signal edited(string value)
    width: parent ? parent.width : 260
    spacing: 6

    Rectangle {
        Layout.preferredWidth: 28
        Layout.preferredHeight: 28
        radius: Math.min(4, field.app.radius)
        color: field.app.color(field.value, "transparent")
        border.color: Qt.rgba(1, 1, 1, 0.3)
        MouseArea {
            anchors.fill: parent
            onClicked: presets.open()
        }
        Popup {
            id: presets
            y: parent.height + 4
            padding: 8
            Grid {
                columns: 6
                spacing: 6
                Repeater {
                    model: ["#000000", "#16161a", "#2a2a33", "#3a3a48", "#8a8fa8", "#ffffff",
                            "#f4f1ea", "#1e3a5f", "#2f6fb3", "#7fd1ff", "#4cd964", "#ffd60a",
                            "#ff9f1c", "#ff4d4f", "#c86bfa", "#120a1e", "#0d2b24", "#5a3e2b"]
                    Rectangle {
                        required property string modelData
                        width: 24
                        height: 24
                        radius: 4
                        color: modelData
                        border.color: Qt.rgba(1, 1, 1, 0.3)
                        MouseArea {
                            anchors.fill: parent
                            onClicked: { field.edited(modelData); presets.close() }
                        }
                    }
                }
            }
        }
    }
    TextField {
        id: input
        Layout.fillWidth: true
        text: field.value
        enabled: field.value !== "accent"
        placeholderText: "#rrggbb"
        selectByMouse: true
        onEditingFinished: {
            const v = text.trim()
            if (/^#([0-9a-fA-F]{6}|[0-9a-fA-F]{8})$/.test(v)) field.edited(v)
            else text = field.value
        }
    }
    CheckBox {
        visible: field.allowAccent
        text: qsTr("Accent")
        checked: field.value === "accent"
        onToggled: field.edited(checked ? "accent" : "#ffffff")
    }
}
