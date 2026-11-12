import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

SettingsPage {
    id: page
    required property var app
    readonly property var v: config.values
    heading: qsTr("Appearance")
    subheading: qsTr("Where the dock sits and how it looks. The theme sets colors; these settings work with any theme.")

    SectionTitle { text: qsTr("Placement") }
    SettingRow {
        title: qsTr("Screen edge")
        ComboBox {
            width: parent.width
            textRole: "text"
            valueRole: "value"
            model: [{ text: qsTr("Bottom"), value: "bottom" }, { text: qsTr("Top"), value: "top" },
                    { text: qsTr("Left"), value: "left" }, { text: qsTr("Right"), value: "right" }]
            Component.onCompleted: currentIndex = indexOfValue(page.v.edge)
            onActivated: config.set("edge", currentValue)
        }
    }
    SettingRow {
        title: qsTr("Screen")
        description: qsTr("Changing the screen restarts the dock.")
        ComboBox {
            width: parent.width
            model: [qsTr("Primary screen")].concat(shell.screens)
            Component.onCompleted: currentIndex = Math.max(0, shell.screens.indexOf(page.v.screen) + 1)
            onActivated: {
                const name = currentIndex === 0 ? "" : shell.screens[currentIndex - 1]
                if (name !== page.v.screen) {
                    config.set("screen", name)
                    config.saveNow()
                    shell.restart()
                }
            }
        }
    }
    SettingRow {
        title: qsTr("Distance from the edge")
        description: qsTr("The gap under a floating dock.")
        ValueSlider { from: 0; to: 48; stepSize: 1; value: page.v.margin; suffix: " px"; onMoved: (x) => config.set("margin", x) }
    }

    SectionTitle { text: qsTr("Style") }
    SettingRow {
        title: qsTr("View")
        description: qsTr("Floating shelf, full-width panel, or icons standing on a 3D plane.")
        ComboBox {
            width: parent.width
            textRole: "text"
            valueRole: "value"
            model: [{ text: qsTr("As the theme says"), value: "" }, { text: qsTr("Floating"), value: "floating" },
                    { text: qsTr("Panel"), value: "panel" }, { text: qsTr("3D plane"), value: "plane" }]
            Component.onCompleted: currentIndex = indexOfValue(page.v.view)
            onActivated: config.set("view", currentValue)
        }
    }
    SettingRow {
        title: qsTr("Icon size")
        ValueSlider { from: 24; to: 96; stepSize: 2; value: page.v.iconSize; suffix: " px"; onMoved: (x) => config.set("iconSize", x) }
    }
    SettingRow {
        title: qsTr("Magnification")
        description: qsTr("How big icons grow under the pointer. 1× turns zoom off.")
        ValueSlider { from: 1; to: 2.6; stepSize: 0.05; decimals: 2; value: page.v.zoom; suffix: "×"; onMoved: (x) => config.set("zoom", x) }
    }
    SettingRow {
        title: qsTr("Magnification width")
        description: qsTr("How many neighbouring icons grow along with the one under the pointer.")
        ValueSlider { from: 1.2; to: 6; stepSize: 0.1; decimals: 1; value: page.v.zoomRange; onMoved: (x) => config.set("zoomRange", x) }
    }
    SettingRow {
        title: qsTr("Spacing between icons")
        RowLayout {
            width: parent.width
            CheckBox {
                text: qsTr("Theme")
                checked: page.v.spacing < 0
                onToggled: config.set("spacing", checked ? -1 : config.theme.spacing)
            }
            ValueSlider {
                Layout.fillWidth: true
                enabled: page.v.spacing >= 0
                from: 0; to: 32; stepSize: 1
                value: page.v.spacing >= 0 ? page.v.spacing : config.theme.spacing
                onMoved: (x) => config.set("spacing", x)
            }
        }
    }
    SettingRow {
        title: qsTr("Show names on hover")
        Switch { checked: page.v.showLabels; onToggled: config.set("showLabels", checked) }
    }
}
