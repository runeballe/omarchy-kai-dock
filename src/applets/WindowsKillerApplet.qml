import QtQuick

// WindowsKiller: left click, then click a misbehaving window to kill it;
// middle click, then click a window to see its pid, command, uptime and
// class. Clicking the dock or pressing Escape cancels.
Item {
    id: applet
    property string mode: "dock"
    property var app
    readonly property bool desklet: mode === "desklet"
    readonly property size deskletSize: Qt.size(260, 150)
    readonly property bool picking: windowKiller.picking

    readonly property string title: picking ? qsTr("Click the window to kill — click the dock or press Esc to cancel")
        : windowKiller.status !== "" ? windowKiller.status
        : qsTr("WindowsKiller — click, then pick a window to kill")

    function activate(button) {
        windowKiller.pick(button === Qt.MiddleButton ? "info" : "kill")
    }
    function menuEntries() {
        return [
            { text: qsTr("Kill a window…"), action: () => windowKiller.pick("kill") },
            { text: qsTr("Window info…"), action: () => windowKiller.pick("info") }
        ]
    }

    AppletTile {
        visible: !applet.desklet
        app: applet.app
        tint: applet.picking ? "#b3262a" : "#3a1416"
        Behavior on tint { ColorAnimation { duration: 150 } }
    }

    // A crosshair over a skull-ish red target.
    Item {
        id: glyph
        width: applet.desklet ? 64 : applet.width * 0.66
        height: width
        anchors.horizontalCenter: parent.horizontalCenter
        y: applet.desklet ? 16 : (applet.height - height) / 2
        scale: applet.picking ? pulse.value : 1

        QtObject { id: pulse; property real value: 1 }
        SequentialAnimation {
            running: applet.picking
            loops: Animation.Infinite
            NumberAnimation { target: pulse; property: "value"; to: 1.12; duration: 380; easing.type: Easing.InOutSine }
            NumberAnimation { target: pulse; property: "value"; to: 1.0; duration: 380; easing.type: Easing.InOutSine }
            onStopped: pulse.value = 1
        }

        Rectangle {
            anchors.fill: parent
            radius: width / 2
            color: "transparent"
            border.width: Math.max(2, width * 0.09)
            border.color: "#ff5a5f"
        }
        Rectangle {
            anchors.centerIn: parent
            width: parent.width * 0.42
            height: width
            radius: width / 2
            color: "transparent"
            border.width: Math.max(1.5, parent.width * 0.07)
            border.color: "white"
        }
        Repeater {
            model: 4
            Rectangle {
                required property int index
                readonly property bool horizontal: index % 2 === 0
                readonly property real len: glyph.width * 0.28
                width: horizontal ? len : Math.max(2, glyph.width * 0.07)
                height: horizontal ? Math.max(2, glyph.width * 0.07) : len
                radius: Math.min(width, height) / 2
                color: "white"
                x: index === 0 ? -glyph.width * 0.06 : index === 2 ? glyph.width - len + glyph.width * 0.06 : (glyph.width - width) / 2
                y: index === 1 ? -glyph.width * 0.06 : index === 3 ? glyph.width - len + glyph.width * 0.06 : (glyph.height - height) / 2
            }
        }
        Rectangle {
            anchors.centerIn: parent
            width: parent.width * 0.12
            height: width
            radius: width / 2
            color: "#ff5a5f"
        }
    }

    // The desklet shows the last window it looked at.
    Column {
        visible: applet.desklet
        anchors.top: glyph.bottom
        anchors.topMargin: 10
        anchors.horizontalCenter: parent.horizontalCenter
        width: parent.width - 24
        spacing: 2
        readonly property color ink: applet.app ? applet.app.color(config.theme.textColor, "#f2f2f2") : "#f2f2f2"
        Text {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideRight
            text: applet.picking ? qsTr("Pick a window…") : windowKiller.status || qsTr("Left click: kill · Middle click: info")
            color: parent.ink
            font.pixelSize: 13
        }
        Text {
            width: parent.width
            visible: !applet.picking && windowKiller.last.command !== undefined
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideMiddle
            text: windowKiller.last.command || ""
            color: parent.ink
            opacity: 0.6
            font.pixelSize: 11
        }
    }
}
