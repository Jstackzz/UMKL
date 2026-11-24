import QtQuick

// One controller button as the player's controller labels it (Xbox, PlayStation, Switch), or a key on a keyboard.
Item {
    id: root
    property string action: "accept"   // accept back option tabLeft tabRight start pause dpad
    property real size: Theme.px(34)
    readonly property string kind: input.glyphs

    readonly property var spec: {
        const x = {
            accept: ["A", "#6abf3c"], back: ["B", "#d8433a"], option: ["Y", "#e0b62e"], other: ["X", "#3b82d8"],
            tabLeft: ["LB", "#8d8378"], tabRight: ["RB", "#8d8378"], start: ["☰", "#8d8378"],
            pause: ["⧉", "#8d8378"], dpad: ["✚", "#8d8378"]
        }
        const p = {
            accept: ["✕", "#7fa6e8"], back: ["○", "#e0625a"], option: ["△", "#4fc39a"], other: ["□", "#d77bc4"],
            tabLeft: ["L1", "#8d8378"], tabRight: ["R1", "#8d8378"], start: ["OPT", "#8d8378"],
            pause: ["SHR", "#8d8378"], dpad: ["✚", "#8d8378"]
        }
        const n = {
            accept: ["B", "#d8433a"], back: ["A", "#6abf3c"], option: ["X", "#3b82d8"], other: ["Y", "#e0b62e"],
            tabLeft: ["L", "#8d8378"], tabRight: ["R", "#8d8378"], start: ["+", "#8d8378"],
            pause: ["−", "#8d8378"], dpad: ["✚", "#8d8378"]
        }
        const k = {
            accept: ["ENTER", "#8d8378"], back: ["ESC", "#8d8378"], option: ["TAB", "#8d8378"],
            other: ["U", "#8d8378"], tabLeft: ["Q", "#8d8378"], tabRight: ["E", "#8d8378"],
            start: ["ENTER", "#8d8378"], pause: ["ESC", "#8d8378"], dpad: ["ARROWS", "#8d8378"]
        }
        const t = kind === "playstation" ? p : kind === "switch" ? n : kind === "keyboard" ? k : x
        return t[action] || ["?", "#8d8378"]
    }
    readonly property bool round: kind !== "keyboard" && spec[0].length <= 1

    implicitWidth: round ? size : Math.max(size, keyLabel.implicitWidth + Theme.px(18))
    implicitHeight: size

    Rectangle {
        anchors.fill: parent
        radius: root.round ? width / 2 : Theme.px(7)
        color: "#0c0a09"
        border.color: root.spec[1]
        border.width: Theme.px(2)
    }
    Text {
        id: keyLabel
        anchors.centerIn: parent
        text: root.spec[0]
        color: root.spec[1]
        font.family: Theme.font
        font.weight: Font.Bold
        font.pixelSize: root.round ? root.size * 0.52 : root.size * 0.42
    }
}
