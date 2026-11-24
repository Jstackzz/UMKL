import QtQuick
import QtQuick.Controls
import MKArcade.Native

// A game being played: the picture in the arcade monitor's shape, a pause menu (BACK / VIEW or Esc), and online
// the connection's state and ping.
Item {
    id: root
    property var app
    property string set
    property string mode: "arcade"
    property bool menuOpen: false
    property int menuIndex: 0
    property string endReason: ""
    // net: Direct Connect; link: a match the server set up (it goes back to its screen by itself once over)
    readonly property bool link: mode === "link"
    readonly property bool net: mode === "net" || link
    readonly property bool linkOver: link && !emu.linkActive && endReason === ""
    readonly property bool waiting: net && emu.netStatus !== "Online" && endReason === "" && !linkOver
    readonly property var menuItems: net ? ["RESUME", "LEAVE MATCH"] : ["RESUME", "RESTART", "QUIT TO MENU"]
    readonly property var prompts: []

    function syncInput() { input.gameMode = StackView.status === StackView.Active && !menuOpen && endReason === "" }
    onMenuOpenChanged: { if (!net) emu.setPaused(menuOpen); syncInput() }
    StackView.onActivated: syncInput()
    StackView.onDeactivated: input.gameMode = false

    function pauseRequested() { if (endReason === "") { menuOpen = !menuOpen; menuIndex = 0 } }
    // leaving a server's match: in a lobby, not ready for its next one (Online Practice's would start by itself)
    function leave() {
        if (link) online.standDown()
        app.leaveGame()
    }
    function choose(i) {
        const item = menuItems[i]
        if (item === "RESUME") menuOpen = false
        else if (item === "RESTART") { menuOpen = false; emu.restart() }
        else leave()
    }
    function nav(a) {
        if (endReason !== "") { if (a === "accept" || a === "back" || a === "start") app.leaveGame(); return }
        if (waiting && a === "back") { leave(); return }
        if (!menuOpen) return
        if (a === "up") menuIndex = (menuIndex + menuItems.length - 1) % menuItems.length
        else if (a === "down") menuIndex = (menuIndex + 1) % menuItems.length
        else if (a === "accept") choose(menuIndex)
        else if (a === "back" || a === "start") menuOpen = false
    }
    Connections {
        target: emu
        function onEnded(reason) {
            root.endReason = reason
            root.menuOpen = false
            input.gameMode = false
        }
    }

    Rectangle { anchors.fill: parent; color: "black" }

    GameView {
        id: view
        session: emu
        filtered: appSettings.smooth
        readonly property real aspect: appSettings.scaleMode === "stretch" ? root.width / root.height
                                     : appSettings.scaleMode === "pixel" ? sourceWidth / sourceHeight : 4 / 3
        readonly property int whole: Math.max(1, Math.floor(Math.min(root.width / sourceWidth, root.height / sourceHeight)))
        width: appSettings.scaleMode === "pixel" ? sourceWidth * whole : Math.min(root.width, root.height * aspect)
        height: appSettings.scaleMode === "pixel" ? sourceHeight * whole : width / aspect
        anchors.centerIn: parent
    }

    // the first seconds of a local game: how to join
    Rectangle {
        id: hint
        visible: !root.net && opacity > 0
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Theme.px(60)
        width: hintRow.implicitWidth + Theme.px(48)
        height: Theme.px(60)
        radius: Theme.px(8)
        color: "#d8100c0b"
        border.color: Theme.gold
        Row {
            id: hintRow
            anchors.centerIn: parent
            spacing: Theme.px(12)
            ButtonGlyph { action: "start"; size: Theme.px(32); anchors.verticalCenter: parent.verticalCenter }
            Text {
                text: root.mode === "versus" ? "BOTH PLAYERS: PRESS START TO JOIN" : "PRESS START TO PLAY"
                color: Theme.text
                font.family: Theme.font
                font.pixelSize: Theme.px(22)
                font.weight: Font.Bold
                font.letterSpacing: Theme.px(2)
                anchors.verticalCenter: parent.verticalCenter
            }
            Text {
                text: "·   PAUSE"
                color: Theme.dim
                font.family: Theme.font
                font.pixelSize: Theme.px(20)
                font.weight: Font.DemiBold
                anchors.verticalCenter: parent.verticalCenter
            }
            ButtonGlyph { action: "pause"; size: Theme.px(32); anchors.verticalCenter: parent.verticalCenter }
        }
        SequentialAnimation on opacity {
            running: true
            PauseAnimation { duration: 7000 }
            NumberAnimation { to: 0; duration: 800 }
        }
    }

    // a server's match: who plays whom
    Rectangle {
        visible: root.link && emu.linkNames !== "" && !root.waiting
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: Theme.px(18)
        width: namesText.implicitWidth + Theme.px(30)
        height: Theme.px(36)
        radius: Theme.px(5)
        color: "#a0000000"
        Text {
            id: namesText
            anchors.centerIn: parent
            text: emu.linkNames.toUpperCase()
            color: Theme.goldLight
            font.family: Theme.font
            font.pixelSize: Theme.px(17)
            font.weight: Font.Bold
            font.letterSpacing: Theme.px(2)
        }
    }
    // a server's match whose game has stopped after its result: the screen goes once the server has it
    Rectangle {
        visible: root.linkOver
        anchors.centerIn: parent
        width: overText.implicitWidth + Theme.px(60)
        height: Theme.px(80)
        radius: Theme.px(8)
        color: "#d0000000"
        border.color: Theme.gold
        GoldText {
            id: overText
            anchors.centerIn: parent
            text: "MATCH OVER"
            pixelSize: Theme.px(44)
            weight: Font.Black
            spacing: Theme.px(4)
        }
    }

    // online: connection state and ping
    Rectangle {
        visible: root.net && !root.waiting
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: Theme.px(18)
        width: netText.implicitWidth + Theme.px(26)
        height: Theme.px(36)
        radius: Theme.px(5)
        color: "#a0000000"
        Text {
            id: netText
            anchors.centerIn: parent
            text: "PING " + emu.netPing + " MS   ·   ROLLBACK " + emu.netRollback
            color: emu.netPing > 120 ? Theme.bad : Theme.dim
            font.family: Theme.font
            font.pixelSize: Theme.px(16)
            font.weight: Font.DemiBold
            font.letterSpacing: Theme.px(1)
        }
    }

    // online: waiting for the other player
    Rectangle {
        anchors.fill: parent
        visible: root.waiting
        color: "#c8000000"
        Column {
            anchors.centerIn: parent
            spacing: Theme.px(22)
            GoldText {
                anchors.horizontalCenter: parent.horizontalCenter
                text: emu.netStatus === "Loading the match" ? "LOADING THE MATCH" : "WAITING FOR THE OTHER PLAYER"
                pixelSize: Theme.px(52)
                weight: Font.Black
                spacing: Theme.px(4)
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: root.link ? emu.linkNames.toUpperCase() : (emu.netStatus || "Connecting").toUpperCase()
                color: Theme.dim
                font.family: Theme.font
                font.pixelSize: Theme.px(24)
                font.weight: Font.DemiBold
                font.letterSpacing: Theme.px(3)
            }
            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: Theme.px(10)
                ButtonGlyph { action: "back"; size: Theme.px(32) }
                Text {
                    text: "CANCEL"
                    color: Theme.text
                    font.family: Theme.font
                    font.pixelSize: Theme.px(20)
                    font.weight: Font.Bold
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }
    }

    // the pause menu
    Rectangle {
        anchors.fill: parent
        visible: root.menuOpen
        color: "#b8000000"
        Rectangle {
            anchors.centerIn: parent
            width: Theme.px(560)
            height: menuCol.implicitHeight + Theme.px(70)
            radius: Theme.px(12)
            color: Theme.panel
            border.color: Theme.gold
            border.width: Theme.px(2)
            Column {
                id: menuCol
                anchors.centerIn: parent
                spacing: Theme.px(12)
                GoldText {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: root.net ? "ONLINE MATCH" : "PAUSED"
                    pixelSize: Theme.px(50)
                    weight: Font.Black
                    spacing: Theme.px(6)
                }
                Item { width: 1; height: Theme.px(8) }
                Repeater {
                    model: root.menuItems
                    ModeButton {
                        required property string modelData
                        required property int index
                        anchors.horizontalCenter: parent ? parent.horizontalCenter : undefined
                        width: Theme.px(420)
                        label: modelData
                        current: index === root.menuIndex
                    }
                }
            }
        }
    }

    // the game stopped by itself: an error, or the other player left
    Rectangle {
        anchors.fill: parent
        visible: root.endReason !== ""
        color: "#d0000000"
        Column {
            anchors.centerIn: parent
            spacing: Theme.px(24)
            GoldText {
                anchors.horizontalCenter: parent.horizontalCenter
                text: root.net ? "MATCH OVER" : "THE GAME STOPPED"
                pixelSize: Theme.px(56)
                weight: Font.Black
                spacing: Theme.px(4)
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                width: Math.min(root.width * 0.7, implicitWidth)
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
                text: root.endReason
                color: Theme.text
                font.family: Theme.font
                font.pixelSize: Theme.px(26)
            }
            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: Theme.px(10)
                ButtonGlyph { action: "accept"; size: Theme.px(32) }
                Text {
                    text: "BACK TO THE MENU"
                    color: Theme.text
                    font.family: Theme.font
                    font.pixelSize: Theme.px(20)
                    font.weight: Font.Bold
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }
    }
}
