import QtQuick

// A short message near the bottom that fades away by itself.
Item {
    id: root
    anchors.horizontalCenter: parent.horizontalCenter
    anchors.bottom: parent.bottom
    anchors.bottomMargin: Theme.px(120)
    width: Math.min(parent.width * 0.7, label.implicitWidth + Theme.px(60))
    height: Theme.px(64)
    opacity: 0
    visible: opacity > 0

    function show(text) {
        label.text = text
        anim.restart()
    }

    Rectangle {
        anchors.fill: parent
        radius: Theme.px(8)
        color: "#e6100c0b"
        border.color: Theme.gold
        border.width: Theme.px(1)
    }
    Text {
        id: label
        anchors.centerIn: parent
        width: parent.width - Theme.px(40)
        horizontalAlignment: Text.AlignHCenter
        elide: Text.ElideRight
        color: Theme.text
        font.family: Theme.font
        font.pixelSize: Theme.px(22)
        font.weight: Font.DemiBold
    }
    SequentialAnimation {
        id: anim
        NumberAnimation { target: root; property: "opacity"; to: 1; duration: 160 }
        PauseAnimation { duration: 2600 }
        NumberAnimation { target: root; property: "opacity"; to: 0; duration: 500 }
    }
}
