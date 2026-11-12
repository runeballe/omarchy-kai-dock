import QtQuick

// Battery charge, or a plug when the machine has no battery.
Item {
    id: applet
    property string mode: "dock"
    property var app
    readonly property bool desklet: mode === "desklet"
    readonly property size deskletSize: Qt.size(180, 120)
    readonly property real level: Math.max(0, system.battery)
    readonly property color tint: !system.hasBattery ? "#8a8a95" : system.charging ? "#4cd964" : level < 0.15 ? "#ff4d4f" : level < 0.3 ? "#ffb020" : "#e8e8ee"

    readonly property string title: !system.hasBattery ? qsTr("No battery — on mains power")
        : system.charging ? qsTr("Battery %1% · charging").arg(Math.round(level * 100))
        : qsTr("Battery %1%").arg(Math.round(level * 100))

    function activate(button) {}

    AppletTile { visible: !applet.desklet; app: applet.app; tint: "#1a1a20" }

    Item {
        id: body
        width: applet.desklet ? 110 : applet.width * 0.62
        height: width * 0.5
        anchors.centerIn: parent
        anchors.verticalCenterOffset: applet.desklet ? -12 : -applet.height * 0.06
        Rectangle {
            anchors.fill: parent
            anchors.rightMargin: parent.width * 0.08
            radius: height * 0.18
            color: "transparent"
            border.width: Math.max(2, parent.height * 0.08)
            border.color: applet.tint
            Rectangle {
                x: parent.border.width * 2
                y: parent.border.width * 2
                height: parent.height - parent.border.width * 4
                width: (parent.width - parent.border.width * 4) * (system.hasBattery ? applet.level : 1)
                radius: height * 0.12
                color: applet.tint
                opacity: system.hasBattery ? 1 : 0.25
            }
        }
        Rectangle {
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            width: parent.width * 0.07
            height: parent.height * 0.4
            radius: width / 3
            color: applet.tint
        }
        Text {
            visible: system.charging || !system.hasBattery
            anchors.centerIn: parent
            anchors.horizontalCenterOffset: -parent.width * 0.04
            text: "ϟ"
            color: "white"
            style: Text.Outline
            styleColor: "#202020"
            font.pixelSize: parent.height * 0.8
            font.bold: true
        }
    }
    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: body.bottom
        anchors.topMargin: applet.desklet ? 8 : applet.height * 0.03
        text: system.hasBattery ? Math.round(applet.level * 100) + "%" : qsTr("AC")
        color: "white"
        font.pixelSize: applet.desklet ? 20 : applet.width * 0.18
        font.weight: Font.DemiBold
    }
}
