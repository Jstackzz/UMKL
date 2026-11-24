import QtQuick

// The row of button hints along the bottom: [{action: "accept", text: "Select"}, ...]
Item {
    id: root
    property var items: []
    height: Theme.px(70)

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#00000000" }
            GradientStop { position: 1.0; color: "#cc000000" }
        }
    }
    Row {
        anchors.right: parent.right
        anchors.rightMargin: Theme.px(64)
        anchors.verticalCenter: parent.verticalCenter
        spacing: Theme.px(34)
        Repeater {
            model: root.items
            Row {
                spacing: Theme.px(10)
                ButtonGlyph {
                    action: modelData.action
                    size: Theme.px(32)
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text: modelData.text.toUpperCase()
                    color: Theme.text
                    font.family: Theme.font
                    font.pixelSize: Theme.px(20)
                    font.weight: Font.DemiBold
                    font.letterSpacing: Theme.px(1.5)
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }
    }
}
