import QtQuick
import QtQuick.Controls

// Lobbies for one game: make one (a room with a code, an optional password) or join one from the list or by its code.
// Online Practice (practice: true) is the same with a private room for two, which a friend joins by its code alone.
// Once in a lobby, its room screen takes over.
Item {
    id: root
    property var app
    property var game: ({})
    property bool practice: false
    property int row: 0
    readonly property var shown: online.lobbies.filter(l => l.game === root.game.id && l.mode === "kombat")
    readonly property int actions: practice ? 2 : 3
    readonly property int rows: actions + (practice ? 0 : shown.length)
    readonly property var prompts: [{action: "accept", text: "Select"}, {action: "option", text: "Refresh"},
                                    {action: "back", text: "Back"}]

    function refresh() { if (!practice) online.refreshLobbies(root.game.id) }
    Component.onCompleted: refresh()
    StackView.onActivated: refresh()
    Timer { interval: 5000; repeat: true; running: root.StackView.status === StackView.Active; onTriggered: root.refresh() }

    Connections {
        target: online
        function onLobbyChanged() {
            if (root.StackView.status === StackView.Active && online.lobby.id)
                root.app.push("lobby", {game: root.game})
        }
    }

    function create(password) {
        online.createLobby(root.game.id, root.practice ? "sparring" : "kombat", password)
    }
    function joinByCode() {
        app.keyboard("Lobby code (5 letters and digits)", "", false, 5, (code) => {
            if (code.trim().length === 5) online.joinLobby(code.trim(), "")
            else app.toast("A lobby code has 5 letters and digits")
        })
    }
    function activate(i) {
        if (practice) {
            if (i === 0) create("")
            else joinByCode()
            return
        }
        if (i === 0) create("")
        else if (i === 1)
            app.keyboard("Password for your lobby", "", false, 32, (pw) => create(pw), {keepCase: true})
        else if (i === 2) joinByCode()
        else {
            const l = shown[i - actions]
            if (!l) return
            if (l.locked)
                app.keyboard("Password for " + l.name, "", false, 32, (pw) => online.joinLobby(l.id, pw), {keepCase: true})
            else online.joinLobby(l.id, "")
        }
    }
    function nav(a) {
        if (a === "up") row = (row + rows - 1) % rows
        else if (a === "down") row = (row + 1) % rows
        else if (a === "back") app.pop()
        else if (a === "option") { refresh(); app.toast("Lobbies refreshed") }
        else if (a === "accept" || a === "start") activate(row)
    }

    Column {
        anchors.left: parent.left
        anchors.leftMargin: Theme.px(96)
        anchors.top: parent.top
        anchors.topMargin: Theme.px(70)
        spacing: Theme.px(6)
        Text {
            text: (root.game.title || "").toUpperCase() + "  ·  ONLINE"
            color: Theme.dim
            font.family: Theme.font
            font.pixelSize: Theme.px(22)
            font.weight: Font.Bold
            font.letterSpacing: Theme.px(4)
        }
        GoldText {
            text: root.practice ? "ONLINE PRACTICE" : "LOBBIES"
            pixelSize: Theme.px(84)
            weight: Font.Black
            spacing: Theme.px(8)
        }
    }
    AccountChip {
        anchors.right: parent.right
        anchors.rightMargin: Theme.px(96)
        anchors.top: parent.top
        anchors.topMargin: Theme.px(96)
    }

    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: Theme.px(260)
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Theme.px(120)
        width: Theme.px(1240)
        radius: Theme.px(12)
        color: "#e0110d0c"
        border.color: Theme.line
        clip: true
        Flickable {
            id: flick
            anchors.fill: parent
            anchors.margins: Theme.px(30)
            contentHeight: list.implicitHeight
            interactive: false
            contentY: Math.max(0, Math.min(contentHeight - height, root.row * Theme.px(78) - height / 2))
            Behavior on contentY { NumberAnimation { duration: 120 } }
            Column {
                id: list
                width: flick.width
                spacing: Theme.px(8)
                Repeater {
                    model: root.practice ? ["Make a practice room", "Join a friend's room by code"]
                                         : ["Make a lobby", "Make a lobby with a password", "Join by code"]
                    SettingRow {
                        required property string modelData
                        required property int index
                        current: root.row === index
                        label: modelData
                        kind: "action"
                    }
                }
                Text {
                    visible: !root.practice
                    topPadding: Theme.px(16)
                    bottomPadding: Theme.px(6)
                    text: root.shown.length ? "OPEN LOBBIES" : "NO OPEN LOBBIES FOR THIS GAME RIGHT NOW"
                    color: Theme.dim
                    font.family: Theme.font
                    font.pixelSize: Theme.px(20)
                    font.weight: Font.Bold
                    font.letterSpacing: Theme.px(3)
                }
                Repeater {
                    model: root.practice ? [] : root.shown
                    SettingRow {
                        required property var modelData
                        required property int index
                        current: root.row === index + root.actions
                        label: (modelData.locked ? "🔒 " : "") + modelData.name
                        kind: "info"
                        value: modelData.id + "   ·   " + modelData.players + "/" + modelData.max_players +
                               (modelData.state === "in_match" ? "   ·   IN A MATCH" : "") +
                               "   ·   " + String(modelData.region || "").toUpperCase()
                    }
                }
                Text {
                    width: list.width
                    visible: root.practice
                    topPadding: Theme.px(20)
                    wrapMode: Text.WordWrap
                    text: "A practice room is for you and one friend: give them its code. You both pick on the select " +
                          "screen and fight for as long as you like; nothing counts toward your record."
                    color: Theme.dim
                    font.family: Theme.font
                    font.pixelSize: Theme.px(20)
                }
            }
        }
    }
}
