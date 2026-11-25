import QtQuick

// An applet made from a folder in ~/.config/kai/applets (see
// ScriptApplets): its icon, its script's output as quick-info and progress,
// and its commands on click, middle click, double click and scroll.
Item {
    id: applet
    property string appletId
    property string mode: "dock"
    property var app
    readonly property bool desklet: mode === "desklet"
    readonly property size deskletSize: Qt.size(220, 150)

    readonly property var manifest: { scriptApplets.applets; return scriptApplets.info(appletId) }
    readonly property var state: scriptApplets.states[appletId] || ({})
    readonly property string name: manifest.name || appletId
    readonly property string quickInfo: state.error ? "!" : (state.text || "")
    readonly property real progress: state.progress !== undefined ? state.progress : -1
    readonly property string title: state.error ? qsTr("%1 — %2").arg(name).arg(state.error)
        : state.tooltip ? name + " — " + state.tooltip.split("\n")[0] : name

    readonly property string iconName: state.icon || manifest.icon || "application-x-executable"
    readonly property string iconSource: iconName.startsWith("/") ? "image://icon/" + encodeURIComponent(iconName)
        : iconName.indexOf("/") > 0 || iconName.endsWith(".png") || iconName.endsWith(".svg")
            ? "image://icon/" + encodeURIComponent(manifest.dir + "/" + iconName)
            : "image://icon/" + encodeURIComponent(iconName)

    function activate(button) {
        scriptApplets.trigger(appletId, button === Qt.MiddleButton ? "middle" : "click")
    }
    // Only offered when the script has the action, so a plain click on an
    // applet without one isn't delayed waiting for a second click.
    readonly property var doubleClick: { scriptApplets.applets; return scriptApplets.hasAction(appletId, "double") ? () => scriptApplets.trigger(appletId, "double") : null }
    readonly property var scroll: { scriptApplets.applets; return scriptApplets.hasAction(appletId, "scroll-up") || scriptApplets.hasAction(appletId, "scroll-down")
        ? (direction) => scriptApplets.trigger(appletId, direction > 0 ? "scroll-up" : "scroll-down") : null }
    function menuEntries() {
        return [{ text: qsTr("Run now"), action: () => scriptApplets.refresh(appletId) }]
    }

    Image {
        id: icon
        width: applet.desklet ? 72 : applet.width
        height: width
        anchors.horizontalCenter: parent.horizontalCenter
        y: applet.desklet ? 14 : (applet.height - height) / 2
        sourceSize: Qt.size(width * 2, height * 2)
        source: applet.iconSource
    }
    Text {
        visible: applet.desklet
        anchors.top: icon.bottom
        anchors.topMargin: 8
        anchors.horizontalCenter: parent.horizontalCenter
        width: parent.width - 20
        horizontalAlignment: Text.AlignHCenter
        elide: Text.ElideRight
        text: applet.state.error ? applet.state.error : (applet.state.tooltip || applet.name)
        color: applet.app ? applet.app.color(config.theme.textColor, "#f2f2f2") : "#f2f2f2"
        font.pixelSize: 13
    }
}
