import QtQuick
import QtQuick.Controls

// A scrolling settings page with a heading.
ScrollView {
    id: page
    property string heading
    property string subheading
    default property alias content: column.data
    contentWidth: availableWidth
    clip: true

    Column {
        id: column
        x: 28
        width: page.availableWidth - 56
        topPadding: 22
        bottomPadding: 28
        spacing: 0
        Label {
            width: parent.width
            text: page.heading
            font.pixelSize: 24
            font.weight: Font.DemiBold
        }
        Label {
            width: parent.width
            visible: text !== ""
            text: page.subheading
            opacity: 0.6
            wrapMode: Text.Wrap
            topPadding: 4
            bottomPadding: 6
        }
    }
}
