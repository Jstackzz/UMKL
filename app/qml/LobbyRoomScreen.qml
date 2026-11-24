import QtQuick
import QtQuick.Controls

// Inside a lobby: its code, who is in it and who is ready, the chat. When both players are ready the server counts
// down three seconds and the match starts (the game screen comes up by itself, and goes again after the match).
Item {
    id: root
    property var app
    property var game: ({})
    property int index: 0
    readonly property var lobby: online.lobby
    readonly property bool practice: lobby.mode === "sparring"
    readonly property var members: lobby.members || []
    readonly property var chatLines: lobby.chat || []
    readonly property var buttons: [online.lobbyReady ? "NOT READY" : "READY", "CHAT", "LEAVE"]
    readonly property var prompts: [{action: "accept", text: "Select"}, {action: "back", text: "Leave"}]

    // the lobby is gone (left, or the server closed it): back to the list
    Connections {
        target: online
        function onLobbyChanged() {
            if (!online.lobby.id && root.StackView.status === StackView.Active) root.app.pop()
        }
    }

    function nav(a) {
        if (a === "left") index = (index + buttons.length - 1) % buttons.length
        else if (a === "right") index = (index + 1) % buttons.length
        else if (a === "back") online.leaveLobby()
        else if (a === "accept" || a === "start") {
            if (index === 0) online.setReady(!online.lobbyReady)
            else if (index === 1) app.keyboard("Say something", "", false, 120, (t) => online.chat(t), {keepCase: true})
            else online.leaveLobby()
        }
    }

    Column {
        anchors.left: parent.left
        anchors.leftMargin: Theme.px(96)
        anchors.top: parent.top
        anchors.topMargin: Theme.px(70)
        spacing: Theme.px(6)
        Text {
            text: (root.game.title || "").toUpperCase() + "  ·  " + (root.practice ? "ONLINE PRACTICE" : "LOBBY")
            color: Theme.dim
            font.family: Theme.font
            font.pixelSize: Theme.px(22)
            font.weight: Font.Bold
            font.letterSpacing: Theme.px(4)
        }
        GoldText {
            text: (root.lobby.name || "LOBBY").toUpperCase()
            pixelSize: Theme.px(68)
            weight: Font.Black
            spacing: Theme.px(5)
        }
    }
    // the code, to give a friend
    Rectangle {
        anchors.right: parent.right
        anchors.rightMargin: Theme.px(96)
        anchors.top: parent.top
        anchors.topMargin: Theme.px(80)
        width: codeCol.implicitWidth + Theme.px(60)
        height: codeCol.implicitHeight + Theme.px(30)
        radius: Theme.px(10)
        color: "#e0110d0c"
        border.color: Theme.gold
        border.width: Theme.px(2)
        Column {
            id: codeCol
            anchors.centerIn: parent
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "CODE"
                color: Theme.dim
                font.family: Theme.font
                font.pixelSize: Theme.px(18)
                font.weight: Font.Bold
                font.letterSpacing: Theme.px(4)
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: root.lobby.id || ""
                color: Theme.goldLight
                font.family: Theme.font
                font.pixelSize: Theme.px(52)
                font.weight: Font.Black
                font.letterSpacing: Theme.px(8)
            }
        }
    }

    Row {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: Theme.px(250)
        spacing: Theme.px(30)

        // players
        Rectangle {
            width: Theme.px(700)
            height: Theme.px(560)
            radius: Theme.px(12)
            color: "#e0110d0c"
            border.color: Theme.line
            Column {
                anchors.fill: parent
                anchors.margins: Theme.px(30)
                spacing: Theme.px(14)
                Text {
                    text: root.lobby.starts_in !== undefined && root.lobby.starts_in !== null
                          ? "THE MATCH STARTS IN " + Math.ceil(root.lobby.starts_in)
                          : root.lobby.state === "in_match" ? "A MATCH IS ON"
                          : root.members.filter(m => !m.spectator).length < 2 ? "WAITING FOR A SECOND PLAYER"
                          : "BOTH READY STARTS THE MATCH"
                    color: Theme.gold
                    font.family: Theme.font
                    font.pixelSize: Theme.px(24)
                    font.weight: Font.Bold
                    font.letterSpacing: Theme.px(2)
                }
                Repeater {
                    model: root.members
                    Rectangle {
                        required property var modelData
                        width: parent.width
                        height: Theme.px(84)
                        radius: Theme.px(8)
                        color: modelData.name === online.account ? "#2a1714" : "#1a1412"
                        border.color: modelData.ready ? Theme.good : Theme.line
                        border.width: Theme.px(2)
                        Column {
                            anchors.left: parent.left
                            anchors.leftMargin: Theme.px(22)
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: Theme.px(2)
                            Text {
                                text: modelData.name.toUpperCase() + (modelData.host ? "   ♛" : "")
                                color: Theme.text
                                font.family: Theme.font
                                font.pixelSize: Theme.px(28)
                                font.weight: Font.Bold
                                font.letterSpacing: Theme.px(2)
                            }
                            Text {
                                text: (modelData.title || "").toUpperCase() +
                                      (modelData.ping !== undefined && modelData.ping !== null ? "   ·   PING " + modelData.ping + " MS" : "")
                                color: Theme.dim
                                font.family: Theme.font
                                font.pixelSize: Theme.px(17)
                                font.weight: Font.DemiBold
                            }
                        }
                        Text {
                            anchors.right: parent.right
                            anchors.rightMargin: Theme.px(22)
                            anchors.verticalCenter: parent.verticalCenter
                            text: modelData.spectator ? "WATCHING" : modelData.ready ? "READY" : "NOT READY"
                            color: modelData.ready ? Theme.good : Theme.faint
                            font.family: Theme.font
                            font.pixelSize: Theme.px(22)
                            font.weight: Font.Black
                            font.letterSpacing: Theme.px(2)
                        }
                    }
                }
            }
        }

        // chat
        Rectangle {
            width: Theme.px(560)
            height: Theme.px(560)
            radius: Theme.px(12)
            color: "#e0110d0c"
            border.color: Theme.line
            clip: true
            Text {
                id: chatTitle
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.margins: Theme.px(30)
                text: "CHAT"
                color: Theme.dim
                font.family: Theme.font
                font.pixelSize: Theme.px(20)
                font.weight: Font.Bold
                font.letterSpacing: Theme.px(3)
            }
            Column {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: Theme.px(30)
                spacing: Theme.px(6)
                Repeater {
                    model: root.chatLines.slice(-12)
                    Text {
                        required property var modelData
                        width: parent.width
                        wrapMode: Text.WordWrap
                        text: "<b>" + modelData.from + "</b>  " + modelData.text.replace(/</g, "&lt;")
                        textFormat: Text.StyledText
                        color: Theme.text
                        font.family: Theme.font
                        font.pixelSize: Theme.px(19)
                    }
                }
            }
        }
    }

    Row {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Theme.px(120)
        spacing: Theme.px(24)
        Repeater {
            model: root.buttons
            ModeButton {
                required property string modelData
                required property int index
                label: modelData
                current: index === root.index
            }
        }
    }
}
