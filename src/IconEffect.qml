import QtQuick

// Transition effects for an icon leaving or joining the dock: evaporate,
// fade, explode, break and black hole. The icon is cut into tiles, and each
// effect moves the tiles by how far the transition has come; joining plays
// the same motion backwards, so exploded pieces fly back together.
Item {
    id: fx
    property url source
    property string effect: "fade"
    property bool appearing: false
    property real duration: 700
    // 0 = whole icon, 1 = gone.
    property real t: 0
    readonly property real p: appearing ? 1 - t : t
    signal finished()

    readonly property int grid: effect === "evaporate" ? 8 : effect === "explode" ? 6
                              : effect === "break" ? 4 : effect === "blackhole" ? 7 : 1
    readonly property real tw: width / grid
    readonly property real th: height / grid

    NumberAnimation on t {
        id: run
        from: 0; to: 1
        duration: fx.duration
        easing.type: Easing.Linear
        running: false
        onFinished: fx.finished()
    }
    function start() { run.start() }

    // A stable pseudo-random number in [0, 1) per tile and purpose.
    function rnd(i, k) {
        const x = Math.sin(i * 12.9898 + k * 78.233) * 43758.5453
        return x - Math.floor(x)
    }
    function clamp01(v) { return Math.max(0, Math.min(1, v)) }

    // The black hole itself, under the pieces it swallows.
    Rectangle {
        visible: fx.effect === "blackhole"
        anchors.centerIn: parent
        readonly property real k: Math.sin(Math.PI * fx.p)
        width: fx.width * 0.55 * k
        height: width
        radius: width / 2
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#000000" }
            GradientStop { position: 0.7; color: "#1a0d2e" }
            GradientStop { position: 1.0; color: "#7b4dff" }
        }
        opacity: 0.85 * k
        rotation: fx.p * 720
    }

    // The flash at the start of an explosion.
    Rectangle {
        visible: fx.effect === "explode"
        anchors.centerIn: parent
        readonly property real k: fx.clamp01(1 - fx.p * 4)
        width: fx.width * (0.4 + fx.p * 2.4)
        height: width
        radius: width / 2
        color: "#fff3c4"
        opacity: fx.p > 0 ? 0.7 * k : 0
    }

    Image {
        anchors.fill: parent
        visible: fx.p <= 0.001
        source: fx.source
        sourceSize: Qt.size(fx.width * 2, fx.height * 2)
        smooth: true
    }

    Repeater {
        model: fx.grid * fx.grid
        Item {
            id: tile
            required property int index
            readonly property int col: index % fx.grid
            readonly property int row: Math.floor(index / fx.grid)
            // The tile's centre relative to the icon's centre, in -0.5..0.5.
            readonly property real ux: (col + 0.5) / fx.grid - 0.5
            readonly property real uy: (row + 0.5) / fx.grid - 0.5
            readonly property real r1: fx.rnd(index, 1)
            readonly property real r2: fx.rnd(index, 2)
            readonly property real r3: fx.rnd(index, 3)

            // Each effect gives the tile an offset (dx, dy in icon sizes), a
            // rotation, a scale and an opacity for the current progress.
            readonly property var m: {
                const p = fx.p
                switch (fx.effect) {
                case "evaporate": {
                    // Rising from the top down, drifting and puffing up.
                    const q = fx.clamp01((p - (row / fx.grid) * 0.35 - r1 * 0.25) / 0.45)
                    return { dx: Math.sin((q + r2) * 6.28) * 0.08 * q + (r3 - 0.5) * 0.2 * q,
                             dy: -q * (0.6 + r1 * 0.6), rot: (r2 - 0.5) * 60 * q,
                             s: 1 + q * 1.2, o: 1 - q, puff: q }
                }
                case "explode": {
                    const q = fx.clamp01((p - r1 * 0.1) / 0.9)
                    const len = Math.hypot(ux, uy) || 0.01
                    const dist = (1.2 + r2 * 1.4) * Math.sqrt(q)
                    return { dx: ux / len * dist * (len + 0.15) + (r3 - 0.5) * 0.3 * q,
                             dy: uy / len * dist * (len + 0.15) + (r1 - 0.5) * 0.3 * q,
                             rot: (r3 - 0.5) * 720 * q, s: 1 - q * 0.4, o: 1 - q * q, puff: 0 }
                }
                case "break": {
                    // Cracks first, then the shards fall away one by one.
                    const crack = fx.clamp01(p / 0.15)
                    const q = fx.clamp01((p - 0.15 - r1 * 0.3) / 0.55)
                    return { dx: ux * 0.06 * crack + (r2 - 0.5) * 0.4 * q,
                             dy: uy * 0.06 * crack + q * q * (1.6 + r3),
                             rot: (r2 - 0.5) * 30 * crack + (r3 - 0.5) * 200 * q,
                             s: 1, o: 1 - fx.clamp01((q - 0.6) / 0.4), puff: 0 }
                }
                case "blackhole": {
                    // Outer pieces go last; everything spirals into the middle.
                    const dist = Math.hypot(ux, uy)
                    const q = fx.clamp01((p - (0.7 - dist) * 0.35 - r1 * 0.1) / 0.65)
                    const a = q * Math.PI * 2.2
                    const k = 1 - q
                    const x = (ux * Math.cos(a) - uy * Math.sin(a)) * k
                    const y = (ux * Math.sin(a) + uy * Math.cos(a)) * k
                    return { dx: x - ux, dy: y - uy, rot: q * 540, s: 1 - q, o: 1 - q * q * q, puff: 0 }
                }
                default: // fade
                    return { dx: 0, dy: 0, rot: 0, s: 1 - p * 0.25, o: 1 - p, puff: 0 }
                }
            }

            // Tiles overlap by a pixel so no seams show between them, and the
            // whole icon stands in for them before anything has moved.
            visible: fx.p > 0.001
            width: fx.tw + 1
            height: fx.th + 1
            x: col * fx.tw + m.dx * fx.width
            y: row * fx.th + m.dy * fx.height
            rotation: m.rot
            scale: m.s
            opacity: m.o
            clip: true

            Image {
                x: -tile.col * fx.tw
                y: -tile.row * fx.th
                width: fx.width
                height: fx.height
                source: fx.source
                sourceSize: Qt.size(fx.width * 2, fx.height * 2)
                smooth: true
            }
            // Evaporating pieces turn into soft grey smoke as they rise.
            Rectangle {
                visible: fx.effect === "evaporate"
                anchors.centerIn: parent
                width: parent.width * 1.4
                height: width
                radius: width / 2
                color: "#d8dbe4"
                opacity: 0.8 * tile.m.puff
            }
        }
    }
}
