import QtQuick
import QtQuick.Effects

// A large choice in a hub screen (RANKED, KASUAL, LOBBIES...): a symbol, a title, a line about it and a status.
Item {
    id: root
    property string symbol
    property string title
    property string description
    property string status
    property bool current: false
    property bool locked: false
    width: Theme.px(520)
    height: Theme.px(230)
    scale: current ? 1.03 : 1.0
    Behavior on scale { NumberAnimation { duration: 160; easing.type: Easing.OutQuad } }

    Rectangle {
        id: box
        anchors.fill: parent
        radius: Theme.px(10)
        border.width: root.current ? Theme.px(3) : Theme.px(1)
        border.color: root.current ? Theme.gold : Theme.line
        gradient: Gradient {
            GradientStop { position: 0.0; color: root.current ? "#2a1512" : "#16110f" }
            GradientStop { position: 1.0; color: root.current ? "#170909" : "#0d0a09" }
        }
        opacity: 0.94
    }
    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.margins: Theme.px(2)
        width: Theme.px(6)
        radius: Theme.px(3)
        color: root.current ? Theme.red : "transparent"
    }
    Text {
        id: sym
        anchors.left: parent.left
        anchors.leftMargin: Theme.px(34)
        anchors.verticalCenter: parent.verticalCenter
        text: root.symbol
        color: root.locked ? Theme.faint : (root.current ? Theme.gold : Theme.goldDeep)
        font.pixelSize: Theme.px(74)
        font.family: "Segoe UI Symbol"
    }
    Column {
        anchors.left: sym.right
        anchors.leftMargin: Theme.px(30)
        anchors.right: parent.right
        anchors.rightMargin: Theme.px(24)
        anchors.verticalCenter: parent.verticalCenter
        spacing: Theme.px(8)
        Text {
            text: root.title
            color: root.locked ? Theme.dim : Theme.text
            font.family: Theme.font
            font.pixelSize: Theme.px(36)
            font.weight: Font.Bold
            font.letterSpacing: Theme.px(3)
        }
        Text {
            width: parent.width
            text: root.description
            wrapMode: Text.WordWrap
            color: Theme.dim
            font.family: Theme.font
            font.pixelSize: Theme.px(19)
        }
        Text {
            visible: root.status !== ""
            width: parent.width
            text: root.status
            wrapMode: Text.WordWrap
            color: root.locked ? "#c98b4b" : Theme.good
            font.family: Theme.font
            font.pixelSize: Theme.px(16)
            font.weight: Font.DemiBold
            font.letterSpacing: Theme.px(1)
        }
    }
}
