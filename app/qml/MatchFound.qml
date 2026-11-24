import QtQuick

// MATCH FOUND, over whatever screen the player is on while a Ranked or Kasual search runs: A accepts, B declines, and
// the time to answer runs down.
Item {
    id: root
    anchors.fill: parent
    visible: online.phase === "found"
    property int index: 0
    readonly property var found: online.found
    readonly property var opponent: found.opponent || ({})
    onVisibleChanged: if (visible) index = 0

    function nav(a) {
        if (a === "left" || a === "right") index = 1 - index
        else if (a === "back") online.decline()
        else if (a === "accept" || a === "start") {
            if (index === 0) online.accept()
            else online.decline()
        }
    }

    Rectangle { anchors.fill: parent; color: "#d8000000" }
    Rectangle {
        anchors.centerIn: parent
        width: Theme.px(860)
        height: col.implicitHeight + Theme.px(80)
        radius: Theme.px(14)
        color: Theme.panel
        border.color: Theme.gold
        border.width: Theme.px(3)
        Column {
            id: col
            anchors.centerIn: parent
            spacing: Theme.px(18)
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: ((root.found.ranked ? "RANKED" : "KASUAL") + "  ·  " +
                       ((library.game(root.found.game || "") || {}).title || "")).toUpperCase()
                color: Theme.dim
                font.family: Theme.font
                font.pixelSize: Theme.px(22)
                font.weight: Font.Bold
                font.letterSpacing: Theme.px(4)
            }
            GoldText {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "MATCH FOUND"
                pixelSize: Theme.px(72)
                weight: Font.Black
                spacing: Theme.px(6)
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: root.opponent.name ? "VS  " + root.opponent.name.toUpperCase() : "AN OPPONENT IS READY"
                color: Theme.text
                font.family: Theme.font
                font.pixelSize: Theme.px(34)
                font.weight: Font.Bold
                font.letterSpacing: Theme.px(2)
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                visible: text !== ""
                text: {
                    const c = root.found.connection || {}
                    const parts = []
                    if (root.opponent.title) parts.push(root.opponent.title.toUpperCase())
                    if (c.estimate !== undefined && c.estimate !== null) parts.push("PING ABOUT " + c.estimate + " MS")
                    else if (c.ping !== undefined && c.ping !== null) parts.push("THEIR PING " + c.ping + " MS")
                    return parts.join("   ·   ")
                }
                color: Theme.dim
                font.family: Theme.font
                font.pixelSize: Theme.px(20)
                font.weight: Font.DemiBold
                font.letterSpacing: Theme.px(2)
            }
            Item { width: 1; height: Theme.px(6) }
            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: Theme.px(24)
                ModeButton { label: "ACCEPT"; current: root.index === 0 }
                ModeButton { label: "DECLINE"; current: root.index === 1 }
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: online.acceptSeconds + " S"
                color: online.acceptSeconds <= 5 ? Theme.bad : Theme.gold
                font.family: Theme.font
                font.pixelSize: Theme.px(26)
                font.weight: Font.Bold
            }
        }
    }
}
