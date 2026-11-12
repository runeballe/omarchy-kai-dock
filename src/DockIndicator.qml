import QtQuick

// Shows that an app is running: dots for each window (up to three), a bar, or
// a soft glow, brighter for the focused app.
Item {
    id: ind
    required property var dock
    required property var app
    property int count: 1
    property bool active: false

    readonly property var t: dock.t
    readonly property color tint: app.color(active ? t.activeColor : t.indicatorColor, app.accent)
    readonly property real unit: Math.max(3, Math.round(dock.s / 12))
    readonly property string style: t.indicator

    width: style === "dot" ? row.width : style === "bar" ? dock.s * (active ? 0.6 : 0.35) : dock.s * 0.9
    height: style === "glow" ? unit * 2.4 : unit
    Behavior on width { NumberAnimation { duration: 160 } }

    Row {
        id: row
        visible: ind.style === "dot"
        spacing: ind.unit * 0.8
        Repeater {
            model: Math.min(3, Math.max(1, ind.count))
            Rectangle {
                width: ind.unit
                height: ind.unit
                radius: width / 2
                color: ind.tint
                opacity: ind.active ? 1 : 0.7
            }
        }
    }

    Rectangle {
        visible: ind.style === "bar"
        anchors.fill: parent
        radius: height / 2
        color: ind.tint
        opacity: ind.active ? 1 : 0.6
    }

    Item {
        visible: ind.style === "glow"
        anchors.fill: parent
        Repeater {
            model: 3
            Rectangle {
                required property int index
                anchors.centerIn: parent
                width: parent.width * (1 - index * 0.28)
                height: parent.height * (1 - index * 0.25)
                radius: height / 2
                color: ind.tint
                opacity: (ind.active ? 0.32 : 0.18) + index * 0.18
            }
        }
    }
}
