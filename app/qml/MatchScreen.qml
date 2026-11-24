import QtQuick
import QtQuick.Controls

// Ranked or Kasual for one game: the search runs as soon as this opens; MATCH FOUND comes up over it; the match plays
// in the game screen; back here, the series: Kasual rematches as long as both want, a ranked set to 3 wins (its next
// match starts by itself), then the set's outcome and the Elo it moved.
Item {
    id: root
    property var app
    property var game: ({})
    property bool ranked: false
    property int index: 0
    readonly property string phase: online.phase
    readonly property var series: online.series
    readonly property var result: online.result
    readonly property bool seriesOn: series.state === "between" || series.state === "playing"
    readonly property bool seriesOver: series.state === "over"
    readonly property bool searching: phase === "searching"
    readonly property bool busy: ["found", "connecting", "starting", "playing", "reporting", "finishing"].indexOf(phase) >= 0
    readonly property var buttons: seriesOn ? (series.ranked ? ["LEAVE SET"] : ["REMATCH", "LEAVE"])
                                 : searching || busy ? ["CANCEL"]
                                 : ["SEARCH AGAIN", "BACK"]
    readonly property var prompts: [{action: "accept", text: "Select"}, {action: "back", text: searching ? "Cancel" : "Back"}]

    function start() {
        if (online.phase === "idle" && !seriesOn) online.search(root.game.id, root.ranked)
    }
    Component.onCompleted: start()

    function choose(label) {
        if (label === "REMATCH") online.rematch()
        else if (label === "LEAVE" || label === "LEAVE SET") online.leaveSeries()
        else if (label === "CANCEL") online.cancelSearch()
        else if (label === "SEARCH AGAIN") { online.clearResult(); online.search(root.game.id, root.ranked) }
        else if (label === "BACK") leave()
    }
    function leave() {
        if (searching || busy) online.cancelSearch()
        if (seriesOn) online.leaveSeries()
        online.clearResult()
        app.pop()
    }
    function nav(a) {
        index = Math.min(index, buttons.length - 1)
        if (a === "left") index = (index + buttons.length - 1) % buttons.length
        else if (a === "right") index = (index + 1) % buttons.length
        else if (a === "back") leave()
        else if (a === "accept" || a === "start") choose(buttons[index])
    }
    onButtonsChanged: index = 0

    function outcomeText() {
        const o = series.outcome
        const me = online.account
        if (o === "won") return series.winner === me ? "YOU WON THE SET" : "YOU LOST THE SET"
        if (o === "forfeit") return series.left === me ? "YOU LEFT THE SET: A LOSS" : "THEY LEFT: THE SET IS YOURS"
        if (o === "void") return "THE SET DID NOT COUNT"
        if (o === "timeout") return "NO REMATCH IN TIME"
        return "THE SERIES IS OVER"
    }
    function eloText() {
        const e = (series.elo || {})[online.account]
        if (!e) return ""
        const d = e.delta > 0 ? "+" + e.delta : String(e.delta)
        return "ELO " + e.before + " → " + e.after + "  (" + d + ")" +
               (e.title_after && e.title_after !== e.title_before ? "   ·   NOW " + e.title_after.toUpperCase() : "")
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
            text: root.ranked ? "RANKED" : "KASUAL"
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
        anchors.centerIn: parent
        anchors.verticalCenterOffset: Theme.px(50)
        width: Theme.px(1200)
        height: Theme.px(560)
        radius: Theme.px(14)
        color: "#e0110d0c"
        border.color: Theme.line
        border.width: Theme.px(2)

        Column {
            anchors.centerIn: parent
            width: parent.width - Theme.px(100)
            spacing: Theme.px(20)

            // what is going on, in big letters
            GoldText {
                anchors.horizontalCenter: parent.horizontalCenter
                text: root.searching ? "SEARCHING"
                    : root.phase === "connecting" || root.phase === "starting" ? "CONNECTING"
                    : root.phase === "reporting" || root.phase === "finishing" ? "MATCH OVER"
                    : root.seriesOver ? root.outcomeText()
                    : root.seriesOn ? (root.result.you_won === true ? "YOU WIN" : root.result.you_won === false ? "YOU LOSE" : "NEXT MATCH")
                    : root.result.you_won === true ? "YOU WIN" : root.result.you_won === false ? "YOU LOSE"
                    : "READY"
                pixelSize: Theme.px(64)
                weight: Font.Black
                spacing: Theme.px(5)
            }
            // the line under it
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                text: {
                    if (root.searching) {
                        const s = online.searchSeconds
                        const c = online.gameCounts[root.game.id] || {}
                        return Math.floor(s / 60) + ":" + String(s % 60).padStart(2, "0") + "   ·   " +
                               (c.searching || 0) + " SEARCHING THIS GAME"
                    }
                    if (root.busy) return online.phaseText.toUpperCase()
                    if (root.seriesOver) return root.eloText()
                    if (root.seriesOn) {
                        const score = "YOU " + (root.series.you || 0) + " – " + (root.series.them || 0) + " " +
                                      (root.series.opponent || "").toUpperCase()
                        return root.series.ranked ? score + "   ·   FIRST TO " + (root.series.need || 3) : score
                    }
                    return online.phaseText.toUpperCase()
                }
                color: Theme.text
                font.family: Theme.font
                font.pixelSize: Theme.px(26)
                font.weight: Font.Bold
                font.letterSpacing: Theme.px(2)
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                visible: text !== ""
                text: {
                    if (root.seriesOn && root.series.state === "between") {
                        const t = online.seriesSeconds
                        if (root.series.ranked) return "THE NEXT MATCH STARTS ONCE YOU BOTH ARE BACK  ·  " + t + " S"
                        if (root.series.you_ready) return "WAITING FOR " + (root.series.opponent || "THEM").toUpperCase() + "  ·  " + t + " S"
                        if (root.series.them_ready) return (root.series.opponent || "THEY").toUpperCase() + " WANTS A REMATCH  ·  " + t + " S"
                        return "REMATCH?  ·  " + t + " S"
                    }
                    if (root.searching && root.ranked) return "RANKED: YOU MEET PLAYERS NEAR YOUR ELO FIRST, THEN ANYONE"
                    if (root.seriesOver || (!root.busy && !root.searching)) return "SEARCH AGAIN, OR GO BACK TO THE ONLINE MENU"
                    return ""
                }
                color: Theme.dim
                font.family: Theme.font
                font.pixelSize: Theme.px(20)
                font.weight: Font.DemiBold
                font.letterSpacing: Theme.px(1)
            }
            // a pulse while searching
            Item {
                width: parent.width
                height: Theme.px(70)
                Row {
                    anchors.centerIn: parent
                    spacing: Theme.px(18)
                    visible: root.searching || root.busy
                    Repeater {
                        model: 5
                        Rectangle {
                            required property int index
                            width: Theme.px(18)
                            height: width
                            radius: width / 2
                            color: Theme.red
                            SequentialAnimation on opacity {
                                loops: Animation.Infinite
                                running: root.searching || root.busy
                                PauseAnimation { duration: index * 140 }
                                NumberAnimation { from: 0.2; to: 1; duration: 350 }
                                NumberAnimation { from: 1; to: 0.2; duration: 350 }
                                PauseAnimation { duration: (4 - index) * 140 }
                            }
                        }
                    }
                }
            }
            Row {
                anchors.horizontalCenter: parent.horizontalCenter
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
    }
}
