import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import ".."
import "../AppletInfo.js" as AppletInfo

SettingsPage {
    id: page
    required property var app
    readonly property var v: config.values
    heading: qsTr("Applets & desklets")
    subheading: qsTr("Small live tools that sit in the dock, or on the desktop as desklets. Drag a desklet to move it; hover it to put it back.")

    readonly property var applets: v.applets

    Repeater {
        model: page.applets
        delegate: Item {
            id: card
            required property var modelData
            required property int index
            readonly property string id: modelData.id
            width: parent ? parent.width : 400
            height: body.implicitHeight + 20
            Rectangle {
                anchors.fill: parent
                anchors.topMargin: 6
                radius: Math.min(8, page.app.radius + 2)
                color: Qt.rgba(1, 1, 1, 0.04)
                border.color: Qt.rgba(1, 1, 1, 0.06)
            }
            ColumnLayout {
                id: body
                x: 14
                y: 14
                width: parent.width - 28
                spacing: 6
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12
                    Item {
                        Layout.preferredWidth: 44
                        Layout.preferredHeight: 44
                        AppletView {
                            width: 44 * 1.8
                            height: width
                            scale: 1 / 1.8
                            transformOrigin: Item.TopLeft
                            appletId: card.id
                            mode: "dock"
                            app: page.app
                        }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 1
                        Label { text: AppletInfo.name(card.id); font.pixelSize: 15; font.weight: Font.DemiBold }
                        Label { Layout.fillWidth: true; text: AppletInfo.description(card.id); opacity: 0.6; wrapMode: Text.Wrap; font.pixelSize: 12 }
                    }
                    ToolButton { text: "↑"; enabled: card.index > 0; onClicked: config.moveApplet(card.index, card.index - 1) }
                    ToolButton { text: "↓"; enabled: card.index < page.applets.length - 1; onClicked: config.moveApplet(card.index, card.index + 1) }
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 18
                    Switch {
                        text: qsTr("Show")
                        checked: card.modelData.enabled
                        onToggled: config.setApplet(card.id, "enabled", checked)
                    }
                    RadioButton {
                        text: qsTr("In the dock")
                        enabled: card.modelData.enabled
                        checked: !card.modelData.detached
                        onToggled: if (checked) config.setApplet(card.id, "detached", false)
                    }
                    RadioButton {
                        text: qsTr("On the desktop")
                        enabled: card.modelData.enabled
                        checked: card.modelData.detached
                        onToggled: if (checked) config.setApplet(card.id, "detached", true)
                    }
                    Item { Layout.fillWidth: true }
                }

                // Per-applet settings.
                ColumnLayout {
                    visible: card.id === "clock"
                    Layout.fillWidth: true
                    RowLayout {
                        spacing: 18
                        Switch { text: qsTr("Analog"); checked: page.v.clockAnalog; onToggled: config.set("clockAnalog", checked) }
                        Switch { text: qsTr("24-hour"); checked: page.v.clock24h; onToggled: config.set("clock24h", checked) }
                        Switch { text: qsTr("Seconds"); checked: page.v.clockSeconds; onToggled: config.set("clockSeconds", checked) }
                    }
                    RowLayout {
                        visible: !page.v.clockAnalog
                        Layout.fillWidth: true
                        spacing: 10
                        ComboBox {
                            id: clockFontBox
                            Layout.fillWidth: true
                            model: fonts.families
                            editable: false
                            currentIndex: model.indexOf(page.v.clockFont)
                            displayText: currentIndex < 0 && page.v.clockFont.length > 0 ? page.v.clockFont : currentText
                            onActivated: (index) => config.set("clockFont", model[index])
                        }
                        ValueSlider {
                            Layout.preferredWidth: 190
                            from: 0.6; to: 2.5; stepSize: 0.1; decimals: 1
                            value: page.v.clockFontScale
                            suffix: "×"
                            onMoved: (x) => config.set("clockFontScale", x)
                        }
                    }
                }
                ColumnLayout {
                    visible: card.id === "weather"
                    Layout.fillWidth: true
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10
                        TextField {
                            id: city
                            Layout.fillWidth: true
                            placeholderText: qsTr("City, e.g. Oslo")
                            text: page.v.weatherCity
                            selectByMouse: true
                            onEditingFinished: config.set("weatherCity", text.trim())
                        }
                        ComboBox {
                            Layout.preferredWidth: 150
                            textRole: "text"
                            valueRole: "value"
                            model: [{ text: qsTr("°C, km/h"), value: "metric" }, { text: qsTr("°F, mph"), value: "imperial" }]
                            Component.onCompleted: currentIndex = indexOfValue(page.v.weatherUnits)
                            onActivated: config.set("weatherUnits", currentValue)
                        }
                    }
                    Label {
                        Layout.fillWidth: true
                        wrapMode: Text.Wrap
                        opacity: 0.7
                        font.pixelSize: 12
                        color: weather.status === "error" ? "#ff8080" : palette.text
                        text: weather.status === "ok" ? qsTr("%1: %2, %3").arg(weather.place).arg(Math.round(weather.temperature) + weather.unit).arg(weather.description)
                            : weather.status === "loading" ? qsTr("Looking up the weather…")
                            : weather.status === "error" ? weather.error
                            : qsTr("Weather comes from Open-Meteo; no account needed.")
                    }
                }
            }
        }
    }

    SectionTitle { text: qsTr("Desklets") }
    SettingRow {
        title: qsTr("Desklet layer")
        description: qsTr("Below windows, desklets are part of the desktop; above, they float over everything.")
        ComboBox {
            width: parent.width
            textRole: "text"
            valueRole: "value"
            model: [{ text: qsTr("Below windows"), value: "bottom" }, { text: qsTr("Above windows"), value: "top" }]
            Component.onCompleted: currentIndex = indexOfValue(page.v.deskletLayer)
            onActivated: config.set("deskletLayer", currentValue)
        }
    }
}
