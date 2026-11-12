import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

SettingsPage {
    id: page
    required property var app
    readonly property var v: config.values
    heading: qsTr("Rendering & system")
    property bool confirmReset: false

    SectionTitle { text: qsTr("Rendering") }
    SettingRow {
        title: qsTr("Renderer")
        description: qsTr("OpenGL draws on the GPU: smooth and light on the CPU. Software draws on the CPU, for graphics drivers that crash or flicker; effects like shadows and fading reflections are simplified.")
        ColumnLayout {
            width: parent.width
            RadioButton {
                text: qsTr("OpenGL (hardware)")
                checked: page.v.renderer !== "software"
                onToggled: if (checked) config.set("renderer", "opengl")
            }
            RadioButton {
                text: qsTr("Software (CPU)")
                checked: page.v.renderer === "software"
                onToggled: if (checked) config.set("renderer", "software")
            }
        }
    }
    SettingRow {
        title: qsTr("Running with")
        description: (page.v.renderer === "software") !== (shell.renderer === "software")
                     ? qsTr("Restart the dock to switch renderers.") : ""
        RowLayout {
            width: parent.width
            Label { Layout.fillWidth: true; text: shell.renderer === "software" ? qsTr("Software") : qsTr("OpenGL"); opacity: 0.8 }
            Button {
                visible: (page.v.renderer === "software") !== (shell.renderer === "software")
                text: qsTr("Restart")
                highlighted: true
                onClicked: { config.saveNow(); shell.restart() }
            }
        }
    }

    SectionTitle { text: qsTr("Files") }
    SettingRow {
        title: qsTr("Settings file")
        controlWidth: 340
        Label { width: parent.width; text: config.configPath; elide: Text.ElideMiddle; opacity: 0.7 }
    }
    SettingRow {
        title: qsTr("Installed themes")
        controlWidth: 340
        Label { width: parent.width; text: config.themesPath; elide: Text.ElideMiddle; opacity: 0.7 }
    }
    SettingRow {
        title: qsTr("Start with the desktop")
        description: qsTr("Add this line to ~/.config/hypr/autostart.lua:")
        controlWidth: 340
        TextField { width: parent.width; readOnly: true; text: 'o.launch_on_start("kai")'; selectByMouse: true }
    }

    SectionTitle { text: qsTr("Reset") }
    SettingRow {
        title: qsTr("Reset all settings")
        description: page.confirmReset ? qsTr("Everything except your pinned apps goes back to the defaults.") : qsTr("Pinned apps and installed themes are kept.")
        RowLayout {
            width: parent.width
            Button {
                text: page.confirmReset ? qsTr("Yes, reset") : qsTr("Reset…")
                Material.background: page.confirmReset ? "#c0392b" : undefined
                onClicked: {
                    if (page.confirmReset) { config.resetAll(); page.confirmReset = false }
                    else page.confirmReset = true
                }
            }
            Button { visible: page.confirmReset; text: qsTr("Cancel"); flat: true; onClicked: page.confirmReset = false }
        }
    }

    SectionTitle { text: qsTr("About") }
    Label {
        width: parent.width
        wrapMode: Text.Wrap
        opacity: 0.7
        text: qsTr("kai %1 — a dock for Hyprland. Run “kai” again to open these settings, “kai quit” to stop it.").arg(shell.version)
    }
    Item { width: 1; height: 12 }
    Button { text: qsTr("Quit kai"); flat: true; onClicked: Qt.quit() }
}
