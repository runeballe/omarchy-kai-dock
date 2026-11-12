import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

// A setting: its name and a line on what it does, with the control on the right.
Item {
    id: row
    property string title
    property string description
    default property alias control: holder.data
    property real controlWidth: 260
    width: parent ? parent.width : 400
    implicitHeight: Math.max(texts.implicitHeight, holder.childrenRect.height) + 18

    ColumnLayout {
        id: texts
        anchors.left: parent.left
        anchors.right: holder.left
        anchors.rightMargin: 16
        anchors.verticalCenter: parent.verticalCenter
        spacing: 2
        Label {
            Layout.fillWidth: true
            text: row.title
            font.pixelSize: 14
            wrapMode: Text.Wrap
        }
        Label {
            Layout.fillWidth: true
            visible: text !== ""
            text: row.description
            opacity: 0.6
            font.pixelSize: 12
            wrapMode: Text.Wrap
        }
    }
    Item {
        id: holder
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        width: row.controlWidth
        height: childrenRect.height
    }
}
