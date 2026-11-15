import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Controls.Material

// The app launcher: every installed app in a searchable grid over the screen.
// Opened from the Applications applet or with `kai apps`. Type to filter,
// arrows to move, Enter to launch, Esc or a click outside to close.
Window {
    id: grid
    required property var app

    readonly property var t: config.theme
    readonly property color fg: app.color(t.textColor, "#f2f2f2")
    readonly property bool darkCard: fg.hslLightness > 0.5
    property var ids: []

    readonly property int cell: 124
    readonly property int columns: Math.max(1, Math.min(7, Math.floor((width - 160) / cell)))
    readonly property int rows: Math.max(1, Math.min(4, Math.floor((height - 260) / cell)))

    function refresh() {
        ids = apps.search(search.text)
        view.currentIndex = 0
    }
    function open() {
        search.text = ""
        refresh()
        visible = true
        requestActivate()
        search.forceActiveFocus()
    }
    function dismiss() { visible = false }
    function toggle() { if (visible) dismiss(); else open() }
    function launch(id) {
        if (!id) return
        dismiss()
        apps.launch(id)
    }

    width: Screen.width
    height: Screen.height
    color: "transparent"
    flags: Qt.FramelessWindowHint
    title: "kai apps"
    visible: false
    Material.theme: darkCard ? Material.Dark : Material.Light
    Material.accent: app.accent

    Component.onCompleted: shell.setupLauncher(grid)
    Connections {
        target: apps
        function onChanged() { if (grid.visible) grid.refresh() }
    }

    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(0, 0, 0, 0.45)
        MouseArea { anchors.fill: parent; acceptedButtons: Qt.AllButtons; onClicked: grid.dismiss() }
    }

    Rectangle {
        id: card
        anchors.centerIn: parent
        width: grid.columns * grid.cell + 48
        height: 24 + search.height + 16 + grid.rows * grid.cell + 24
        radius: Math.min(grid.app.radius + 8, 26)
        // The same two-stop fill as the desklets, close to opaque so names read.
        gradient: Gradient {
            GradientStop { position: 0; color: grid.app.withAlpha(grid.app.color(grid.t.deskletBackground || grid.t.background2 || grid.t.background, "#16161a"), 0.96) }
            GradientStop { position: 1; color: grid.app.withAlpha(grid.app.color(grid.t.deskletBackground || grid.t.background, "#16161a"), 0.96) }
        }
        border.width: 1
        border.color: grid.app.withAlpha(grid.app.color(grid.t.border, "#ffffff"), Math.max(0.14, grid.t.borderOpacity))
        MouseArea { anchors.fill: parent; acceptedButtons: Qt.AllButtons }

        TextField {
            id: search
            x: 24
            y: 24
            width: parent.width - 48
            placeholderText: qsTr("Search apps")
            selectByMouse: true
            onTextEdited: grid.refresh()
            Keys.onPressed: (event) => {
                const n = grid.ids.length
                const i = view.currentIndex
                if (event.key === Qt.Key_Escape) grid.dismiss()
                else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) grid.launch(grid.ids[i])
                else if (event.key === Qt.Key_Down) view.currentIndex = Math.min(n - 1, i + grid.columns)
                else if (event.key === Qt.Key_Up) view.currentIndex = Math.max(0, i - grid.columns)
                else if (event.key === Qt.Key_Right || event.key === Qt.Key_Tab) view.currentIndex = Math.min(n - 1, i + 1)
                else if (event.key === Qt.Key_Left || event.key === Qt.Key_Backtab) view.currentIndex = Math.max(0, i - 1)
                else return
                event.accepted = true
            }
        }

        GridView {
            id: view
            x: 24
            y: search.y + search.height + 16
            width: grid.columns * grid.cell
            height: grid.rows * grid.cell
            cellWidth: grid.cell
            cellHeight: grid.cell
            clip: true
            model: grid.ids
            boundsBehavior: Flickable.StopAtBounds
            highlightMoveDuration: 0
            ScrollBar.vertical: ScrollBar {}

            delegate: Item {
                id: tile
                required property string modelData
                required property int index
                readonly property var entry: apps.entry(modelData)
                readonly property bool current: GridView.isCurrentItem
                width: grid.cell
                height: grid.cell

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 4
                    radius: Math.min(grid.app.radius + 4, 16)
                    color: tile.current ? grid.app.withAlpha(grid.app.accent, 0.22)
                         : hover.containsMouse ? Qt.rgba(grid.fg.r, grid.fg.g, grid.fg.b, 0.08) : "transparent"
                    border.width: tile.current ? 1 : 0
                    border.color: grid.app.withAlpha(grid.app.accent, 0.7)
                }
                Image {
                    id: icon
                    anchors.horizontalCenter: parent.horizontalCenter
                    y: 16
                    width: 56
                    height: 56
                    sourceSize: Qt.size(112, 112)
                    asynchronous: true
                    source: "image://icon/" + encodeURIComponent(tile.entry.icon || tile.modelData)
                }
                Text {
                    anchors.top: icon.bottom
                    anchors.topMargin: 6
                    x: 8
                    width: parent.width - 16
                    horizontalAlignment: Text.AlignHCenter
                    text: tile.entry.name || tile.modelData
                    color: grid.fg
                    font.pixelSize: 12
                    wrapMode: Text.Wrap
                    maximumLineCount: 2
                    elide: Text.ElideRight
                }
                MouseArea {
                    id: hover
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: grid.launch(tile.modelData)
                }
            }
        }

        Text {
            anchors.centerIn: view
            visible: grid.ids.length === 0
            text: qsTr("No apps match “%1”.").arg(search.text.trim())
            color: Qt.rgba(grid.fg.r, grid.fg.g, grid.fg.b, 0.6)
            font.pixelSize: 15
        }
    }
}
