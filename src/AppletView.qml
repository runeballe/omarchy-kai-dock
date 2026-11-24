import QtQuick

// Loads an applet by id, for the dock or as a desklet.
Loader {
    id: view
    property string appletId
    property string mode: "dock"
    property var app
    property var dock

    readonly property var sources: ({
        apps: "qrc:/applets/AppsApplet.qml",
        clock: "qrc:/applets/ClockApplet.qml",
        weather: "qrc:/applets/WeatherApplet.qml",
        sysmon: "qrc:/applets/SysmonApplet.qml",
        battery: "qrc:/applets/BatteryApplet.qml",
        trash: "qrc:/applets/TrashApplet.qml",
        windowskiller: "qrc:/applets/WindowsKillerApplet.qml"
    })
    source: sources[appletId] || ""
    onLoaded: {
        item.mode = Qt.binding(() => view.mode)
        item.app = Qt.binding(() => view.app)
        if (item.dock !== undefined) item.dock = Qt.binding(() => view.dock)
    }
    function activate(button, row) {
        if (item && item.activate) item.activate(button, row)
    }
}
