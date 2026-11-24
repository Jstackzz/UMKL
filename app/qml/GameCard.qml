import QtQuick
import QtQuick.Effects
import MKArcade.Native

// A game in the home carousel: its attract mode live when it is the selected one, else the best moment of it seen
// so far, with its title under it.
Item {
    id: card
    property var game: ({})
    property bool current: false
    property bool live: false
    property int artRevision: 0
    readonly property string art: game.set ? library.artUrl(game.set) : ""
    readonly property bool showingLive: live && liveView.hasFrame && emu.set === game.set

    width: Theme.px(440)
    height: frame.height + Theme.px(64)
    scale: current ? 1.0 : 0.82
    opacity: current ? 1.0 : 0.6
    z: current ? 2 : 1
    Behavior on scale { NumberAnimation { duration: 240; easing.type: Easing.OutCubic } }
    Behavior on opacity { NumberAnimation { duration: 240 } }

    Rectangle {
        id: glow
        anchors.fill: frame
        anchors.margins: -Theme.px(6)
        radius: frame.radius + Theme.px(6)
        color: "transparent"
        border.color: Theme.gold
        border.width: Theme.px(2)
        opacity: card.current ? 0.55 : 0
        Behavior on opacity { NumberAnimation { duration: 240 } }
        layer.enabled: true
        layer.effect: MultiEffect {
            blurEnabled: true
            blur: 0.6
            blurMax: 24
        }
    }

    Rectangle {
        id: frame
        width: parent.width
        height: width * 3 / 4
        radius: Theme.px(10)
        color: "#0b0908"
        border.color: card.current ? Theme.gold : Theme.line
        border.width: card.current ? Theme.px(3) : Theme.px(1)
        clip: true

        Image {
            anchors.fill: parent
            anchors.margins: parent.border.width
            source: card.art ? card.art + "?r=" + card.artRevision : ""
            fillMode: Image.PreserveAspectCrop
            cache: false
            smooth: true
            visible: status === Image.Ready && !card.showingLive
        }
        GameView {
            id: liveView
            anchors.fill: parent
            anchors.margins: parent.border.width
            session: card.live ? emu : null
            filtered: true
            visible: card.showingLive
        }
        // no picture yet: the game's short title, large
        Text {
            anchors.centerIn: parent
            visible: !card.art && !card.showingLive
            text: card.game.tag || ""
            color: Theme.goldDeep
            font.family: Theme.display
            font.pixelSize: Theme.px(84)
            font.weight: Font.Black
            font.letterSpacing: Theme.px(4)
        }
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: parent.height * 0.4
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#00000000" }
                GradientStop { position: 1.0; color: "#c0000000" }
            }
        }
        Text {
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            anchors.margins: Theme.px(14)
            text: card.game.year || ""
            color: Theme.dim
            font.family: Theme.font
            font.pixelSize: Theme.px(18)
            font.weight: Font.DemiBold
        }
        Rectangle {
            visible: !card.game.ready
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: Theme.px(12)
            width: needLabel.implicitWidth + Theme.px(18)
            height: Theme.px(30)
            radius: Theme.px(4)
            color: "#b0100a"
            Text {
                id: needLabel
                anchors.centerIn: parent
                text: "ROM SET NEEDED"
                color: "white"
                font.family: Theme.font
                font.pixelSize: Theme.px(15)
                font.weight: Font.Bold
                font.letterSpacing: Theme.px(1)
            }
        }
    }

    Text {
        anchors.top: frame.bottom
        anchors.topMargin: Theme.px(14)
        anchors.horizontalCenter: frame.horizontalCenter
        text: (card.game.title || "").toUpperCase()
        color: card.current ? Theme.text : Theme.dim
        font.family: Theme.font
        font.pixelSize: Theme.px(24)
        font.weight: Font.Bold
        font.letterSpacing: Theme.px(2)
    }
}
