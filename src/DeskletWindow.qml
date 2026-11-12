import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Controls.Material

// An applet detached from the dock onto the desktop. Drag it anywhere; hover
// for buttons to put it back in the dock or hide it.
Window {
    id: desklet
    required property string appletId
    required property var app

    readonly property var t: config.theme
    readonly property var info: {
        for (const a of config.values.applets)
            if (a.id === appletId) return a
        return ({ x: 80, y: 80 })
    }
    property int posX: info.x !== undefined ? info.x : 80
    property int posY: info.y !== undefined ? info.y : 80
    readonly property size wanted: view.item ? view.item.deskletSize : Qt.size(200, 200)

    width: wanted.width
    height: wanted.height
    color: "transparent"
    flags: Qt.FramelessWindowHint | Qt.WindowDoesNotAcceptFocus
    title: "kai desklet"
    visible: false
    Material.theme: Material.Dark
    Material.accent: app.accent

    Component.onCompleted: {
        shell.setupDesklet(desklet, posX, posY, config.values.deskletLayer, config.values.screen)
        visible = true
    }
    Connections {
        target: config
        function onChanged() { shell.setDeskletLayer(desklet, config.values.deskletLayer) }
    }

    Rectangle {
        id: card
        anchors.fill: parent
        radius: Math.min(app.radius + 6, 22)
        readonly property real alpha: desklet.t.deskletOpacity >= 0 ? desklet.t.deskletOpacity : Math.max(0.55, desklet.t.backgroundOpacity)
        gradient: Gradient {
            GradientStop { position: 0; color: app.withAlpha(app.color(desklet.t.deskletBackground || desklet.t.background2 || desklet.t.background), card.alpha) }
            GradientStop { position: 1; color: app.withAlpha(app.color(desklet.t.deskletBackground || desklet.t.background), card.alpha) }
        }
        border.width: Math.max(1, desklet.t.borderWidth)
        border.color: app.withAlpha(app.color(desklet.t.border, "#ffffff"), Math.max(0.12, desklet.t.borderOpacity))
    }

    AppletView {
        id: view
        anchors.fill: parent
        appletId: desklet.appletId
        mode: "desklet"
        app: desklet.app
    }

    MouseArea {
        id: drag
        anchors.fill: parent
        hoverEnabled: true
        preventStealing: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        property point origin
        property bool moved: false
        cursorShape: pressed && moved ? Qt.ClosedHandCursor : Qt.ArrowCursor

        onPressed: (mouse) => {
            origin = Qt.point(mouse.x, mouse.y)
            moved = false
        }
        // The surface moves under the pointer, so each event's offset from
        // the press point is how much further to move it.
        onPositionChanged: (mouse) => {
            if (!pressed || !(pressedButtons & Qt.LeftButton)) return
            const dx = mouse.x - origin.x, dy = mouse.y - origin.y
            if (!moved && Math.hypot(dx, dy) < 4) return
            moved = true
            const scr = desklet.screen
            desklet.posX = Math.max(0, Math.min(desklet.posX + dx, (scr ? scr.width : 3840) - desklet.width))
            desklet.posY = Math.max(0, Math.min(desklet.posY + dy, (scr ? scr.height : 2160) - desklet.height))
            shell.moveDesklet(desklet, desklet.posX, desklet.posY)
            if (!shell.layerShell) origin = Qt.point(mouse.x - dx, mouse.y - dy)
        }
        onReleased: {
            if (moved) {
                config.setApplet(desklet.appletId, "x", desklet.posX)
                config.setApplet(desklet.appletId, "y", desklet.posY)
            }
        }
        onClicked: (mouse) => {
            if (moved) return
            if (mouse.button === Qt.RightButton) app.openSettings("applets")
            else view.activate(mouse.button)
        }
    }

    Row {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 6
        spacing: 4
        opacity: drag.containsMouse || attach.hovered || hide.hovered ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: 150 } }

        component Chip: AbstractButton {
            id: chip
            property string glyph
            width: 24
            height: 24
            hoverEnabled: true
            background: Rectangle {
                radius: Math.min(4, app.radius + 4)
                color: chip.hovered ? Qt.rgba(1, 1, 1, 0.22) : Qt.rgba(0, 0, 0, 0.45)
            }
            contentItem: Text {
                text: chip.glyph
                color: "white"
                font.pixelSize: 13
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            ToolTip.visible: hovered
            ToolTip.delay: 400
        }
        Chip {
            id: attach
            glyph: "⤓"
            ToolTip.text: qsTr("Put back in the dock")
            onClicked: config.setApplet(desklet.appletId, "detached", false)
        }
        Chip {
            id: hide
            glyph: "✕"
            ToolTip.text: qsTr("Hide")
            onClicked: config.setApplet(desklet.appletId, "enabled", false)
        }
    }
}
