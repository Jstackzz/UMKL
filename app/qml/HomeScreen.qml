import QtQuick
import QtQuick.Controls

// The main menu: the four games in a carousel (the selected one's attract mode plays in its card and behind
// everything), and under them what to do with it.
Item {
    id: root
    property var app
    property int gameIndex: 3
    property int row: 0        // 0 the carousel, 1 the modes
    property int modeIndex: 0
    readonly property var games: library.games
    readonly property var game: games.length ? games[gameIndex] : ({})

    readonly property var modes: [
        {id: "arcade", label: "ARCADE", tag: "1P"},
        {id: "versus", label: "VERSUS", tag: "2P"},
        {id: "online", label: "ONLINE", tag: ""},
        {id: "training", label: "TRAINING", tag: "SOON", locked: true},
        {id: "jukebox", label: "JUKEBOX", tag: "SOON", locked: true},
        {id: "settings", label: "SETTINGS", tag: ""},
        {id: "quit", label: "QUIT", tag: ""}
    ]
    readonly property var prompts: row === 0
        ? [{action: "dpad", text: "Game"}, {action: "accept", text: "Choose"}, {action: "option", text: "Settings"},
           {action: "back", text: "Title"}]
        : [{action: "tabLeft", text: "Prev game"}, {action: "tabRight", text: "Next game"},
           {action: "accept", text: "Select"}, {action: "back", text: "Games"}]

    function selectGame(i) {
        if (!games.length) return
        gameIndex = (i + games.length) % games.length
        app.preview(game.ready ? game.set : "")
    }
    function activate(m) {
        if (m.locked) { app.toast(m.label + " is coming in the next build"); return }
        if (m.id === "settings") { app.push("settings", {}); return }
        if (m.id === "quit") { app.confirmQuit(); return }
        if (!game.ready) { app.toast(game.status); return }
        if (m.id === "arcade" || m.id === "versus") app.startGame(game.set, m.id)
        else if (m.id === "online") app.push("online", {game: game})
    }
    function nav(a) {
        if (a === "tabLeft") { selectGame(gameIndex - 1); return }
        if (a === "tabRight") { selectGame(gameIndex + 1); return }
        if (a === "option") { app.push("settings", {}); return }
        if (row === 0) {
            if (a === "left") selectGame(gameIndex - 1)
            else if (a === "right") selectGame(gameIndex + 1)
            else if (a === "down" || a === "accept" || a === "start") row = 1
            else if (a === "back") app.pop()
        } else {
            if (a === "left") modeIndex = (modeIndex + modes.length - 1) % modes.length
            else if (a === "right") modeIndex = (modeIndex + 1) % modes.length
            else if (a === "up" || a === "back") row = 0
            else if (a === "accept" || a === "start") activate(modes[modeIndex])
        }
    }
    StackView.onActivated: app.preview(game.ready ? game.set : "")
    Component.onCompleted: {
        // start on the last ready game (UMK3 when all four are there)
        for (let i = games.length - 1; i >= 0; i--)
            if (games[i].ready) { gameIndex = i; break }
    }

    // --- top bar
    Row {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.leftMargin: Theme.px(64)
        anchors.topMargin: Theme.px(40)
        spacing: Theme.px(16)
        GoldText {
            text: "MK"
            pixelSize: Theme.px(46)
            weight: Font.Black
            spacing: Theme.px(4)
            glow: false
        }
        Text {
            text: "ARCADE ONLINE"
            color: Theme.text
            font.family: Theme.font
            font.pixelSize: Theme.px(22)
            font.weight: Font.Bold
            font.letterSpacing: Theme.px(6)
            anchors.verticalCenter: parent.verticalCenter
        }
    }
    Row {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.rightMargin: Theme.px(64)
        anchors.topMargin: Theme.px(48)
        spacing: Theme.px(28)
        Text {
            text: "🎮 " + (input.padCount > 0 ? input.padCount + (input.padCount > 1 ? " CONTROLLERS" : " CONTROLLER")
                                               : "KEYBOARD")
            color: Theme.dim
            font.family: Theme.font
            font.pixelSize: Theme.px(18)
            font.weight: Font.DemiBold
            font.letterSpacing: Theme.px(1)
        }
        Text {
            text: appSettings.playerName
            color: Theme.gold
            font.family: Theme.font
            font.pixelSize: Theme.px(20)
            font.weight: Font.Bold
            font.letterSpacing: Theme.px(2)
        }
    }

    // --- the selected game
    Column {
        anchors.left: parent.left
        anchors.leftMargin: Theme.px(96)
        anchors.top: parent.top
        anchors.topMargin: Theme.px(150)
        spacing: Theme.px(10)
        GoldText {
            text: (root.game.title || "").toUpperCase()
            pixelSize: Theme.px(68)
            weight: Font.Black
            spacing: Theme.px(3)
        }
        Text {
            text: (root.game.year || "") + "   ·   " + (root.game.board || "")
            color: Theme.dim
            font.family: Theme.font
            font.pixelSize: Theme.px(22)
            font.weight: Font.DemiBold
            font.letterSpacing: Theme.px(3)
        }
        Text {
            text: root.game.blurb || ""
            color: Theme.text
            font.family: Theme.font
            font.pixelSize: Theme.px(22)
        }
        Text {
            text: root.game.ready ? "●  READY" : "●  " + (root.game.status || "").toUpperCase()
            color: root.game.ready ? Theme.good : Theme.bad
            font.family: Theme.font
            font.pixelSize: Theme.px(17)
            font.weight: Font.Bold
            font.letterSpacing: Theme.px(2)
        }
    }

    // --- the carousel
    ListView {
        id: carousel
        anchors.left: parent.left
        anchors.right: parent.right
        y: Theme.px(400)
        height: Theme.px(420)
        orientation: ListView.Horizontal
        model: root.games
        currentIndex: root.gameIndex
        spacing: -Theme.px(10)
        interactive: false
        highlightMoveDuration: 260
        preferredHighlightBegin: width / 2 - Theme.px(220)
        preferredHighlightEnd: width / 2 + Theme.px(220)
        highlightRangeMode: ListView.StrictlyEnforceRange
        delegate: GameCard {
            required property var modelData
            required property int index
            game: modelData
            current: index === root.gameIndex
            live: current && root.game.ready
            artRevision: root.app ? root.app.artRevision : 0
        }
    }

    // --- the modes
    Row {
        id: modeRow
        anchors.horizontalCenter: parent.horizontalCenter
        y: Theme.px(880)
        spacing: Theme.px(22)
        opacity: root.row === 1 ? 1 : 0.75
        Repeater {
            model: root.modes
            ModeButton {
                required property var modelData
                required property int index
                label: modelData.label
                tag: modelData.tag
                locked: !!modelData.locked
                current: root.row === 1 && index === root.modeIndex
            }
        }
    }
}
