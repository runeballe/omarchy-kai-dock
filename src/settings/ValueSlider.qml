import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// A slider with its value shown beside it.
RowLayout {
    id: vs
    property real from: 0
    property real to: 1
    property real stepSize: 0
    property real value: 0
    property int decimals: 0
    property string suffix: ""
    signal moved(real value)
    width: parent ? parent.width : 260
    spacing: 8
    Slider {
        id: slider
        Layout.fillWidth: true
        from: vs.from
        to: vs.to
        stepSize: vs.stepSize
        value: vs.value
        onMoved: vs.moved(value)
    }
    Label {
        Layout.preferredWidth: 46
        horizontalAlignment: Text.AlignRight
        text: { translator.language; return slider.value.toLocaleString(Qt.locale(), 'f', vs.decimals) + vs.suffix }
        opacity: 0.8
    }
}
