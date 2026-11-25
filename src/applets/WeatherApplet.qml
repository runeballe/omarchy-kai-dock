import QtQuick

// Current conditions in the dock; the forecast as a desklet.
Item {
    id: applet
    property string mode: "dock"
    property var app
    readonly property bool desklet: mode === "desklet"
    readonly property size deskletSize: Qt.size(320, 210)
    // Text follows the theme, so light themes stay readable.
    readonly property color fg: app ? app.color(config.theme.textColor, "#f2f2f2") : "#f2f2f2"
    readonly property color soft: Qt.rgba(fg.r, fg.g, fg.b, 0.72)
    readonly property bool ready: weather.status === "ok"
    readonly property string temp: ready ? Math.round(weather.temperature) + "°" : "–"

    readonly property string title: {
        if (weather.status === "idle") return qsTr("Weather — set a city in Settings")
        if (weather.status === "error") return qsTr("Weather unavailable: %1").arg(weather.error)
        if (!ready) return qsTr("Loading weather…")
        return weather.place + ": " + Math.round(weather.temperature) + weather.unit + ", " + weather.description
    }

    function activate(button) {
        if (weather.status === "idle") app.openSettings("applets")
        else weather.refresh()
    }
    function menuEntries() {
        return [{ text: qsTr("Refresh weather"), action: () => weather.refresh() }]
    }

    // Dock.
    Item {
        visible: !applet.desklet
        anchors.fill: parent
        WeatherGlyph {
            width: parent.width * 0.74
            height: width
            x: parent.width * 0.08
            y: parent.height * 0.04
            kind: applet.ready ? weather.kind : "unknown"
            day: weather.day
        }
        Text {
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: parent.width * 0.1
            text: applet.temp
            color: applet.fg
            style: Text.Outline
            styleColor: applet.fg.hslLightness > 0.5 ? Qt.rgba(0, 0, 0, 0.35) : Qt.rgba(1, 1, 1, 0.6)
            font.pixelSize: parent.width * 0.27
            font.weight: Font.Bold
        }
    }

    // Desklet.
    Item {
        visible: applet.desklet
        anchors.fill: parent
        anchors.margins: 16

        Text {
            visible: !applet.ready
            anchors.centerIn: parent
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            text: applet.title
            color: applet.fg
            font.pixelSize: 15
        }

        Row {
            visible: applet.ready
            spacing: 12
            WeatherGlyph {
                width: 86
                height: 86
                kind: weather.kind
                day: weather.day
            }
            Column {
                anchors.verticalCenter: parent.verticalCenter
                Text {
                    text: Math.round(weather.temperature) + weather.unit
                    color: applet.fg
                    font.pixelSize: 40
                    font.weight: Font.DemiBold
                }
                Text {
                    text: weather.description + " · " + weather.place
                    color: applet.soft
                    font.pixelSize: 13
                }
            }
        }

        Row {
            visible: applet.ready
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 18
            Repeater {
                model: weather.forecast.slice(1, 4)
                Column {
                    required property var modelData
                    spacing: 2
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: { translator.language; return modelData.date.toLocaleDateString(Qt.locale(), "ddd") }
                        color: applet.soft
                        font.pixelSize: 12
                    }
                    WeatherGlyph {
                        anchors.horizontalCenter: parent.horizontalCenter
                        width: 40
                        height: 40
                        kind: modelData.kind
                    }
                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: Math.round(modelData.high) + "° / " + Math.round(modelData.low) + "°"
                        color: applet.fg
                        font.pixelSize: 12
                    }
                }
            }
        }
    }
}
