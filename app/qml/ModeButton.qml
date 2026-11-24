import QtQuick

// A mode in a row of them (ARCADE, VERSUS, ONLINE...): red and gold when selected.
Item {
    id: root
    property string label
    property string tag        // a small corner tag: "SOON", "2P"
    property bool current: false
    property bool locked: false
    width: Math.max(Theme.px(190), text.implicitWidth + Theme.px(56))
    height: Theme.px(66)

    Rectangle {
        anchors.fill: parent
        radius: Theme.px(6)
        border.width: root.current ? Theme.px(2) : Theme.px(1)
        border.color: root.current ? Theme.gold : Theme.line
        gradient: Gradient {
            GradientStop { position: 0.0; color: root.current ? "#b3161c" : "#1a1412" }
            GradientStop { position: 1.0; color: root.current ? "#5a090c" : "#100c0b" }
        }
        opacity: root.locked && !root.current ? 0.6 : 1
    }
    Rectangle {
        // a bright edge along the top of the selected one
        visible: root.current
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: Theme.px(2)
        height: Theme.px(2)
        color: Theme.goldLight
        opacity: 0.6
    }
    Text {
        id: text
        anchors.centerIn: parent
        text: root.label
        color: root.current ? "white" : (root.locked ? Theme.faint : Theme.text)
        font.family: Theme.font
        font.pixelSize: Theme.px(24)
        font.weight: Font.Bold
        font.letterSpacing: Theme.px(2.5)
    }
    Rectangle {
        visible: root.tag !== ""
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.rightMargin: -Theme.px(8)
        anchors.topMargin: -Theme.px(10)
        width: tagText.implicitWidth + Theme.px(12)
        height: Theme.px(22)
        radius: Theme.px(3)
        color: root.locked ? "#4a3f36" : Theme.gold
        Text {
            id: tagText
            anchors.centerIn: parent
            text: root.tag
            color: root.locked ? Theme.dim : "#1a0f05"
            font.family: Theme.font
            font.pixelSize: Theme.px(13)
            font.weight: Font.Bold
            font.letterSpacing: Theme.px(1)
        }
    }
    scale: root.current ? 1.06 : 1.0
    Behavior on scale { NumberAnimation { duration: 140; easing.type: Easing.OutQuad } }
}
