pragma Singleton
import QtQuick

// Colours, fonts and the UI's scale (s: 1 at 1920x1080), shared by every screen.
QtObject {
    property real s: 1.0

    readonly property color bg: "#070605"
    readonly property color panel: "#15110f"
    readonly property color panelHi: "#221a16"
    readonly property color line: "#3d2f27"
    readonly property color text: "#f1e8da"
    readonly property color dim: "#a59a8b"
    readonly property color faint: "#6e655a"
    readonly property color red: "#d0181f"
    readonly property color redDeep: "#5c0a0c"
    readonly property color gold: "#eebd55"
    readonly property color goldLight: "#ffe9a8"
    readonly property color goldDeep: "#8b5e1a"
    readonly property color good: "#7ccf6a"
    readonly property color bad: "#ff5a4f"

    readonly property string font: "Bahnschrift"
    readonly property string display: "Bahnschrift"

    function px(v) { return Math.round(v * s) }
}
