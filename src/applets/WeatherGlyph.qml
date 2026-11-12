import QtQuick

// A small drawn weather symbol: sun, moon, clouds, rain, snow, storm or fog.
Item {
    id: glyph
    property string kind: "clear"
    property bool day: true
    readonly property real u: width / 100
    readonly property bool sunny: kind === "clear" || kind === "partly"
    readonly property bool cloudy: kind !== "clear" && kind !== "unknown"
    readonly property bool wet: kind === "rain" || kind === "drizzle" || kind === "storm"

    // Sun or moon.
    Item {
        visible: glyph.sunny
        width: glyph.kind === "partly" ? 62 * glyph.u : 76 * glyph.u
        height: width
        x: glyph.kind === "partly" ? 8 * glyph.u : (glyph.width - width) / 2
        y: glyph.kind === "partly" ? 4 * glyph.u : (glyph.height - height) / 2
        Repeater {
            model: glyph.day ? 8 : 0
            Item {
                required property int index
                anchors.fill: parent
                rotation: index * 45
                Rectangle {
                    x: parent.width / 2 - width / 2
                    y: 0
                    width: parent.width * 0.08
                    height: parent.width * 0.18
                    radius: width / 2
                    color: "#ffc93c"
                }
            }
        }
        Rectangle {
            anchors.centerIn: parent
            width: parent.width * (glyph.day ? 0.56 : 0.7)
            height: width
            radius: width / 2
            color: glyph.day ? "#ffb300" : "#e8e6f0"
            gradient: glyph.day ? sunGradient : null
            Gradient {
                id: sunGradient
                GradientStop { position: 0; color: "#ffe066" }
                GradientStop { position: 1; color: "#ff9f1c" }
            }
            // A dark bite out of the full circle makes a crescent.
            Rectangle {
                visible: !glyph.day
                width: parent.width * 0.8
                height: width
                radius: width / 2
                x: parent.width * 0.35
                y: -parent.height * 0.12
                color: "#26262e"
            }
        }
    }

    // Cloud.
    Item {
        id: cloud
        visible: glyph.cloudy
        width: 84 * glyph.u
        height: 50 * glyph.u
        x: (glyph.width - width) / 2 + (glyph.kind === "partly" ? 8 * glyph.u : 0)
        y: glyph.wet || glyph.kind === "snow" ? 14 * glyph.u : 28 * glyph.u
        readonly property color tint: glyph.kind === "storm" ? "#6f7385" : glyph.wet ? "#a9b0c2" : "#eef1f6"
        Rectangle { x: 0; y: parent.height * 0.42; width: parent.width; height: parent.height * 0.58; radius: height / 2; color: cloud.tint }
        Rectangle { x: parent.width * 0.14; y: parent.height * 0.18; width: parent.width * 0.42; height: width; radius: width / 2; color: cloud.tint }
        Rectangle { x: parent.width * 0.4; y: 0; width: parent.width * 0.48; height: width; radius: width / 2; color: cloud.tint }
    }

    // Rain or drizzle.
    Repeater {
        model: glyph.wet ? (glyph.kind === "drizzle" ? 3 : 4) : 0
        Rectangle {
            required property int index
            x: (22 + index * 17) * glyph.u
            y: 70 * glyph.u
            width: 5 * glyph.u
            height: (glyph.kind === "drizzle" ? 10 : 18) * glyph.u
            radius: width / 2
            rotation: 18
            color: "#4fa3ff"
        }
    }

    // Snow.
    Repeater {
        model: glyph.kind === "snow" ? 4 : 0
        Rectangle {
            required property int index
            x: (22 + index * 17) * glyph.u
            y: (72 + (index % 2) * 10) * glyph.u
            width: 9 * glyph.u
            height: width
            radius: width / 2
            color: "white"
        }
    }

    // Lightning.
    Text {
        visible: glyph.kind === "storm"
        text: "ϟ"
        color: "#ffd23f"
        font.pixelSize: 44 * glyph.u
        font.bold: true
        x: 40 * glyph.u
        y: 50 * glyph.u
    }

    // Fog.
    Repeater {
        model: glyph.kind === "fog" ? 3 : 0
        Rectangle {
            required property int index
            x: (12 + index * 6) * glyph.u
            y: (64 + index * 11) * glyph.u
            width: (72 - index * 10) * glyph.u
            height: 6 * glyph.u
            radius: height / 2
            color: Qt.rgba(1, 1, 1, 0.75)
        }
    }

    Text {
        visible: glyph.kind === "unknown"
        anchors.centerIn: parent
        text: "?"
        color: "white"
        font.pixelSize: 56 * glyph.u
        font.bold: true
    }
}
