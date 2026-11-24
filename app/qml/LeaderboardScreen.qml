import QtQuick
import QtQuick.Controls

// The ranked leaderboard of one game: rank, player, title, Elo and sets won and lost; your own row stands out, and
// shows at the bottom when you are further down.
Item {
    id: root
    property var app
    property var game: ({})
    property int first: 0   // the first row shown
    readonly property var rows: online.board.game === root.game.id ? (online.board.rows || []) : []
    readonly property var me: online.board.game === root.game.id ? online.board.me : undefined
    readonly property int visibleRows: 10
    readonly property var prompts: [{action: "dpad", text: "Scroll"}, {action: "option", text: "Refresh"},
                                    {action: "back", text: "Back"}]

    Component.onCompleted: online.loadBoard(root.game.id)

    function nav(a) {
        if (a === "up") first = Math.max(0, first - 1)
        else if (a === "down") first = Math.max(0, Math.min(rows.length - visibleRows, first + 1))
        else if (a === "option") online.loadBoard(root.game.id)
        else if (a === "back") app.pop()
    }

    Column {
        anchors.left: parent.left
        anchors.leftMargin: Theme.px(96)
        anchors.top: parent.top
        anchors.topMargin: Theme.px(70)
        spacing: Theme.px(6)
        Text {
            text: (root.game.title || "").toUpperCase() + "  ·  RANKED"
            color: Theme.dim
            font.family: Theme.font
            font.pixelSize: Theme.px(22)
            font.weight: Font.Bold
            font.letterSpacing: Theme.px(4)
        }
        GoldText {
            text: "LEADERBOARD"
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

    component BoardRow: Rectangle {
        property var entry: ({})
        property bool mine: false
        property bool header: false
        width: Theme.px(1240)
        height: Theme.px(56)
        radius: Theme.px(6)
        color: header ? "transparent" : mine ? "#2a1714" : "#1a1412"
        border.color: mine ? Theme.gold : "transparent"
        border.width: Theme.px(2)
        Row {
            anchors.left: parent.left
            anchors.leftMargin: Theme.px(24)
            anchors.verticalCenter: parent.verticalCenter
            Repeater {
                model: [
                    {w: 110, t: header ? "RANK" : "#" + entry.rank},
                    {w: 420, t: header ? "PLAYER" : String(entry.name || online.account).toUpperCase()},
                    {w: 280, t: header ? "TITLE" : String(entry.title || "").toUpperCase()},
                    {w: 180, t: header ? "ELO" : String(entry.elo)},
                    {w: 200, t: header ? "SETS W–L" : entry.wins + " – " + entry.losses}
                ]
                Text {
                    required property var modelData
                    width: Theme.px(modelData.w)
                    text: modelData.t
                    elide: Text.ElideRight
                    color: header ? Theme.dim : Theme.text
                    font.family: Theme.font
                    font.pixelSize: header ? Theme.px(18) : Theme.px(23)
                    font.weight: header ? Font.Bold : Font.DemiBold
                    font.letterSpacing: Theme.px(2)
                }
            }
        }
    }

    Column {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: Theme.px(260)
        spacing: Theme.px(8)
        BoardRow { header: true }
        Repeater {
            model: root.rows.slice(root.first, root.first + root.visibleRows)
            BoardRow {
                required property var modelData
                entry: modelData
                mine: modelData.name === online.account
            }
        }
        Text {
            visible: root.rows.length === 0
            topPadding: Theme.px(30)
            text: online.board.game === root.game.id ? "NOBODY HAS PLAYED RANKED IN THIS GAME YET" : "LOADING..."
            color: Theme.dim
            font.family: Theme.font
            font.pixelSize: Theme.px(24)
            font.weight: Font.Bold
            font.letterSpacing: Theme.px(2)
        }
        Item { width: 1; height: Theme.px(14); visible: root.me !== undefined }
        BoardRow {
            visible: root.me !== undefined && root.rows.slice(root.first, root.first + root.visibleRows)
                                                  .every(r => r.name !== online.account)
            entry: root.me || ({})
            mine: true
        }
    }
}
