import QtQuick

// The time, as a clock face or digits, with the date in the desklet.
Item {
    id: applet
    property string mode: "dock"
    property var app
    readonly property bool desklet: mode === "desklet"
    readonly property bool analog: config.values.clockAnalog
    readonly property bool seconds: config.values.clockSeconds
    readonly property bool h24: config.values.clock24h
    readonly property size deskletSize: Qt.size(230, analog ? 270 : 150)

    property date now: new Date()
    Timer {
        interval: applet.seconds ? 1000 : 10000
        running: true
        repeat: true
        triggeredOnStart: true
        onTriggered: applet.now = new Date()
    }

    readonly property string timeText: {
        translator.language
        const base = applet.h24 ? "HH:mm" : "h:mm AP"
        return applet.now.toLocaleTimeString(Qt.locale(), applet.seconds ? base + ":ss" : base)
    }
    // The tooltip and desklet heading stay at minute resolution so they don't
    // churn every second.
    readonly property string titleTime: {
        translator.language
        return applet.now.toLocaleTimeString(Qt.locale(), applet.h24 ? "HH:mm" : "h:mm AP")
    }
    readonly property string dateText: {
        translator.language
        return applet.now.toLocaleDateString(Qt.locale(), "dddd d. MMMM")
    }
    readonly property string title: titleTime + " · " + dateText

    function activate(button) {
        if (shell.hasCommand("gnome-calendar")) shell.run("gnome-calendar")
    }

    AppletTile { visible: !applet.desklet && !applet.analog; app: applet.app; tint: "#1a1a20" }

    // Digital.
    Column {
        visible: !applet.analog
        anchors.centerIn: parent
        spacing: 0
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: applet.timeText
            color: "white"
            font.pixelSize: applet.desklet ? 54 : applet.width * 0.27
            font.weight: Font.DemiBold
            font.features: { "tnum": 1 }
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: applet.desklet ? applet.dateText : applet.now.toLocaleDateString(Qt.locale(), "ddd d")
            color: applet.app ? applet.app.accent : "#FFD60A"
            font.pixelSize: applet.desklet ? 16 : applet.width * 0.15
        }
    }

    // Analog.
    Item {
        id: face
        visible: applet.analog
        width: applet.desklet ? 190 : applet.width * 0.9
        height: width
        anchors.horizontalCenter: parent.horizontalCenter
        y: applet.desklet ? 12 : (applet.height - height) / 2
        readonly property real r: width / 2
        readonly property color accent: applet.app ? applet.app.accent : "#FFD60A"

        Rectangle {
            anchors.fill: parent
            radius: width / 2
            gradient: Gradient {
                GradientStop { position: 0; color: "#fbfbfb" }
                GradientStop { position: 1; color: "#d9d9de" }
            }
            border.width: Math.max(1.5, face.r * 0.07)
            border.color: "#2a2a30"
        }
        Repeater {
            model: 12
            Item {
                required property int index
                anchors.fill: parent
                rotation: index * 30
                Rectangle {
                    x: face.r - width / 2
                    y: face.r * 0.14
                    width: index % 3 === 0 ? face.r * 0.06 : face.r * 0.035
                    height: index % 3 === 0 ? face.r * 0.16 : face.r * 0.1
                    radius: width / 2
                    color: "#2a2a30"
                }
            }
        }
        component Hand: Item {
            property real angle: 0
            property real length: 0.6
            property real thickness: 0.06
            property color tint: "#1b1b1f"
            anchors.fill: parent
            rotation: angle
            Behavior on rotation {
                enabled: face.visible
                RotationAnimation { duration: 180; direction: RotationAnimation.Clockwise; easing.type: Easing.OutBack }
            }
            Rectangle {
                width: face.r * parent.thickness
                height: face.r * parent.length
                x: face.r - width / 2
                y: face.r - height + width / 2
                radius: width / 2
                color: parent.tint
                antialiasing: true
            }
        }
        Hand {
            angle: (applet.now.getHours() % 12 + applet.now.getMinutes() / 60) * 30
            length: 0.52
            thickness: 0.09
        }
        Hand {
            angle: (applet.now.getMinutes() + applet.now.getSeconds() / 60) * 6
            length: 0.78
            thickness: 0.065
        }
        Hand {
            visible: applet.seconds
            angle: applet.now.getSeconds() * 6
            length: 0.84
            thickness: 0.025
            tint: face.accent
        }
        Rectangle {
            anchors.centerIn: parent
            width: face.r * 0.12
            height: width
            radius: width / 2
            color: face.accent
            border.color: "#1b1b1f"
            border.width: 1
        }
    }

    Column {
        visible: applet.desklet && applet.analog
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: face.bottom
        anchors.topMargin: 10
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: applet.timeText
            color: "white"
            font.pixelSize: 22
            font.weight: Font.DemiBold
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: applet.dateText
            color: Qt.rgba(1, 1, 1, 0.7)
            font.pixelSize: 13
        }
    }
}
