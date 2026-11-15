import QtQuick

// The trash: shows whether anything is in it and opens it.
Item {
    id: applet
    property string mode: "dock"
    property var app
    readonly property bool desklet: mode === "desklet"
    readonly property size deskletSize: Qt.size(150, 160)
    readonly property bool full: system.trashCount > 0

    readonly property string title: full ? qsTr("Trash — %n item(s)", "", system.trashCount) : qsTr("Trash — empty")

    function activate(button) {
        if (shell.hasCommand("nautilus")) shell.run("nautilus trash:///")
        else shell.openUrl("trash:///")
    }

    Image {
        anchors.centerIn: parent
        anchors.verticalCenterOffset: applet.desklet ? -10 : 0
        width: applet.desklet ? 100 : applet.width
        height: width
        sourceSize: Qt.size(width * 2, height * 2)
        source: "image://icon/" + (applet.full ? "user-trash-full" : "user-trash")
    }
    Text {
        visible: applet.desklet
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 10
        text: applet.title
        color: app ? app.color(config.theme.textColor, "#f2f2f2") : "#f2f2f2"
        font.pixelSize: 13
    }
}
