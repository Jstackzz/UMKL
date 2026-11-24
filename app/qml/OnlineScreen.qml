import QtQuick
import QtQuick.Controls

// The online hub for one game, on the MK3D server (Oracle Cloud): Ranked, Kasual, lobbies, Online Practice and the
// leaderboard need an account; Direct Connect plays by address with no server. Y signs out.
Item {
    id: root
    property var app
    property var game: ({})
    property int index: 0
    readonly property bool signedIn: online.state === "signedIn"
    readonly property var counts: online.gameCounts[root.game.id] || ({})
    readonly property var tiles: [
        {id: "ranked", symbol: "♛", title: "RANKED", description: "First to 3 wins takes the set. Your Elo gives your rank, from Beginner up.",
         status: (counts.searching || 0) + " SEARCHING  ·  " + (counts.matches || 0) + " PLAYING"},
        {id: "kasual", symbol: "⚔", title: "KASUAL", description: "Quick matches against anyone, with rematches.",
         status: (counts.searching || 0) + " SEARCHING  ·  " + (counts.matches || 0) + " PLAYING"},
        {id: "lobbies", symbol: "⌂", title: "LOBBIES", description: "Make a room or join one: play friends by code, with chat.",
         status: (counts.lobbies || 0) + " IN LOBBIES"},
        {id: "practice", symbol: "✦", title: "ONLINE PRACTICE", description: "A private room for two: fight as long as you like, no records kept.",
         status: "BY CODE"},
        {id: "direct", symbol: "⇄", title: "DIRECT CONNECT", description: "Host or join by address. Rollback netcode, no server.",
         status: "NO ACCOUNT NEEDED"},
        {id: "leaderboard", symbol: "☰", title: "LEADERBOARD", description: "Everyone's rank and record in this game.",
         status: ""}
    ]
    readonly property var prompts: signedIn
        ? [{action: "accept", text: "Select"}, {action: "option", text: "Sign out"}, {action: "back", text: "Back"}]
        : [{action: "accept", text: "Select"}, {action: "back", text: "Back"}]

    StackView.onActivated: online.connectServer()
    Component.onCompleted: online.connectServer()

    function open(id) {
        if (id === "direct") { app.push("direct", {game: root.game}); return }
        const props = {game: root.game}
        let screen = "lobbies"
        if (id === "ranked" || id === "kasual") { screen = "match"; props.ranked = id === "ranked" }
        else if (id === "practice") props.practice = true
        else if (id === "leaderboard") screen = "board"
        if (!signedIn) app.push("signin", {next: screen, nextProps: props})
        else app.push(screen, props)
    }
    function nav(a) {
        if (a === "left" && index % 2 === 1) index--
        else if (a === "right" && index % 2 === 0) index++
        else if (a === "up" && index >= 2) index -= 2
        else if (a === "down" && index + 2 < tiles.length) index += 2
        else if (a === "back") app.pop()
        else if (a === "option" && signedIn) { online.signOut(); app.toast("Signed out") }
        else if (a === "accept" || a === "start") open(tiles[index].id)
    }

    Column {
        anchors.left: parent.left
        anchors.leftMargin: Theme.px(96)
        anchors.top: parent.top
        anchors.topMargin: Theme.px(70)
        spacing: Theme.px(6)
        Text {
            text: (root.game.title || "").toUpperCase()
            color: Theme.dim
            font.family: Theme.font
            font.pixelSize: Theme.px(22)
            font.weight: Font.Bold
            font.letterSpacing: Theme.px(4)
        }
        GoldText {
            text: "ONLINE"
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

    Grid {
        anchors.centerIn: parent
        anchors.verticalCenterOffset: Theme.px(60)
        columns: 2
        spacing: Theme.px(32)
        Repeater {
            model: root.tiles
            Tile {
                required property var modelData
                required property int index
                symbol: modelData.symbol
                title: modelData.title
                description: modelData.description
                status: modelData.id === "direct" || root.signedIn ? modelData.status : "SIGN IN TO PLAY"
                locked: modelData.id !== "direct" && !root.signedIn
                current: index === root.index
            }
        }
    }
}
