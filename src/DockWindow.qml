import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Shapes

// The dock: a transparent layer-shell surface along one screen edge. Everything
// is laid out in a "frame" that always has the edge at its bottom; the frame
// is rotated onto the real edge, and icons are rotated back upright. Only the
// visible dock takes the pointer (see updateMask).
Window {
    id: win
    required property var app

    readonly property var t: config.theme
    readonly property var v: config.values
    readonly property string edge: v.edge || "bottom"
    readonly property bool vertical: edge === "left" || edge === "right"
    readonly property bool flipAlong: edge === "top" || edge === "right"
    readonly property real frameRotation: edge === "top" ? 180 : edge === "left" ? 90 : edge === "right" ? -90 : 0
    readonly property string view: v.view !== "" ? v.view : t.view
    readonly property bool plane: view === "plane"
    readonly property bool panel: view === "panel"

    readonly property real s: Math.round(v.iconSize)
    readonly property real zoomFactor: Math.max(1, v.zoom)
    readonly property real zoomRange: Math.max(1, v.zoomRange)
    readonly property real spacing: v.spacing >= 0 ? v.spacing : t.spacing
    readonly property real pad: t.padding
    readonly property real gap: panel ? 0 : v.margin
    readonly property real speed: Math.max(0.2, v.animationSpeed)
    readonly property real reflectRoom: (t.reflection > 0 && !panel && !plane) ? Math.round(s * 0.22) : 0
    // Thickness of the shelf behind the icons, and how far from the screen
    // edge the icons stand.
    readonly property real shelf: plane ? Math.round(s * 0.85) : s + 2 * pad + reflectRoom
    readonly property real iconBase: plane ? gap + Math.round(shelf * 0.32) : gap + pad + reflectRoom
    readonly property real labelRoom: v.showLabels ? (vertical ? 260 : 46) : 10
    readonly property real depth: Math.ceil(iconBase + s * zoomFactor + s * 0.5 + labelRoom)
    property bool menuOpen: false
    readonly property real surfaceDepth: (menuOpen || popupOpen) ? Math.max(depth, vertical ? 420 : 560) : depth
    readonly property real along: vertical ? (screen ? screen.height : 1080) : (screen ? screen.width : 1920)

    width: vertical ? surfaceDepth : along
    height: vertical ? along : surfaceDepth
    color: "transparent"
    flags: Qt.FramelessWindowHint | Qt.WindowDoesNotAcceptFocus
    title: "kai"
    visible: false

    Material.theme: Material.Dark
    Material.accent: app.accent
    Material.roundedScale: app.radius

    readonly property int exclusiveZone: (v.reserveSpace && !v.autohide) ? Math.ceil(gap + shelf) : 0

    Component.onCompleted: {
        shell.trackWindow(win)
        shell.setupDock(win, edge, v.layer, v.screen)
        shell.updateDock(win, edge, v.layer, exclusiveZone)
        visible = true
        refreshKinds()
        updateMask()
    }
    onEdgeChanged: shell.updateDock(win, edge, v.layer, exclusiveZone)
    onExclusiveZoneChanged: shell.updateDock(win, edge, v.layer, exclusiveZone)
    Connections {
        target: config
        function onChanged() { shell.updateDock(win, win.edge, win.v.layer, win.exclusiveZone) }
    }

    // --- Layout --------------------------------------------------------------

    // The kinds of the rows, cached because the zoom is recomputed on every
    // pointer move.
    property var kinds: []
    function refreshKinds() {
        const k = []
        for (let i = 0; i < dockModel.count; ++i)
            k.push(dockModel.get(i).kind)
        kinds = k
    }
    Connections {
        target: dockModel
        function onRowsInserted() { win.refreshKinds() }
        function onRowsRemoved() { win.refreshKinds() }
        function onRowsMoved() { win.refreshKinds() }
        function onModelReset() { win.refreshKinds() }
        function onDataChanged() { win.refreshKinds() }
        function onCountChanged() { win.refreshKinds() }
    }

    // Bumped by a cell whose width changes, so the layout below is redone.
    property int layoutRev: 0
    readonly property real sepWidth: t.separator === "gap" ? Math.round(s * 0.35) : Math.max(10, Math.round(s * 0.3))
    property real hoverU: -1          // pointer position along the dock, in logical coordinates
    property real zoomAmount: 0       // 0..1, eases in and out with the hover
    Behavior on zoomAmount { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }

    // The parabolic zoom: each icon grows with a bell curve of its distance
    // to the pointer, and the row is shifted so that the point under the
    // pointer stays under it.
    readonly property var geom: {
        layoutRev
        const n = kinds.length
        const fw = along
        const base = []
        let total = 0
        for (let i = 0; i < n; ++i) {
            const cellItem = items.itemAt(i)
            const w = (kinds[i] === "separator" ? sepWidth : s * (cellItem ? cellItem.widthFactor : 1)) + spacing
            base.push(w)
            total += w
        }
        const u0 = (fw - total) / 2
        const scales = new Array(n).fill(1)
        let start = u0
        const amount = zoomAmount
        if (amount > 0.001 && hoverU >= 0 && n > 0 && !dragging) {
            const m = Math.min(Math.max(hoverU, u0), u0 + total - 0.01)
            const R = zoomRange * (s + spacing)
            let acc = u0
            for (let i = 0; i < n; ++i) {
                const c = acc + base[i] / 2
                const d = Math.abs(m - c)
                const f = d < R ? (Math.cos(Math.PI * d / R) + 1) / 2 : 0
                scales[i] = 1 + (zoomFactor - 1) * f * amount
                acc += base[i]
            }
            // Find the cell under the pointer and keep that point fixed.
            acc = u0
            let zoomedBefore = 0
            for (let i = 0; i < n; ++i) {
                if (m < acc + base[i] || i === n - 1) {
                    const frac = (m - acc) / base[i]
                    start = m - zoomedBefore - frac * base[i] * scales[i]
                    break
                }
                zoomedBefore += base[i] * scales[i]
                acc += base[i]
            }
        }
        const u = [], w = []
        let length = 0
        for (let i = 0; i < n; ++i) {
            w.push(base[i] * scales[i])
            length += base[i] * scales[i]
        }
        const edgeRoom = pad + 6
        start = Math.max(edgeRoom, Math.min(start, fw - length - edgeRoom))
        let x = start
        for (let i = 0; i < n; ++i) {
            u.push(x)
            x += w[i]
        }
        let maxScale = 1
        for (const sc of scales) maxScale = Math.max(maxScale, sc)
        return { u: u, w: w, scale: scales, start: start, length: length, maxScale: maxScale, restStart: u0, restLength: total }
    }

    // Logical "along" position to the frame's x, flipped where the rotation
    // would otherwise reverse the order.
    function frameX(u, w) { return flipAlong ? along - u - w : u }
    // Logical (along, distance from edge) to window coordinates.
    function toWindow(u, a) {
        if (edge === "top") return Qt.point(u, a)
        if (edge === "left") return Qt.point(a, u)
        if (edge === "right") return Qt.point(width - a, u)
        return Qt.point(u, height - a)
    }
    function rectToWindow(u, a, lenU, lenA) {
        const p1 = toWindow(u, a), p2 = toWindow(u + lenU, a + lenA)
        return Qt.rect(Math.min(p1.x, p2.x), Math.min(p1.y, p2.y), Math.abs(p2.x - p1.x), Math.abs(p2.y - p1.y))
    }

    // --- Hover, hide, mask ----------------------------------------------------

    property int hoveredIndex: -1
    property bool dragging: false
    property int dragIndex: -1
    readonly property bool hovered: band.containsMouse || menuOpen || popupOpen || dragging || band.pressed
    property bool hiddenState: false
    readonly property real hideOffset: hiddenState ? gap + shelf + s * 0.2 + 6 : 0
    property real slide: hideOffset
    Behavior on slide { NumberAnimation { duration: 260; easing.type: Easing.InOutCubic } }

    onHoveredChanged: {
        if (hovered) {
            hideTimer.stop()
            hiddenState = false
        } else if (v.autohide) {
            hideTimer.restart()
        }
        updateMask()
    }
    Timer {
        id: hideTimer
        interval: win.v.autohideDelay
        onTriggered: if (!win.hovered && win.v.autohide) win.hiddenState = true
    }
    Connections {
        target: config
        function onChanged() {
            if (!win.v.autohide) win.hiddenState = false
            else if (!win.hovered) hideTimer.restart()
        }
    }

    onGeomChanged: Qt.callLater(updateMask)
    onHiddenStateChanged: Qt.callLater(updateMask)
    onWidthChanged: Qt.callLater(updateMask)
    onHeightChanged: Qt.callLater(updateMask)
    function updateMask() {
        if (menuOpen || popupOpen) {
            shell.setInputRect(win, Qt.rect(0, 0, width, height))
            return
        }
        const g = geom
        const startU = panel ? 0 : g.start - pad - 4
        const lenU = panel ? along : g.length + 2 * pad + 8
        if (hiddenState) {
            shell.setInputRect(win, rectToWindow(startU, 0, lenU, 3))
            return
        }
        const reach = Math.max(gap + shelf, iconBase + s * (hovered ? g.maxScale : 1) + 4)
        shell.setInputRect(win, rectToWindow(startU, 0, lenU, reach))
    }

    // --- Drawing --------------------------------------------------------------

    Item {
        id: frame
        width: win.along
        height: win.surfaceDepth
        x: (win.width - width) / 2
        y: (win.height - height) / 2
        rotation: win.frameRotation

        Item {
            id: content
            anchors.fill: parent
            transform: Translate { y: win.slide }

            DockShelf {
                id: shelfItem
                dock: win
                app: win.app
            }

            Repeater {
                id: items
                model: dockModel
                delegate: DockItem {
                    dock: win
                    app: win.app
                }
            }
        }

        MouseArea {
            id: band
            anchors.fill: parent
            hoverEnabled: true
            preventStealing: true
            acceptedButtons: Qt.LeftButton | Qt.MiddleButton | Qt.RightButton
            property point pressPoint
            property int pressIndex: -1

            function logical(mx) { return win.flipAlong ? win.along - mx : mx }
            function indexAt(mx) {
                const u = logical(mx)
                const g = win.geom
                for (let i = 0; i < g.u.length; ++i)
                    if (u >= g.u[i] && u < g.u[i] + g.w[i])
                        return i
                return -1
            }

            onPositionChanged: (mouse) => {
                if (pressed && pressIndex >= 0 && !win.dragging && pressedButtons & Qt.LeftButton) {
                    const d = Math.hypot(mouse.x - pressPoint.x, mouse.y - pressPoint.y)
                    const row = dockModel.get(pressIndex)
                    if (d > 10 && row.pinned) {
                        win.dragging = true
                        win.dragIndex = pressIndex
                    }
                }
                if (win.dragging) {
                    dragGhost.at = Qt.point(mouse.x, mouse.y)
                    return
                }
                win.hoverU = logical(mouse.x)
                win.zoomAmount = 1
                const i = indexAt(mouse.x)
                if (i !== win.hoveredIndex) {
                    const d = i >= 0 ? items.itemAt(i) : null
                    if (d && d.kind !== "separator" && !win.dragging) d.playHover()
                    win.hoveredIndex = i
                }
            }
            onEntered: win.zoomAmount = 1
            onExited: {
                if (win.menuOpen || win.popupOpen || win.dragging) return
                win.zoomAmount = 0
                win.hoveredIndex = -1
            }
            onPressed: (mouse) => {
                pressPoint = Qt.point(mouse.x, mouse.y)
                pressIndex = indexAt(mouse.x)
            }
            onReleased: (mouse) => {
                if (win.dragging) {
                    const outside = mouse.y < frame.height - win.iconBase - win.s * win.zoomFactor - 40
                    const from = win.dragIndex
                    win.dragging = false
                    win.dragIndex = -1
                    if (outside) {
                        // Dragged off the dock: unpin it.
                        const row = dockModel.get(from)
                        if (row.pinned) config.unpin(row.appId)
                    } else {
                        const to = indexAt(mouse.x)
                        if (to >= 0 && to !== from) dockModel.movePinned(from, to)
                    }
                    win.hoverU = logical(mouse.x)
                    pressIndex = -2
                    return
                }
            }
            onClicked: (mouse) => {
                if (pressIndex === -2) return
                const i = indexAt(mouse.x)
                if (mouse.button === Qt.RightButton) {
                    win.openMenu(i)
                    return
                }
                if (i < 0) return
                const row = dockModel.get(i)
                const delegate = items.itemAt(i)
                if (row.kind === "applet") {
                    if (!delegate) return
                    // An applet with a double-click action waits a moment so
                    // a double click doesn't also count as a single one.
                    const applet = delegate.applet
                    if (mouse.button === Qt.LeftButton && applet && applet.doubleClick) {
                        appletClick.row = i
                        appletClick.restart()
                    } else {
                        delegate.activateApplet(mouse.button, i)
                    }
                    return
                }
                if (row.kind !== "app") return
                if (mouse.button === Qt.MiddleButton) {
                    dockModel.launchNew(i)
                } else {
                    if (delegate) delegate.playClick()
                    dockModel.activate(i)
                }
            }
            onDoubleClicked: (mouse) => {
                const i = indexAt(mouse.x)
                if (i < 0 || i !== appletClick.row) return
                appletClick.stop()
                const delegate = items.itemAt(i)
                if (delegate && delegate.applet && delegate.applet.doubleClick) {
                    delegate.playClick()
                    delegate.applet.doubleClick()
                }
            }
            onWheel: (wheel) => {
                const i = indexAt(wheel.x)
                if (i < 0) return
                const row = dockModel.get(i)
                if (row.kind === "applet") {
                    const d = items.itemAt(i)
                    if (d && d.applet && d.applet.scroll) d.applet.scroll(wheel.angleDelta.y > 0 ? 1 : -1)
                    return
                }
                if (row.kind === "app" && row.windowCount > 0)
                    dockModel.cycle(i, wheel.angleDelta.y < 0 ? 1 : -1)
            }
        }
    }

    // Overlay items live in window coordinates, unrotated, so text reads
    // normally on every edge.
    Item {
        id: overlay
        anchors.fill: parent

        DockLabel {
            id: label
            dock: win
            app: win.app
        }

        // The icon being dragged to a new place.
        Image {
            id: dragGhost
            property point at: Qt.point(0, 0)
            visible: win.dragging
            width: win.s * 1.1
            height: width
            sourceSize: Qt.size(width * Screen.devicePixelRatio, height * Screen.devicePixelRatio)
            opacity: 0.85
            source: win.dragIndex >= 0 ? "image://icon/" + encodeURIComponent(dockModel.get(win.dragIndex).icon) : ""
            readonly property point p: frame.mapToItem(overlay, at.x, at.y)
            x: p.x - width / 2
            y: p.y - height / 2
        }
    }

    Timer {
        id: appletClick
        property int row: -1
        interval: 260
        onTriggered: {
            const d = items.itemAt(row)
            if (d) d.activateApplet(Qt.LeftButton, row)
        }
    }

    DockMenu {
        id: menu
        dock: win
        app: win.app
    }

    property bool popupOpen: false
    CalendarPopup {
        id: calendar
        dock: win
        app: win.app
        parent: overlay
        onClosed: if (win) win.calendarClosed()
    }

    function openCalendar(index) {
        popupOpen = true
        Qt.callLater(() => calendar.openFor(index))
    }
    function calendarClosed() {
        popupOpen = false
        if (!band.containsMouse) {
            zoomAmount = 0
            hoveredIndex = -1
        }
        Qt.callLater(updateMask)
    }

    function appletTitle(index) {
        const d = items.itemAt(index)
        return d && d.applet ? d.applet.title : ""
    }
    function appletAt(index) {
        const d = items.itemAt(index)
        return d ? d.applet : null
    }

    function openMenu(index) {
        menuOpen = true
        Qt.callLater(() => menu.openFor(index))
    }
    function menuClosed() {
        menuOpen = false
        if (!band.containsMouse) {
            zoomAmount = 0
            hoveredIndex = -1
        }
        Qt.callLater(updateMask)
    }

    // Position, in window coordinates, just outside the icon at `index`, on
    // the side away from the screen edge.
    function anchorFor(index, extra) {
        const g = geom
        if (index < 0 || index >= g.u.length)
            return toWindow(hoverU >= 0 ? hoverU : along / 2, iconBase + s + (extra || 0))
        const sc = g.scale[index]
        const delegate = items.itemAt(index)
        const lift = delegate ? delegate.bounce : 0
        return toWindow(g.u[index] + g.w[index] / 2, iconBase + s * sc + lift + (extra || 0) - slide)
    }
}
