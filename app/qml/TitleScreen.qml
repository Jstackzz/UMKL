import QtQuick
import QtQuick.Controls
import QtQuick.Effects

// PRESS START: the title over the attract mode of one of the games, changing every so often.
Item {
    id: root
    property var app
    readonly property var prompts: [{action: "start", text: "Start"}]
    property int pick: 0

    function nav(a) {
        if (a === "accept" || a === "start") app.leaveTitle()
        else if (a === "back") app.confirmQuit()
    }
    function nextPreview() {
        const ready = library.games.filter(g => g.ready)
        if (!ready.length) return
        pick = (pick + 1) % ready.length
        app.preview(ready[pick].set)
    }
    StackView.onActivated: {
        const ready = library.games.filter(g => g.ready)
        if (ready.length) app.preview(ready[ready.length - 1].set)
        cycle.restart()
    }
    StackView.onDeactivated: cycle.stop()
    Timer {
        id: cycle
        interval: 28000
        repeat: true
        onTriggered: root.nextPreview()
    }

    Column {
        anchors.centerIn: parent
        anchors.verticalCenterOffset: -Theme.px(40)
        spacing: Theme.px(6)
        GoldText {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "MK"
            pixelSize: Theme.px(260)
            weight: Font.Black
            spacing: Theme.px(18)
        }
        GoldText {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "ARCADE  ONLINE"
            pixelSize: Theme.px(72)
            weight: Font.Bold
            spacing: Theme.px(16)
            topColor: "#ffffff"
            bottomColor: "#9a9187"
            glow: false
        }
        Item { width: 1; height: Theme.px(18) }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "MORTAL KOMBAT  ·  MORTAL KOMBAT II  ·  MORTAL KOMBAT 3  ·  ULTIMATE MORTAL KOMBAT 3"
            color: Theme.dim
            font.family: Theme.font
            font.pixelSize: Theme.px(20)
            font.weight: Font.DemiBold
            font.letterSpacing: Theme.px(3)
        }
    }

    Row {
        id: press
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Theme.px(170)
        spacing: Theme.px(16)
        ButtonGlyph {
            action: "start"
            size: Theme.px(44)
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            text: "PRESS START"
            color: Theme.text
            font.family: Theme.font
            font.pixelSize: Theme.px(40)
            font.weight: Font.Bold
            font.letterSpacing: Theme.px(8)
            anchors.verticalCenter: parent.verticalCenter
        }
        SequentialAnimation on opacity {
            loops: Animation.Infinite
            NumberAnimation { to: 0.25; duration: 900; easing.type: Easing.InOutSine }
            NumberAnimation { to: 1.0; duration: 900; easing.type: Easing.InOutSine }
        }
    }

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Theme.px(100)
        visible: library.readyCount === 0
        text: "No ROM sets found in " + library.romDir + ". Set your ROM folder in Settings."
        color: Theme.bad
        font.family: Theme.font
        font.pixelSize: Theme.px(22)
    }
}
