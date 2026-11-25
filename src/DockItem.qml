import QtQuick
import QtQuick.Effects

// One cell of the dock: an app icon, a separator, or a docked applet, with
// its zoom, reflection, running indicator and animations. Frame coordinates:
// the screen edge is at the bottom.
Item {
    id: cell
    required property var dock
    required property var app
    required property int index
    required property string kind
    required property string appId
    required property string name
    required property string icon
    required property string appletId
    required property bool pinned
    required property bool running
    required property int windowCount
    required property bool active
    required property bool urgent
    required property bool launching

    readonly property var g: dock.geom
    // A row can exist for a moment before the layout has a slot for it (or
    // after it's gone), so check every array the cell reads from.
    readonly property bool valid: index >= 0 && index < g.u.length && index < g.w.length
                                  && index < g.scale.length
    readonly property real sc: valid ? (g.scale[index] ?? 1) : 1
    readonly property real s: dock.s
    readonly property var t: dock.t
    // A docked applet may ask for a cell wider than an icon (the digital
    // clock). Along a side edge the cells stack, so they stay icon-sized.
    readonly property real appletFactor: kind === "applet" && appletView.item && appletView.item.dockWidthFactor
        ? appletView.item.dockWidthFactor : 1
    readonly property real widthFactor: dock.vertical ? 1 : appletFactor
    onWidthFactorChanged: if (dock.layoutRev !== undefined) dock.layoutRev++

    x: valid ? (dock.frameX(g.u[index], g.w[index]) ?? 0) : 0
    width: valid ? g.w[index] : 0
    height: parent ? parent.height : 0

    // Animated values; `bounce` lifts the icon away from the edge.
    property real bounce: 0
    property real pulse: 1
    property real flip: 0
    property real wobble: 0
    property real blink: 1
    property real spin: 0
    property real wave: 0
    opacity: dock.dragging && dock.dragIndex === index ? 0.35 : 1

    readonly property real iconY: height - dock.iconBase - s

    // --- Separator -----------------------------------------------------------
    Rectangle {
        visible: cell.kind === "separator" && cell.t.separator !== "gap"
        width: Math.max(1, Math.round(cell.s / 40))
        height: dock.plane ? dock.shelf * 0.7 : cell.s * 0.7
        x: (cell.width - width) / 2
        y: dock.plane ? cell.height - dock.gap - dock.shelf * 0.85 : cell.iconY + cell.s * 0.15
        radius: width / 2
        color: app.withAlpha(app.color(cell.t.textColor, "#ffffff"), 0.28)
    }

    // --- Icon ----------------------------------------------------------------
    Item {
        id: lift
        visible: cell.kind !== "separator"
        width: cell.s
        height: cell.s
        x: (cell.width - width) / 2
        y: cell.iconY - cell.bounce
        transformOrigin: Item.Bottom
        scale: cell.sc * cell.pulse

        Item {
            id: upright
            anchors.fill: parent
            rotation: -dock.frameRotation
            opacity: cell.blink

            Item {
                id: visual
                anchors.fill: parent
                // Hidden while the appear effect assembles the icon.
                visible: !appearFx.active
                transform: [
                    Rotation { origin.x: visual.width / 2; origin.y: visual.height / 2; axis { x: 0; y: 1; z: 0 } angle: cell.flip },
                    Rotation { origin.x: visual.width / 2; origin.y: visual.height; angle: cell.wobble },
                    Rotation { origin.x: visual.width / 2; origin.y: visual.height / 2; angle: cell.spin },
                    // Without the GPU the wave is a soft squash and stretch.
                    Scale {
                        readonly property real k: app.fancy ? 0 : 0.14 * Math.sin(Math.PI * cell.wave) * Math.sin(6 * Math.PI * cell.wave)
                        origin.x: visual.width / 2; origin.y: visual.height
                        xScale: 1 + k; yScale: 1 - k
                    }
                ]
                // With the GPU the icon itself ripples (shaders/wave.vert).
                layer.enabled: app.fancy && cell.wave > 0
                layer.smooth: true
                layer.effect: ShaderEffect {
                    property real phase: cell.wave
                    property real amplitude: 0.07
                    property real w: width
                    property real h: height
                    mesh: GridMesh { resolution: Qt.size(16, 16) }
                    vertexShader: "qrc:/shaders/wave.vert.qsb"
                    fragmentShader: "qrc:/shaders/wave.frag.qsb"
                }

                Image {
                    id: image
                    visible: cell.kind === "app"
                    anchors.fill: parent
                    source: cell.kind === "app" ? "image://icon/" + encodeURIComponent(cell.icon) : ""
                    sourceSize: Qt.size(cell.s * dock.zoomFactor * Screen.devicePixelRatio, cell.s * dock.zoomFactor * Screen.devicePixelRatio)
                    smooth: true
                    mipmap: true
                    asynchronous: false
                    layer.enabled: app.fancy && cell.t.iconShadow > 0
                    layer.effect: MultiEffect {
                        shadowEnabled: true
                        shadowOpacity: cell.t.iconShadow
                        shadowBlur: 0.6
                        shadowVerticalOffset: 2
                        shadowColor: "black"
                        autoPaddingEnabled: true
                    }
                }

                // Applets draw at full zoom size and are scaled down, so they
                // stay sharp when the dock magnifies them.
                Item {
                    visible: cell.kind === "applet"
                    x: (cell.s - cell.s * cell.appletFactor) / 2
                    width: cell.s * dock.zoomFactor * cell.appletFactor
                    height: cell.s * dock.zoomFactor
                    scale: 1 / dock.zoomFactor
                    transformOrigin: Item.TopLeft
                    AppletView {
                        id: appletView
                        anchors.fill: parent
                        appletId: cell.kind === "applet" ? cell.appletId : ""
                        mode: "dock"
                        app: cell.app
                        dock: cell.dock
                    }
                }
            }

            Loader {
                id: appearFx
                anchors.fill: parent
                active: false
                sourceComponent: IconEffect {
                    source: cell.iconSource
                    effect: dock.v.appearEffect
                    appearing: true
                    duration: 650 * cell.d
                    onFinished: appearFx.active = false
                    Component.onCompleted: start()
                }
            }

            // Quick-info in the top corner, progress along the foot; drawn
            // outside the animated visual so they stay readable while it moves.
            Rectangle {
                id: infoPill
                visible: cell.badge.info !== ""
                readonly property real h: Math.max(10, cell.s * 0.3)
                height: h
                width: Math.max(h, infoText.implicitWidth + h * 0.55)
                radius: h / 2
                x: Math.min(upright.width - width * 0.6, upright.width - width + h * 0.25)
                y: -h * 0.2
                color: app.accent
                border.width: 1
                border.color: Qt.rgba(0, 0, 0, 0.35)
                Text {
                    id: infoText
                    anchors.centerIn: parent
                    text: cell.badge.info
                    color: omarchy.accentForeground
                    font.pixelSize: Math.max(8, Math.round(infoPill.h * 0.68))
                    font.weight: Font.Bold
                }
            }
            Rectangle {
                visible: cell.badge.progress >= 0
                x: upright.width * 0.1
                width: upright.width * 0.8
                height: Math.max(3, cell.s * 0.09)
                y: upright.height - height - upright.height * 0.04
                radius: height / 2
                color: Qt.rgba(0, 0, 0, 0.55)
                border.width: 1
                border.color: Qt.rgba(1, 1, 1, 0.25)
                Rectangle {
                    x: 1; y: 1
                    height: parent.height - 2
                    width: Math.max(height, (parent.width - 2) * cell.badge.progress)
                    radius: height / 2
                    color: app.accent
                    Behavior on width { NumberAnimation { duration: 200 } }
                }
            }
        }

        // A ghost that grows and fades for the pulse animation.
        Image {
            id: ghost
            anchors.fill: parent
            source: image.source
            sourceSize: image.sourceSize
            visible: opacity > 0
            opacity: 0
            rotation: -dock.frameRotation
            property real grow: 1
            scale: grow
        }
    }

    // --- Reflection ------------------------------------------------------------
    // A mirror image of the icon below its foot, cut off at the shelf's edge
    // and faded toward it.
    Item {
        id: reflectionClip
        visible: cell.t.reflection > 0 && !dock.panel && cell.kind !== "separator"
        x: 0
        y: cell.height - dock.iconBase
        width: cell.width
        height: Math.max(0, dock.iconBase - dock.gap)
        clip: true

        Item {
            id: reflectionBody
            anchors.fill: parent
            layer.enabled: app.fancy
            layer.effect: MultiEffect {
                maskEnabled: true
                maskSource: fadeMask
                maskThresholdMin: 0.0
                maskSpreadAtMin: 1.0
            }
            opacity: cell.t.reflection * (app.fancy ? 1 : 0.6)

            Item {
                width: cell.s
                height: cell.s
                x: (cell.width - width) / 2
                y: cell.bounce
                transformOrigin: Item.Top
                scale: cell.sc * cell.pulse
                Item {
                    anchors.fill: parent
                    transform: [
                        Rotation { origin.x: cell.s / 2; origin.y: cell.s / 2; angle: -dock.frameRotation },
                        Scale { origin.y: cell.s / 2; yScale: -1 }
                    ]
                    Image {
                        anchors.fill: parent
                        visible: cell.kind === "app"
                        source: image.source
                        sourceSize: image.sourceSize
                        smooth: true
                    }
                    // Applets reflect as a soft block of their tint.
                    Rectangle {
                        anchors.fill: parent
                        visible: cell.kind === "applet"
                        radius: Math.min(app.radius + 4, width / 4)
                        color: app.withAlpha(app.accent, 0.35)
                    }
                }
            }
        }
        Rectangle {
            id: fadeMask
            anchors.fill: parent
            visible: false
            layer.enabled: true
            gradient: Gradient {
                GradientStop { position: 0; color: Qt.rgba(1, 1, 1, 0.9) }
                GradientStop { position: 1; color: Qt.rgba(1, 1, 1, 0) }
            }
        }
    }

    // --- Indicator -------------------------------------------------------------
    DockIndicator {
        visible: cell.kind === "app" && cell.running && cell.t.indicator !== "none"
        dock: cell.dock
        app: cell.app
        count: cell.windowCount
        active: cell.active
        x: (cell.width - width) / 2
        y: cell.height - dock.gap - height - (dock.panel ? 1 : Math.max(1, dock.pad * 0.25))
    }

    // Attention: an urgent window keeps the icon moving until it's seen.
    Rectangle {
        visible: cell.urgent && cell.badge.info === ""
        width: cell.s * 0.22
        height: width
        radius: width / 2
        color: "#ff4d4f"
        border.color: "white"
        border.width: 1
        x: lift.x + cell.s * cell.sc * 0.5 + cell.s * 0.15
        y: cell.iconY - cell.bounce - cell.s * (cell.sc - 1) + 2
    }

    // --- Animations --------------------------------------------------------------
    readonly property real d: 1 / dock.speed

    // Quick-info and progress: from the app (Unity LauncherEntry), kai's API,
    // or the applet itself.
    readonly property var badge: {
        const e = badges.entries[kind === "applet" ? "applet:" + appletId : appId] || {}
        const a = kind === "applet" ? appletView.item : null
        const info = e.info || (a && a.quickInfo ? a.quickInfo : "")
        const progress = e.progress !== undefined && e.progress >= 0 ? e.progress
                       : a && a.progress !== undefined && a.progress >= 0 ? a.progress : -1
        return { info: info, progress: progress, urgent: !!e.urgent }
    }

    SequentialAnimation {
        id: bounceAnim
        NumberAnimation { target: cell; property: "bounce"; to: cell.s * 0.5; duration: 230 * cell.d; easing.type: Easing.OutQuad }
        NumberAnimation { target: cell; property: "bounce"; to: 0; duration: 380 * cell.d; easing.type: Easing.OutBounce }
    }
    SequentialAnimation {
        id: pulseAnim
        ParallelAnimation {
            NumberAnimation { target: cell; property: "pulse"; to: 1.18; duration: 150 * cell.d; easing.type: Easing.OutQuad }
            NumberAnimation { target: ghost; property: "opacity"; from: 0.6; to: 0; duration: 500 * cell.d }
            NumberAnimation { target: ghost; property: "grow"; from: 1; to: 1.8; duration: 500 * cell.d; easing.type: Easing.OutQuad }
        }
        NumberAnimation { target: cell; property: "pulse"; to: 1; duration: 250 * cell.d; easing.type: Easing.OutBack }
    }
    NumberAnimation {
        id: spinAnim
        target: cell; property: "spin"; from: 0; to: 360; duration: 800 * cell.d; easing.type: Easing.InOutCubic
    }
    NumberAnimation {
        id: waveAnim
        target: cell; property: "wave"; from: 0; to: 1; duration: 900 * cell.d; easing.type: Easing.Linear
    }
    NumberAnimation {
        id: flipAnim
        target: cell; property: "flip"; from: 0; to: 360; duration: 700 * cell.d; easing.type: Easing.InOutCubic
    }
    SequentialAnimation {
        id: wobbleAnim
        NumberAnimation { target: cell; property: "wobble"; to: -14; duration: 70 * cell.d }
        NumberAnimation { target: cell; property: "wobble"; to: 12; duration: 110 * cell.d }
        NumberAnimation { target: cell; property: "wobble"; to: -8; duration: 100 * cell.d }
        NumberAnimation { target: cell; property: "wobble"; to: 5; duration: 90 * cell.d }
        NumberAnimation { target: cell; property: "wobble"; to: 0; duration: 80 * cell.d }
        PauseAnimation { duration: 250 * cell.d }
    }
    SequentialAnimation {
        id: blinkAnim
        NumberAnimation { target: cell; property: "blink"; to: 0.25; duration: 220 * cell.d }
        NumberAnimation { target: cell; property: "blink"; to: 1; duration: 220 * cell.d }
    }

    function animationFor(name) {
        switch (name) {
        case "bounce": return bounceAnim
        case "pulse": return pulseAnim
        case "flip": return flipAnim
        case "wobble": return wobbleAnim
        case "blink": return blinkAnim
        case "rotate": return spinAnim
        case "wave": return waveAnim
        }
        return null
    }
    function play(name, loops) {
        const a = animationFor(name)
        if (!a) return
        a.loops = loops || 1
        a.restart()
    }
    function stopAll() {
        for (const a of [bounceAnim, pulseAnim, flipAnim, wobbleAnim, blinkAnim, spinAnim, waveAnim]) {
            a.loops = 1
            a.complete()
        }
        bounce = 0; pulse = 1; flip = 0; wobble = 0; blink = 1; spin = 0; wave = 0
    }
    function playClick() { play(dock.v.clickAnimation, 1) }

    // Hover runs at most one animation, never on top of a launch/attention loop.
    function playHover() {
        if (loopState !== "") return
        if (bounceAnim.running || pulseAnim.running || flipAnim.running || wobbleAnim.running || blinkAnim.running
            || spinAnim.running || waveAnim.running)
            return
        play(dock.v.hoverAnimation, 1)
    }

    // Looping states: a launch in progress, a window asking for attention.
    readonly property string loopState: launching ? "launch" : (urgent || badge.urgent) ? "attention" : ""
    onLoopStateChanged: {
        stopAll()
        if (loopState === "launch") play(dock.v.launchAnimation, Animation.Infinite)
        else if (loopState === "attention") play(dock.v.attentionAnimation, Animation.Infinite)
    }
    Component.onCompleted: {
        if (loopState !== "") loopStateChanged()
        // Icons that join after start-up arrive with a transition effect.
        if (dock.settled && kind === "app" && dock.v.appearEffect && dock.v.appearEffect !== "none")
            appearFx.active = true
    }

    // The icon's place on screen in frame coordinates, for the effect that
    // plays where it stood once it has been removed.
    function iconRect() {
        const w = s * sc * pulse
        return Qt.rect(x + width / 2 - w / 2, iconY - bounce + s - w, w, w)
    }
    readonly property string iconSource: kind === "app" ? "image://icon/" + encodeURIComponent(icon) : ""

    Connections {
        target: dockModel
        function onLaunched(row) { if (row === cell.index && !cell.launching) cell.play(dock.v.launchAnimation, 1) }
    }

    function activateApplet(button, row) {
        playClick()
        appletView.activate(button, row)
    }
    readonly property var applet: appletView.item
}
