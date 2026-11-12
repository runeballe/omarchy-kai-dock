import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

SettingsPage {
    id: page
    required property var app
    readonly property var t: config.theme
    readonly property var overrides: config.values.themeOverrides || ({})
    readonly property bool tweaked: Object.keys(overrides).length > 0
    heading: qsTr("Themes")
    subheading: qsTr("Pick a theme with one click, then fine-tune it below. Tweaks are kept on top of the theme until you reset them or save them as a theme of your own.")

    property string installMessage: ""
    property bool installOk: true
    Connections {
        target: config
        function onThemeInstallFinished(ok, message) {
            page.installOk = ok
            page.installMessage = message
        }
    }

    SectionTitle { text: qsTr("Gallery") }
    Flow {
        width: parent.width
        spacing: 12
        Repeater {
            model: config.themes
            delegate: Rectangle {
                id: card
                required property var modelData
                readonly property bool current: modelData.id === config.values.theme
                width: (page.availableWidth - 56 - 24) / 3
                height: 196
                radius: Math.min(10, page.app.radius + 4)
                color: Qt.rgba(1, 1, 1, current ? 0.07 : 0.035)
                border.width: current ? 2 : 1
                border.color: current ? page.app.accent : Qt.rgba(1, 1, 1, 0.08)

                ThemePreview {
                    x: 10; y: 10
                    width: parent.width - 20
                    height: 86
                    app: page.app
                    themeData: card.modelData.data
                }
                ColumnLayout {
                    x: 12
                    y: 104
                    width: parent.width - 24
                    spacing: 2
                    Label { text: card.modelData.name; font.weight: Font.DemiBold; elide: Text.ElideRight; Layout.fillWidth: true }
                    Label {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 30
                        text: card.modelData.description || (card.modelData.author ? qsTr("by %1").arg(card.modelData.author) : "")
                        opacity: 0.6
                        font.pixelSize: 11
                        wrapMode: Text.Wrap
                        elide: Text.ElideRight
                        maximumLineCount: 2
                    }
                }
                RowLayout {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.margins: 6
                    Button {
                        text: card.current ? (page.tweaked ? qsTr("Reapply") : qsTr("Applied")) : qsTr("Apply")
                        flat: card.current && !page.tweaked
                        highlighted: !card.current
                        enabled: !card.current || page.tweaked
                        onClicked: config.applyTheme(card.modelData.id)
                    }
                    Item { Layout.fillWidth: true }
                    ToolButton {
                        visible: !card.modelData.builtin
                        text: qsTr("Delete")
                        onClicked: config.deleteTheme(card.modelData.id)
                    }
                }
            }
        }
    }

    SectionTitle { text: qsTr("Install a theme") }
    RowLayout {
        width: parent.width
        spacing: 10
        TextField {
            id: source
            Layout.fillWidth: true
            placeholderText: qsTr("Theme address (https://…/theme.json) or a file path")
            selectByMouse: true
            onAccepted: install.clicked()
        }
        Button {
            id: install
            text: qsTr("Install")
            highlighted: true
            enabled: source.text.trim() !== ""
            onClicked: { page.installMessage = qsTr("Installing…"); page.installOk = true; config.installThemeFromUrl(source.text) }
        }
    }
    Label {
        width: parent.width
        visible: page.installMessage !== ""
        text: page.installMessage
        color: page.installOk ? palette.text : "#ff8080"
        opacity: 0.8
        topPadding: 6
        wrapMode: Text.Wrap
    }

    SectionTitle { text: qsTr("Fine-tune “%1”").arg(page.t.name) }
    Label {
        width: parent.width
        text: page.tweaked ? qsTr("%n setting(s) changed from the theme.", "", Object.keys(page.overrides).length) : qsTr("Exactly as the theme comes.")
        opacity: 0.6
        bottomPadding: 4
    }

    function set(key, value) { config.setThemeOverride(key, value) }

    SettingRow {
        title: qsTr("Background")
        ColorField { app: page.app; allowAccent: false; value: page.t.background; onEdited: (v) => page.set("background", v) }
    }
    SettingRow {
        title: qsTr("Background, far side")
        description: qsTr("A second color for a gradient across the shelf.")
        ColorField { app: page.app; allowAccent: false; value: page.t.background2 || page.t.background; onEdited: (v) => page.set("background2", v) }
    }
    SettingRow {
        title: qsTr("Background opacity")
        ValueSlider { from: 0; to: 1; stepSize: 0.01; decimals: 2; value: page.t.backgroundOpacity; onMoved: (x) => page.set("backgroundOpacity", x) }
    }
    SettingRow {
        title: qsTr("Border")
        ColorField { app: page.app; value: page.t.border; onEdited: (v) => page.set("border", v) }
    }
    SettingRow {
        title: qsTr("Border opacity")
        ValueSlider { from: 0; to: 1; stepSize: 0.01; decimals: 2; value: page.t.borderOpacity; onMoved: (x) => page.set("borderOpacity", x) }
    }
    SettingRow {
        title: qsTr("Border width")
        ValueSlider { from: 0; to: 4; stepSize: 1; value: page.t.borderWidth; suffix: " px"; onMoved: (x) => page.set("borderWidth", x) }
    }
    SettingRow {
        title: qsTr("Corner radius")
        RowLayout {
            width: parent.width
            CheckBox {
                text: qsTr("Hyprland")
                checked: page.t.radius < 0
                onToggled: page.set("radius", checked ? -1 : 16)
            }
            ValueSlider {
                Layout.fillWidth: true
                enabled: page.t.radius >= 0
                from: 0; to: 40; stepSize: 1
                value: page.t.radius >= 0 ? page.t.radius : omarchy.radius
                onMoved: (x) => page.set("radius", x)
            }
        }
    }
    SettingRow {
        title: qsTr("Padding")
        ValueSlider { from: 0; to: 24; stepSize: 1; value: page.t.padding; suffix: " px"; onMoved: (x) => page.set("padding", x) }
    }
    SettingRow {
        title: qsTr("Gloss")
        ValueSlider { from: 0; to: 0.5; stepSize: 0.01; decimals: 2; value: page.t.gloss; onMoved: (x) => page.set("gloss", x) }
    }
    SettingRow {
        title: qsTr("Reflections")
        description: qsTr("Mirror images of the icons on the shelf.")
        ValueSlider { from: 0; to: 0.6; stepSize: 0.01; decimals: 2; value: page.t.reflection; onMoved: (x) => page.set("reflection", x) }
    }
    SettingRow {
        title: qsTr("Icon shadows")
        ValueSlider { from: 0; to: 1; stepSize: 0.05; decimals: 2; value: page.t.iconShadow; onMoved: (x) => page.set("iconShadow", x) }
    }
    SettingRow {
        title: qsTr("3D plane tilt")
        description: qsTr("Steeper angles make the plane recede less.")
        ValueSlider { from: 30; to: 80; stepSize: 1; value: page.t.planeAngle; suffix: "°"; onMoved: (x) => page.set("planeAngle", x) }
    }
    SettingRow {
        title: qsTr("Running indicator")
        ComboBox {
            width: parent.width
            textRole: "text"
            valueRole: "value"
            model: [{ text: qsTr("Dots"), value: "dot" }, { text: qsTr("Bar"), value: "bar" },
                    { text: qsTr("Glow"), value: "glow" }, { text: qsTr("None"), value: "none" }]
            currentIndex: indexOfValue(page.t.indicator)
            onActivated: page.set("indicator", currentValue)
        }
    }
    SettingRow {
        title: qsTr("Indicator color")
        ColorField { app: page.app; value: page.t.indicatorColor; onEdited: (v) => { page.set("indicatorColor", v); page.set("activeColor", v) } }
    }
    SettingRow {
        title: qsTr("Separators")
        ComboBox {
            width: parent.width
            textRole: "text"
            valueRole: "value"
            model: [{ text: qsTr("Line"), value: "line" }, { text: qsTr("Gap"), value: "gap" }]
            currentIndex: indexOfValue(page.t.separator)
            onActivated: page.set("separator", currentValue)
        }
    }
    SettingRow {
        title: qsTr("Accent")
        description: qsTr("Leave empty to follow the Omarchy theme's accent.")
        ColorField { app: page.app; allowAccent: false; value: page.t.accentOverride; onEdited: (v) => page.set("accentOverride", v) }
    }

    Item { width: 1; height: 14 }
    RowLayout {
        width: parent.width
        spacing: 10
        Button {
            text: qsTr("Reset tweaks")
            enabled: page.tweaked
            onClicked: config.resetThemeOverrides()
        }
        Item { Layout.fillWidth: true }
        TextField {
            id: saveName
            Layout.preferredWidth: 220
            placeholderText: qsTr("New theme name")
            selectByMouse: true
        }
        Button {
            text: qsTr("Save as theme")
            highlighted: true
            enabled: saveName.text.trim() !== ""
            onClicked: {
                const id = config.saveThemeAs(saveName.text)
                page.installOk = id !== ""
                page.installMessage = id !== "" ? qsTr("Saved “%1”.").arg(saveName.text.trim()) : qsTr("Couldn't save the theme.")
                saveName.text = ""
            }
        }
    }
}
