import QtQuick
import QtQuick.Shapes

// A miniature of a dock in a theme, for the theme gallery.
Item {
    id: preview
    required property var app
    required property var themeData
    readonly property var d: themeData || {}
    function val(key, fallback) { return d[key] !== undefined ? d[key] : fallback }
    readonly property string view: val("view", "floating")
    readonly property real alpha: val("backgroundOpacity", 0.8)
    readonly property color c1: app.withAlpha(app.color(val("background2", "") || val("background", "#16161a")), alpha)
    readonly property color c2: app.withAlpha(app.color(val("background", "#16161a")), alpha)
    readonly property color line: app.withAlpha(app.color(val("border", "#ffffff"), "#ffffff"), val("borderOpacity", 0.1))
    readonly property int r: val("radius", -1) < 0 ? app.radius : val("radius", 0)

    // A wallpaper-ish backdrop so translucency shows.
    Rectangle {
        anchors.fill: parent
        radius: Math.min(6, app.radius)
        gradient: Gradient {
            GradientStop { position: 0; color: "#3b4a6b" }
            GradientStop { position: 0.6; color: "#7a5c80" }
            GradientStop { position: 1; color: "#d98c6a" }
        }
    }

    readonly property real shelfW: view === "panel" ? width : width * 0.78
    readonly property real shelfH: view === "plane" ? 16 : 30
    readonly property real shelfY: height - (view === "panel" ? 0 : 8) - shelfH

    Rectangle {
        visible: preview.view !== "plane"
        x: (preview.width - width) / 2
        y: preview.shelfY
        width: preview.shelfW
        height: preview.shelfH
        radius: preview.view === "panel" ? 0 : Math.min(preview.r * 0.6, height / 2)
        gradient: Gradient {
            GradientStop { position: 0; color: preview.c1 }
            GradientStop { position: 1; color: preview.c2 }
        }
        border.width: preview.val("borderWidth", 1) > 0 ? 1 : 0
        border.color: preview.line
    }
    Shape {
        visible: preview.view === "plane"
        anchors.fill: parent
        ShapePath {
            strokeColor: preview.line
            strokeWidth: 1
            fillGradient: LinearGradient {
                x1: 0; y1: preview.shelfY; x2: 0; y2: preview.shelfY + preview.shelfH
                GradientStop { position: 0; color: preview.c1 }
                GradientStop { position: 1; color: preview.c2 }
            }
            startX: (preview.width - preview.shelfW) / 2; startY: preview.shelfY + preview.shelfH
            PathLine { x: (preview.width + preview.shelfW) / 2; y: preview.shelfY + preview.shelfH }
            PathLine { x: (preview.width + preview.shelfW) / 2 - 18; y: preview.shelfY }
            PathLine { x: (preview.width - preview.shelfW) / 2 + 18; y: preview.shelfY }
            PathLine { x: (preview.width - preview.shelfW) / 2; y: preview.shelfY + preview.shelfH }
        }
    }
    Row {
        anchors.horizontalCenter: parent.horizontalCenter
        y: preview.shelfY + (preview.view === "plane" ? -14 : 5)
        spacing: 6
        Repeater {
            model: ["#4285f4", "#34a853", "#fbbc05", "#ea4335", "#9c6bff"]
            Column {
                required property string modelData
                required property int index
                spacing: 2
                Rectangle {
                    width: index === 2 ? 26 : 20
                    height: width
                    anchors.horizontalCenter: parent.horizontalCenter
                    radius: width * 0.25
                    color: modelData
                    anchors.bottomMargin: 0
                }
                Rectangle {
                    visible: index < 3 && preview.val("indicator", "dot") !== "none"
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: preview.val("indicator", "dot") === "dot" ? 3 : 10
                    height: 3
                    radius: 1.5
                    color: preview.app.color(preview.val("indicatorColor", "accent"), preview.app.accent)
                }
            }
        }
    }
}
