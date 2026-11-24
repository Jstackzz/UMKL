import QtQuick

// Where online play stands, in a corner of the online screens: the server's state, who is signed in, how many are on.
Rectangle {
    id: root
    width: row.implicitWidth + Theme.px(36)
    height: Theme.px(52)
    radius: Theme.px(26)
    color: "#d0100c0b"
    border.color: online.state === "signedIn" ? Theme.goldDeep : Theme.line
    border.width: Theme.px(2)
    Row {
        id: row
        anchors.centerIn: parent
        spacing: Theme.px(12)
        Rectangle {
            width: Theme.px(14)
            height: width
            radius: width / 2
            anchors.verticalCenter: parent.verticalCenter
            color: online.state === "signedIn" ? Theme.good : online.state === "offline" ? Theme.bad : Theme.gold
        }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: online.state === "signedIn" ? online.account.toUpperCase()
                : online.state === "online" ? "NOT SIGNED IN"
                : online.state === "connecting" ? "CONNECTING..."
                : "OFFLINE"
            color: Theme.text
            font.family: Theme.font
            font.pixelSize: Theme.px(20)
            font.weight: Font.Bold
            font.letterSpacing: Theme.px(2)
        }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            visible: online.state === "signedIn" || online.state === "online"
            text: "·   " + online.onlineCount + " ONLINE"
            color: Theme.dim
            font.family: Theme.font
            font.pixelSize: Theme.px(18)
            font.weight: Font.DemiBold
            font.letterSpacing: Theme.px(1)
        }
    }
}
