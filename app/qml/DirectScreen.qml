import QtQuick
import QtQuick.Controls

// Direct connect: one player hosts (player 1), the other joins with the host's address (player 2). Both games
// then run in lockstep with rollback; nothing goes through a server.
Item {
    id: root
    property var app
    property var game: ({})
    property int row: 0
    property int role: 0   // 0 host, 1 join
    readonly property var prompts: [{action: "dpad", text: "Change"}, {action: "accept", text: "Select"},
                                    {action: "back", text: "Back"}]
    readonly property int rows: 5

    function nav(a) {
        if (a === "up") { row = (row + rows - 1) % rows; return }
        if (a === "down") { row = (row + 1) % rows; return }
        if (a === "back") { app.pop(); return }
        const r = rowItems.itemAt(row)
        if (r && r.nav(a)) return
        if (a === "start") start()
    }
    function start() {
        const peer = appSettings.lastPeer.trim()
        if (role === 1 && peer.indexOf(":") < 0) {
            app.toast("Type the host's address as IP:PORT")
            row = 2
            return
        }
        // the host waits for whoever joins: its peer is filled in as the join's address (host:port+1 locally)
        const target = role === 0 ? (peer.length ? peer : "127.0.0.1:" + (appSettings.netPort + 1)) : peer
        app.startNetGame(root.game.set, role === 0 ? 1 : 2, appSettings.netPort, target, appSettings.inputDelay)
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
            text: "DIRECT CONNECT"
            pixelSize: Theme.px(76)
            weight: Font.Black
            spacing: Theme.px(6)
        }
    }

    Rectangle {
        anchors.centerIn: parent
        anchors.verticalCenterOffset: Theme.px(40)
        width: Theme.px(1240)
        height: form.implicitHeight + Theme.px(60)
        radius: Theme.px(12)
        color: "#e0110d0c"
        border.color: Theme.line
        Column {
            id: form
            anchors.centerIn: parent
            width: parent.width - Theme.px(60)
            spacing: Theme.px(8)
            Repeater {
                id: rowItems
                model: 5
                SettingRow {
                    required property int index
                    current: root.row === index
                    label: ["Role", "Your port", "Other player's address", "Input delay", "Start"][index]
                    kind: ["choice", "text", "text", "choice", "action"][index]
                    values: index === 0 ? ["Host (player 1)", "Join (player 2)"]
                          : index === 3 ? ["0 frames", "1 frame", "2 frames", "3 frames", "4 frames", "5 frames", "6 frames"]
                          : []
                    choice: index === 0 ? root.role : index === 3 ? appSettings.inputDelay : 0
                    value: index === 1 ? String(appSettings.netPort)
                         : index === 2 ? (appSettings.lastPeer.length ? appSettings.lastPeer : "IP:PORT")
                         : ""
                    onChanged: (d) => {
                        if (index === 0) root.role = (root.role + 2 + d) % 2
                        else if (index === 3) appSettings.inputDelay = Math.max(0, Math.min(6, appSettings.inputDelay + d))
                    }
                    onActivated: {
                        if (index === 1)
                            root.app.keyboard("Your UDP port", String(appSettings.netPort), true, 5,
                                              (t) => { const p = parseInt(t); if (p > 1023 && p < 65536) appSettings.netPort = p })
                        else if (index === 2)
                            root.app.keyboard("Other player's address (IP:PORT)", appSettings.lastPeer, true, 21,
                                              (t) => appSettings.lastPeer = t.trim())
                        else if (index === 4)
                            root.start()
                    }
                }
            }
            Text {
                width: parent.width
                topPadding: Theme.px(14)
                wrapMode: Text.WordWrap
                text: root.role === 0
                      ? "Hosting: give the other player your public IP address and port " + appSettings.netPort +
                        " (forward that UDP port on your router), and type their address above. On one PC, two copies " +
                        "of the app can play each other on ports " + appSettings.netPort + " and " + (appSettings.netPort + 1) + "."
                      : "Joining: type the host's address as IP:PORT. You play player 2."
                color: Theme.dim
                font.family: Theme.font
                font.pixelSize: Theme.px(19)
            }
        }
    }
}
