import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import "AppletInfo.js" as AppletInfo

// The right-click menu for an icon, an applet or the dock itself. Entries
// are built as data so one menu serves every kind of item.
Menu {
    id: menu
    required property var dock
    required property var app

    property var entries: []
    property int row: -1
    property bool confirmingTrash: false
    property bool pendingTrash: false
    modal: false
    dim: false
    Material.roundedScale: app.radius
    width: Math.min(360, Math.max(220, implicitContentWidth + 24))

    onClosed: {
        if (pendingTrash) {
            // Ask before anything is deleted, in a second menu in the same place.
            pendingTrash = false
            confirmingTrash = true
            Qt.callLater(() => openFor(row))
            return
        }
        confirmingTrash = false
        dock.menuClosed()
    }

    function build(index) {
        const e = []
        const r = index >= 0 ? dockModel.get(index) : null
        if (r && r.kind === "app") {
            e.push({ header: true, text: r.name })
            for (const w of r.windows)
                e.push({ text: (w.minimized ? "▾ " : (w.focused ? "● " : "")) + (w.title || r.name),
                         action: () => dockModel.focusWindow(w.address), indent: true })
            if (r.windows.length) e.push({ separator: true })
            if (r.appId) e.push({ text: r.running ? qsTr("New window") : qsTr("Open"), action: () => dockModel.launchNew(index) })
            if (r.appId) e.push({ text: r.pinned ? qsTr("Unpin from dock") : qsTr("Pin to dock"), action: () => dockModel.togglePin(index) })
            if (r.windows.length === 1) e.push({ text: qsTr("Minimize"), action: () => hyprland.minimize(r.windows[0].address) })
            if (r.windows.length) e.push({ text: r.windows.length > 1 ? qsTr("Close all windows") : qsTr("Close"), action: () => dockModel.closeAll(index) })
        } else if (r && r.kind === "applet") {
            const id = r.appletId
            e.push({ header: true, text: AppletInfo.name(id) })
            const applet = dock.appletAt(index)
            if (applet && applet.menuEntries)
                for (const m of applet.menuEntries())
                    e.push(m)
            if (id === "trash" && system.trashCount > 0)
                e.push({ text: qsTr("Empty trash…"), action: () => { menu.pendingTrash = true } })
            e.push({ text: qsTr("Detach as desklet"), action: () => config.setApplet(id, "detached", true) })
            e.push({ text: qsTr("Remove from dock"), action: () => config.setApplet(id, "enabled", false) })
            e.push({ text: qsTr("Applet settings…"), action: () => app.openSettings("applets") })
        }
        if (e.length) e.push({ separator: true })
        e.push({ text: qsTr("Dock settings…"), action: () => app.openSettings() })
        e.push({ text: qsTr("Themes…"), action: () => app.openSettings("themes") })
        if (!r) e.push({ text: qsTr("Quit kai"), action: () => Qt.quit() })
        return e
    }

    function buildTrashConfirm() {
        return [
            { header: true, text: qsTr("Empty the trash?") },
            { text: qsTr("Permanently delete %n item(s)", "", system.trashCount), action: () => system.emptyTrash(), danger: true },
            { text: qsTr("Cancel"), action: () => {} }
        ]
    }

    function openFor(index) {
        row = index
        entries = confirmingTrash ? buildTrashConfirm() : build(index)
        Qt.callLater(place)
    }

    function place() {
        const p = dock.anchorFor(row, 6)
        const w = width, h = implicitHeight
        let x = p.x - w / 2, y = p.y - h
        if (dock.edge === "top") y = p.y
        if (dock.edge === "left") { x = p.x; y = p.y - h / 2 }
        if (dock.edge === "right") { x = p.x - w; y = p.y - h / 2 }
        menu.x = Math.max(6, Math.min(x, dock.width - w - 6))
        menu.y = Math.max(6, Math.min(y, dock.height - h - 6))
        menu.open()
    }

    Instantiator {
        model: menu.entries
        delegate: MenuItem {
            id: entry
            required property var modelData
            readonly property var d: modelData
            text: d.separator ? "" : d.text
            enabled: !d.header && !d.separator
            height: d.separator ? 9 : implicitHeight
            leftPadding: d.indent ? 22 : 14
            font.weight: d.header ? Font.DemiBold : Font.Normal
            font.pixelSize: d.header ? 13 : 14
            Material.foreground: d.danger ? "#ff6b6b" : undefined
            contentItem: d.separator ? sepLine : labelText
            Rectangle {
                id: sepLine
                visible: entry.d.separator
                height: 1
                color: Qt.rgba(1, 1, 1, 0.12)
                anchors.verticalCenter: parent ? parent.verticalCenter : undefined
            }
            Text {
                id: labelText
                text: entry.text
                font: entry.font
                color: entry.d.danger ? "#ff6b6b" : entry.Material.foreground
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }
            onTriggered: {
                if (d.action) d.action()
            }
            Component.onCompleted: {
                const t = text
                if (t.length > 60) text = t.slice(0, 57) + "…"
            }
        }
        onObjectAdded: (index, object) => menu.insertItem(index, object)
        onObjectRemoved: (index, object) => menu.removeItem(object)
    }
}
