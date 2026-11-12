import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

SettingsPage {
    id: page
    required property var app
    readonly property var v: config.values
    heading: qsTr("Behavior")
    subheading: qsTr("How the dock responds, hides and animates.")

    component Choice: ComboBox {
        property string key
        width: parent.width
        textRole: "text"
        valueRole: "value"
        Component.onCompleted: currentIndex = Math.max(0, indexOfValue(config.values[key]))
        onActivated: config.set(key, currentValue)
    }
    readonly property var animations: [
        { text: qsTr("Bounce"), value: "bounce" }, { text: qsTr("Pulse"), value: "pulse" },
        { text: qsTr("Flip"), value: "flip" }, { text: qsTr("Wobble"), value: "wobble" },
        { text: qsTr("Blink"), value: "blink" }, { text: qsTr("None"), value: "none" }]

    SectionTitle { text: qsTr("Visibility") }
    SettingRow {
        title: qsTr("Hide automatically")
        description: qsTr("Slides away when the pointer leaves; push the pointer against the edge to bring it back.")
        Switch { checked: page.v.autohide; onToggled: config.set("autohide", checked) }
    }
    SettingRow {
        visible: page.v.autohide
        title: qsTr("Hide after")
        ValueSlider { from: 100; to: 3000; stepSize: 100; value: page.v.autohideDelay; suffix: " ms"; onMoved: (x) => config.set("autohideDelay", x) }
    }
    SettingRow {
        title: qsTr("Keep windows clear of the dock")
        description: qsTr("Reserves the dock's space so maximized and tiled windows stop at it.")
        enabled: !page.v.autohide
        Switch { checked: page.v.reserveSpace; onToggled: config.set("reserveSpace", checked) }
    }
    SettingRow {
        title: qsTr("Layer")
        Choice {
            key: "layer"
            model: [{ text: qsTr("Above windows"), value: "top" }, { text: qsTr("Above everything"), value: "overlay" },
                    { text: qsTr("Below windows"), value: "bottom" }]
        }
    }

    SectionTitle { text: qsTr("Apps") }
    SettingRow {
        title: qsTr("Show running apps that aren't pinned")
        Switch { checked: page.v.showRunning; onToggled: config.set("showRunning", checked) }
    }
    SettingRow {
        title: qsTr("Only from the current workspace")
        description: qsTr("Unpinned apps appear only while they have a window on the workspace in front of you.")
        enabled: page.v.showRunning
        Switch { checked: page.v.currentWorkspaceOnly; onToggled: config.set("currentWorkspaceOnly", checked) }
    }
    SettingRow {
        title: qsTr("Clicking the focused app minimizes it")
        description: qsTr("With several windows, a click cycles through them instead. Scrolling over an icon always cycles.")
        Switch { checked: page.v.clickActiveMinimizes; onToggled: config.set("clickActiveMinimizes", checked) }
    }

    SectionTitle { text: qsTr("Animations") }
    SettingRow {
        title: qsTr("On click")
        Choice { key: "clickAnimation"; model: page.animations }
    }
    SettingRow {
        title: qsTr("While an app starts")
        Choice { key: "launchAnimation"; model: page.animations }
    }
    SettingRow {
        title: qsTr("When a window wants attention")
        Choice { key: "attentionAnimation"; model: page.animations }
    }
    SettingRow {
        title: qsTr("Speed")
        ValueSlider { from: 0.4; to: 2.5; stepSize: 0.1; decimals: 1; value: page.v.animationSpeed; suffix: "×"; onMoved: (x) => config.set("animationSpeed", x) }
    }
}
