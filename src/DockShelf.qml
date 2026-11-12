import QtQuick
import QtQuick.Shapes

// The shelf behind the icons: a floating rounded bar, a full-length panel, or
// a 3D plane receding from the screen edge. Drawn in frame coordinates.
Item {
    id: shelf
    required property var dock
    required property var app
    anchors.fill: parent

    readonly property var t: dock.t
    readonly property var g: dock.geom
    readonly property real inset: Math.max(2, dock.pad - dock.spacing / 2)
    readonly property real startU: dock.panel ? 0 : g.start - inset
    readonly property real lengthU: dock.panel ? dock.along : g.length + 2 * inset
    readonly property real bottomY: height - dock.gap
    readonly property real topY: bottomY - dock.shelf
    readonly property color fill1: app.withAlpha(app.color(t.background2 || t.background), t.backgroundOpacity)
    readonly property color fill2: app.withAlpha(app.color(t.background), t.backgroundOpacity)
    readonly property color line: app.withAlpha(app.color(t.border, "#ffffff"), t.borderOpacity)
    readonly property bool hasFill: t.backgroundOpacity > 0.001
    readonly property bool hasBorder: t.borderWidth > 0 && t.borderOpacity > 0.001

    // Floating and panel.
    Rectangle {
        id: bar
        visible: !dock.plane && (shelf.hasFill || shelf.hasBorder)
        x: dock.frameX(shelf.startU, shelf.lengthU)
        y: shelf.topY
        width: shelf.lengthU
        height: dock.shelf + (dock.panel ? 0 : 0)
        radius: dock.panel ? 0 : Math.min(app.radius, height / 2)
        gradient: Gradient {
            GradientStop { position: 0; color: shelf.hasFill ? shelf.fill1 : "transparent" }
            GradientStop { position: 1; color: shelf.hasFill ? shelf.fill2 : "transparent" }
        }
        border.width: dock.panel ? 0 : (shelf.hasBorder ? shelf.t.borderWidth : 0)
        border.color: shelf.line

        // A gloss over the half away from the edge.
        Rectangle {
            visible: shelf.t.gloss > 0
            anchors { left: parent.left; right: parent.right; top: parent.top; margins: parent.border.width }
            height: parent.height * 0.48
            radius: Math.max(0, parent.radius - parent.border.width)
            gradient: Gradient {
                GradientStop { position: 0; color: Qt.rgba(1, 1, 1, shelf.t.gloss) }
                GradientStop { position: 1; color: Qt.rgba(1, 1, 1, 0) }
            }
        }

        // A panel only draws its inner edge.
        Rectangle {
            visible: dock.panel && shelf.hasBorder
            anchors { left: parent.left; right: parent.right; top: parent.top }
            height: shelf.t.borderWidth
            color: shelf.line
        }
    }

    // The 3D plane: a trapezoid whose far edge is narrower, like a shelf seen
    // from slightly above, with a bright lip along the front.
    Item {
        id: planeItem
        visible: dock.plane
        anchors.fill: parent
        readonly property real angle: (shelf.t.planeAngle || 60) * Math.PI / 180
        readonly property real recede: dock.shelf / Math.tan(angle) * 1.4
        readonly property real x0: dock.frameX(shelf.startU, shelf.lengthU)
        readonly property real x1: x0 + shelf.lengthU

        Shape {
            anchors.fill: parent
            preferredRendererType: Shape.CurveRenderer
            ShapePath {
                strokeWidth: shelf.hasBorder ? shelf.t.borderWidth : -1
                strokeColor: shelf.line
                fillGradient: LinearGradient {
                    x1: 0; y1: shelf.topY
                    x2: 0; y2: shelf.bottomY
                    GradientStop { position: 0; color: shelf.fill1 }
                    GradientStop { position: 1; color: shelf.fill2 }
                }
                startX: planeItem.x0; startY: shelf.bottomY
                PathLine { x: planeItem.x1; y: shelf.bottomY }
                PathLine { x: planeItem.x1 - planeItem.recede; y: shelf.topY }
                PathLine { x: planeItem.x0 + planeItem.recede; y: shelf.topY }
                PathLine { x: planeItem.x0; y: shelf.bottomY }
            }
        }
        // Gloss on the far half of the plane.
        Shape {
            anchors.fill: parent
            visible: shelf.t.gloss > 0
            ShapePath {
                strokeWidth: -1
                fillGradient: LinearGradient {
                    x1: 0; y1: shelf.topY
                    x2: 0; y2: shelf.topY + dock.shelf * 0.6
                    GradientStop { position: 0; color: Qt.rgba(1, 1, 1, shelf.t.gloss) }
                    GradientStop { position: 1; color: Qt.rgba(1, 1, 1, 0) }
                }
                startX: planeItem.x0 + planeItem.recede; startY: shelf.topY
                PathLine { x: planeItem.x1 - planeItem.recede; y: shelf.topY }
                PathLine { x: planeItem.x1 - planeItem.recede * 0.4; y: shelf.topY + dock.shelf * 0.6 }
                PathLine { x: planeItem.x0 + planeItem.recede * 0.4; y: shelf.topY + dock.shelf * 0.6 }
                PathLine { x: planeItem.x0 + planeItem.recede; y: shelf.topY }
            }
        }
        // The front lip.
        Rectangle {
            x: planeItem.x0
            y: shelf.bottomY
            width: shelf.lengthU
            height: Math.max(3, Math.round(dock.s * 0.07))
            gradient: Gradient {
                GradientStop { position: 0; color: app.withAlpha(app.color(shelf.t.border, "#ffffff"), Math.min(1, shelf.t.borderOpacity + 0.25)) }
                GradientStop { position: 1; color: app.withAlpha(app.color(shelf.t.background), shelf.t.backgroundOpacity) }
            }
        }
    }
}
