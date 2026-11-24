import QtQuick
import QtQuick.Effects
import MKArcade.Native

// The running attract mode across the whole screen, blurred and darkened behind the menus.
Item {
    id: root
    property bool active: true
    property real dim: 0.58
    clip: true
    opacity: active && view.hasFrame && emu.mode === "preview" ? 1 : 0
    Behavior on opacity { NumberAnimation { duration: 700; easing.type: Easing.InOutQuad } }

    GameView {
        id: view
        session: root.active ? emu : null
        filtered: true
        width: Math.max(root.width, root.height * 4 / 3)
        height: width * 3 / 4
        anchors.centerIn: parent
        layer.enabled: true
        layer.effect: MultiEffect {
            blurEnabled: true
            blur: 0.7
            blurMax: 40
            saturation: -0.2
        }
    }
    Rectangle {
        anchors.fill: parent
        color: "black"
        opacity: root.dim
    }
}
