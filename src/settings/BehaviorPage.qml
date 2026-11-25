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
        { text: qsTr("Wave"), value: "wave" }, { text: qsTr("Rotation"), value: "rotate" },
        { text: qsTr("Blink"), value: "blink" }, { text: qsTr("None"), value: "none" }]
    readonly property var transitions: [
        { text: qsTr("Evaporate"), value: "evaporate" }, { text: qsTr("Fade out"), value: "fade" },
        { text: qsTr("Explode"), value: "explode" }, { text: qsTr("Break"), value: "break" },
        { text: qsTr("Black hole"), value: "blackhole" }, { text: qsTr("None"), value: "none" }]
    // One toggle per fixed dock element: the launcher and each pinned app.
    readonly property var separatorChoices: {
        const items = [{ key: "launcher", label: qsTr("Launcher") }]
        for (let i = 0; i < dockModel.count; ++i) {
            const row = dockModel.get(i)
            if (row.kind === "app" && row.pinned)
                items.push({ key: row.appId, label: row.name })
        }
        return items
    }

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
    SettingRow {
        title: qsTr("Separators")
        description: qsTr("Adds a separator line after each selected item.")
        enabled: true
        Flow {
            width: parent.width
            spacing: 6
            Repeater {
                model: page.separatorChoices
                delegate: AbstractButton {
                    id: sepChip
                    required property var modelData
                    readonly property bool on: page.v.separators.includes(sepChip.modelData.key)
                    checkable: true
                    checked: on
                    hoverEnabled: true
                    onClicked: {
                        const cur = (page.v.separators || []).filter(function (k) { return k !== sepChip.modelData.key })
                        if (!on) cur.push(sepChip.modelData.key)
                        config.set("separators", cur)
                    }
                    background: Rectangle {
                        radius: Math.min(6, app.radius + 6)
                        color: sepChip.checked ? app.withAlpha(app.accent, 0.85) : (sepChip.hovered ? Qt.rgba(1, 1, 1, 0.14) : Qt.rgba(1, 1, 1, 0.07))
                    }
                    contentItem: Text {
                        text: sepChip.modelData.label
                        color: sepChip.checked ? "#111114" : "#f2f2f2"
                        font.pixelSize: 13
                        leftPadding: 10
                        rightPadding: 10
                        topPadding: 5
                        bottomPadding: 5
                    }
                }
            }
        }
    }

    SectionTitle { text: qsTr("Animations") }
    SettingRow {
        title: qsTr("On click")
        Choice { key: "clickAnimation"; model: page.animations }
    }
    SettingRow {
        title: qsTr("On hover")
        Choice { key: "hoverAnimation"; model: page.animations }
    }
    SettingRow {
        title: qsTr("When an icon appears")
        Choice { key: "appearEffect"; model: page.transitions }
    }
    SettingRow {
        title: qsTr("When an icon is removed")
        Choice { key: "removeEffect"; model: page.transitions }
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
