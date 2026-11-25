import QtQuick
import QtQuick.Effects

// The speaker volume: scroll to change it, click to mute, double click to
// open the mixer.
Item {
    id: applet
    property string mode: "dock"
    property var app
    readonly property bool desklet: mode === "desklet"
    readonly property size deskletSize: Qt.size(170, 150)
    readonly property int percent: Math.round(audio.volume * 100)
    readonly property string quickInfo: audio.available ? (audio.muted ? qsTr("Mute") : percent + "%") : ""

    readonly property string title: !audio.available ? qsTr("Volume — wpctl (PipeWire) not found")
        : audio.muted ? qsTr("Volume: muted") : qsTr("Volume: %1%").arg(percent)

    function activate(button) {
        if (button === Qt.MiddleButton) openMixer()
        else audio.toggleMute()
    }
    function doubleClick() { openMixer() }
    function scroll(direction) { audio.step(direction > 0 ? 5 : -5) }
    function openMixer() {
        if (shell.hasCommand("wiremix")) shell.run("xdg-terminal-exec wiremix")
        else if (shell.hasCommand("pavucontrol")) shell.run("pavucontrol")
    }
    function menuEntries() {
        return [
            { text: audio.muted ? qsTr("Unmute") : qsTr("Mute"), action: () => audio.toggleMute() },
            { text: qsTr("Open mixer"), action: () => applet.openMixer() }
        ]
    }

    // A speaker with sound waves; the waves follow the level.
    Item {
        id: speaker
        width: applet.desklet ? 72 : applet.width * 0.9
        height: width
        anchors.horizontalCenter: parent.horizontalCenter
        y: applet.desklet ? 14 : (applet.height - height) / 2
        // No tile behind it, so it takes the theme's text color (dark on light
        // themes) and a soft shadow to stand out on any shelf.
        readonly property color ink: audio.muted ? "#8a8a95"
            : applet.app ? applet.app.color(config.theme.textColor, "#f2f2f2") : "#f2f2f2"
        layer.enabled: applet.app && applet.app.fancy
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowOpacity: 0.45
            shadowBlur: 0.5
            shadowVerticalOffset: 1
            autoPaddingEnabled: true
        }

        Canvas {
            anchors.fill: parent
            property color ink: speaker.ink
            property real level: audio.muted ? 0 : audio.volume
            onInkChanged: requestPaint()
            onLevelChanged: requestPaint()
            onPaint: {
                const c = getContext("2d"), w = width, h = height
                c.reset()
                c.fillStyle = ink; c.strokeStyle = ink
                c.fillRect(w * 0.08, h * 0.36, w * 0.18, h * 0.28)
                c.beginPath(); c.moveTo(w * 0.24, h * 0.36); c.lineTo(w * 0.46, h * 0.16)
                c.lineTo(w * 0.46, h * 0.84); c.lineTo(w * 0.24, h * 0.64); c.closePath(); c.fill()
                c.lineWidth = Math.max(1.5, w * 0.06); c.lineCap = "round"
                for (let i = 0; i < 3; ++i) {
                    if (level <= i / 3) break
                    c.beginPath()
                    c.arc(w * 0.46, h * 0.5, w * (0.14 + i * 0.13), -Math.PI / 3.2, Math.PI / 3.2)
                    c.stroke()
                }
            }
        }
        Text {
            visible: audio.muted
            text: "✕"
            color: "#ff6b6b"
            font.pixelSize: speaker.width * 0.34
            font.bold: true
            x: speaker.width * 0.58; y: speaker.height * 0.26
        }
    }

    Column {
        visible: applet.desklet
        anchors.top: speaker.bottom
        anchors.topMargin: 8
        anchors.horizontalCenter: parent.horizontalCenter
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: applet.quickInfo
            color: applet.app ? applet.app.color(config.theme.textColor, "#f2f2f2") : "#f2f2f2"
            font.pixelSize: 18
            font.weight: Font.DemiBold
        }
    }
}
