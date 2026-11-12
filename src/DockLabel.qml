import QtQuick

// The name of the hovered item, just outside it, in window coordinates.
Rectangle {
    id: label
    required property var dock
    required property var app

    readonly property var t: dock.t
    readonly property int index: dock.hoveredIndex
    readonly property var row: index >= 0 && index < dockModel.count ? dockModel.get(index) : null
    readonly property string text: {
        if (!row || row.kind === "separator") return ""
        if (row.kind === "applet") return dock.appletTitle(index)
        return row.name
    }

    visible: dock.v.showLabels && text !== "" && !dock.menuOpen && !dock.dragging && !dock.hiddenState
    opacity: visible ? t.labelOpacity : 0
    color: app.color(t.labelBackground, "#101014")
    radius: Math.min(app.radius + 2, height / 2)
    border.width: 1
    border.color: app.withAlpha(app.color(t.border, "#ffffff"), Math.min(0.25, t.borderOpacity + 0.05))
    width: textItem.implicitWidth + 20
    height: textItem.implicitHeight + 10

    readonly property point p: dock.anchorFor(index, 10)
    x: {
        let x = dock.edge === "left" ? p.x : dock.edge === "right" ? p.x - width : p.x - width / 2
        return Math.max(4, Math.min(x, dock.width - width - 4))
    }
    y: {
        let y = dock.edge === "top" ? p.y : (dock.edge === "bottom" ? p.y - height : p.y - height / 2)
        return Math.max(4, Math.min(y, dock.height - height - 4))
    }

    Text {
        id: textItem
        anchors.centerIn: parent
        text: label.text
        color: label.app.color(label.t.labelColor, "#ffffff")
        font.pixelSize: 13
        font.weight: Font.Medium
    }
}
