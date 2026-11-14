import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material

// A month calendar in the glass style, opened from the clock applet. Lives in
// the dock window like DockMenu; openFor() anchors it to the clock icon.
Popup {
    id: popup
    required property var dock
    required property var app
    property int row: -1
    property date viewMonth: new Date(new Date().getFullYear(), new Date().getMonth(), 1)

    // Recomputed on every openFor: the dock can run for weeks at a time.
    property date today: new Date()
    readonly property string monthLabel: popup.viewMonth.toLocaleDateString(Qt.locale(), "MMMM yyyy")
    readonly property int daysInMonth: new Date(popup.viewMonth.getFullYear(), popup.viewMonth.getMonth() + 1, 0).getDate()
    // Qt's firstDayOfWeek is 1 (Monday) … 7 (Sunday); Date.getDay() is 0 (Sunday) … 6 (Saturday).
    readonly property int firstDayOffset:
        (new Date(popup.viewMonth.getFullYear(), popup.viewMonth.getMonth(), 1).getDay() - Qt.locale().firstDayOfWeek + 7) % 7
    readonly property color accent: app.accent
    // Colors follow the active theme; undefined tokens fall back to the glass.
    // dock/app arrive through initial properties in arbitrary order, so color
    // bindings wait for both before touching their values.
    readonly property var t: dock && dock.t ? dock.t : ({})
    readonly property bool ready: dock !== undefined && app !== undefined

    function tint(c, a) {
        const q = Qt.color(c)
        return Qt.rgba(q.r, q.g, q.b, a)
    }

    width: 280
    height: 340
    padding: 14
    modal: false
    dim: false
    closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnEscape
    Material.roundedScale: app.radius
    background: Rectangle {
        id: calBackground
        objectName: "calBackground"
        radius: Math.min(popup.app.radius + 4, 18)
        color: popup.ready ? popup.tint(popup.app.color(popup.t.background, "#1a1a20"), popup.t.backgroundOpacity >= 0 ? popup.t.backgroundOpacity : 0.82) : "transparent"
        border.width: Math.max(1, popup.t.borderWidth || 1)
        border.color: popup.ready ? popup.tint(popup.app.color(popup.t.border, "#ffffff"), Math.max(0.12, popup.t.borderOpacity >= 0 ? popup.t.borderOpacity : 0.14)) : "transparent"
    }

    function openFor(index) {
        row = index
        today = new Date()
        viewMonth = new Date(today.getFullYear(), today.getMonth(), 1)
        if (!dock) {
            open()
            return
        }
        const p = dock.anchorFor(index, 6)
        const w = width, h = height
        let x = p.x - w / 2, y = p.y - h
        if (dock.edge === "top") y = p.y
        if (dock.edge === "left") { x = p.x; y = p.y - h / 2 }
        if (dock.edge === "right") { x = p.x - w; y = p.y - h / 2 }
        popup.x = Math.max(6, Math.min(x, dock.width - w - 6))
        popup.y = Math.max(6, Math.min(y, dock.height - h - 6))
        open()
    }
    function shift(delta) {
        viewMonth = new Date(viewMonth.getFullYear(), viewMonth.getMonth() + delta, 1)
    }

    contentItem: Column {
        spacing: 8

        component NavButton: AbstractButton {
            id: nav
            required property string glyph
            width: 26
            height: 26
            background: Rectangle {
                radius: Math.min(4, popup.app.radius + 4)
                color: nav.hovered ? Qt.rgba(1, 1, 1, 0.2) : "transparent"
            }
            contentItem: Text {
                text: nav.glyph
                color: "#f2f2f2"
                font.pixelSize: 13
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }

        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 8
            NavButton { glyph: "◀"; onClicked: popup.shift(-1) }            Text {
                objectName: "calMonthLabel"
                text: popup.monthLabel
                color: popup.app.color(popup.t.textColor, "#f2f2f2")
                font.pixelSize: 14
                font.weight: Font.DemiBold
                anchors.verticalCenter: parent.verticalCenter
                width: 150
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
            }
            NavButton { glyph: "▶"; onClicked: popup.shift(1) }
        }

        Grid {
            anchors.horizontalCenter: parent.horizontalCenter
            columns: 7
            columnSpacing: 2
            rowSpacing: 2

            Repeater {  // weekday headers, starting at the locale's first day
                model: 7
                Item {
                    required property int index
                    width: 34
                    height: 18
                    Text {
                        anchors.centerIn: parent
                        // standaloneDayShortName wants 1-based days matching firstDayOfWeek
                        text: Qt.locale().standaloneDayName(((Qt.locale().firstDayOfWeek - 1 + parent.index) % 7) + 1, "short")
                        color: Qt.rgba(1, 1, 1, 0.5)
                        font.pixelSize: 10
                    }
                }
            }

            Repeater {
                model: popup.firstDayOffset + popup.daysInMonth
                delegate: Item {
                    required property int index
                    width: 34
                    height: 30
                    readonly property int day: index - popup.firstDayOffset + 1
                    readonly property bool isDay: index >= popup.firstDayOffset
                    readonly property bool isToday: isDay && day === popup.today.getDate()
                        && popup.viewMonth.getMonth() === popup.today.getMonth()
                        && popup.viewMonth.getFullYear() === popup.today.getFullYear()
                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: 2
                        visible: parent.isToday
                        radius: Math.min(4, popup.app.radius + 2)
                        color: popup.accent
                    }
                    Text {
                        anchors.centerIn: parent
                        visible: parent.isDay
                        text: parent.day
                        color: parent.isToday ? "#111114" : popup.app.color(popup.t.textColor, "#f2f2f2")
                        font.pixelSize: 13
                        font.weight: parent.isToday ? Font.DemiBold : Font.Normal
                    }
                }
            }
        }
    }
}
