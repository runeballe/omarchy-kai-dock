import QtQuick

// CPU and memory load: a history graph in the dock, two graphs as a desklet.
Item {
    id: applet
    property string mode: "dock"
    property var app
    readonly property bool desklet: mode === "desklet"
    readonly property size deskletSize: Qt.size(300, 190)
    readonly property color accent: app ? app.accent : "#FFD60A"

    readonly property string title: {
        translator.language
        return qsTr("CPU %1% · Memory %2 of %3 GB")
            .arg(Math.round(system.cpu * 100))
            .arg(system.memoryUsedGiB.toLocaleString(Qt.locale(), 'f', 1))
            .arg(system.memoryTotalGiB.toLocaleString(Qt.locale(), 'f', 0))
            + (system.temperature > 0 ? " · " + Math.round(system.temperature) + "°C" : "")
    }

    function activate(button) {
        if (shell.hasCommand("btop")) shell.run("xdg-terminal-exec btop || alacritty -e btop")
    }

    component Graph: Item {
        id: graph
        property var values: []
        property int bars: 20
        property color tint: "white"
        readonly property var shown: values.slice(Math.max(0, values.length - bars))
        Row {
            anchors.bottom: parent.bottom
            spacing: graph.width / graph.bars * 0.18
            Repeater {
                model: graph.shown.length
                Rectangle {
                    required property int index
                    width: graph.width / graph.bars * 0.82
                    height: Math.max(1, graph.height * graph.shown[index])
                    anchors.bottom: parent.bottom
                    radius: Math.min(width / 2, 2)
                    color: graph.tint
                    opacity: 0.45 + 0.55 * (index / Math.max(1, graph.shown.length - 1))
                }
            }
        }
    }

    // Dock.
    Item {
        visible: !applet.desklet
        anchors.fill: parent
        Graph {
            x: parent.width * 0.16
            y: parent.height * 0.2
            width: parent.width * 0.52
            height: parent.height * 0.6
            values: system.cpuHistory
            bars: 10
            tint: applet.accent
        }
        Rectangle {
            x: parent.width * 0.74
            y: parent.height * 0.2
            width: parent.width * 0.1
            height: parent.height * 0.6
            radius: width / 2
            color: Qt.rgba(1, 1, 1, 0.12)
            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: parent.height * system.memory
                radius: width / 2
                color: "#7fd1ff"
            }
        }
    }

    // Desklet.
    Column {
        visible: applet.desklet
        anchors.fill: parent
        anchors.margins: 16
        spacing: 10
        Repeater {
            model: [
                { label: qsTr("CPU"), value: Math.round(system.cpu * 100) + "%", history: system.cpuHistory, tint: applet.accent },
                { label: qsTr("Memory"), value: system.memoryUsedGiB.toLocaleString(Qt.locale(), 'f', 1) + " / " + system.memoryTotalGiB.toLocaleString(Qt.locale(), 'f', 0) + " GB", history: system.memoryHistory, tint: "#7fd1ff" }
            ]
            Column {
                required property var modelData
                width: parent.width
                spacing: 4
                Item {
                    width: parent.width
                    height: 18
                    Text { text: modelData.label; color: Qt.rgba(1, 1, 1, 0.75); font.pixelSize: 13 }
                    Text { anchors.right: parent.right; text: modelData.value; color: "white"; font.pixelSize: 13; font.weight: Font.DemiBold }
                }
                Graph {
                    width: parent.width
                    height: 44
                    values: modelData.history
                    bars: 40
                    tint: modelData.tint
                }
            }
        }
        Text {
            visible: system.temperature > 0
            text: qsTr("Temperature %1 °C").arg(Math.round(system.temperature))
            color: Qt.rgba(1, 1, 1, 0.6)
            font.pixelSize: 12
        }
    }
}
