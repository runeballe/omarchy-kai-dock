import QtQuick

// The rounded square an applet draws on in the dock, so applets sit among the
// app icons like icons themselves.
Rectangle {
    property var app
    property color tint: "#1c1c22"
    anchors.fill: parent
    anchors.margins: width * 0.06
    radius: width * 0.22
    gradient: Gradient {
        GradientStop { position: 0; color: Qt.lighter(tint, 1.35) }
        GradientStop { position: 1; color: tint }
    }
    border.width: Math.max(1, width / 64)
    border.color: Qt.rgba(1, 1, 1, 0.12)
}
