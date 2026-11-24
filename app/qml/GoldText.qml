import QtQuick
import QtQuick.Effects

// Title text in a metal gradient with a dark outline and a soft glow, the way fighting game menus letter them.
// The metal is a gradient masked by the text; the glow is a separate blurred copy behind it, so the glow's padding
// never moves the mask.
Item {
    id: root
    property alias text: label.text
    property real pixelSize: Theme.px(64)
    property int weight: Font.Bold
    property real spacing: Theme.px(4)
    property color topColor: Theme.goldLight
    property color bottomColor: Theme.goldDeep
    property bool glow: true
    property int horizontalAlignment: Text.AlignLeft
    implicitWidth: label.implicitWidth
    implicitHeight: label.implicitHeight

    // the glow: an ember-coloured copy of the text, blurred, behind everything
    Text {
        id: glowSource
        anchors.fill: parent
        visible: false
        layer.enabled: root.glow
        text: label.text
        font: label.font
        horizontalAlignment: root.horizontalAlignment
        color: "#d0661c"
    }
    MultiEffect {
        anchors.fill: parent
        visible: root.glow
        source: glowSource
        blurEnabled: true
        blur: 0.9
        blurMax: 40
        opacity: 0.55
    }

    // the outline under the metal
    Text {
        anchors.fill: parent
        horizontalAlignment: root.horizontalAlignment
        text: label.text
        font: label.font
        color: "#120a04"
        style: Text.Outline
        styleColor: "#120a04"
    }

    Text {
        id: label
        anchors.fill: parent
        visible: false
        layer.enabled: true
        horizontalAlignment: root.horizontalAlignment
        font.family: Theme.display
        font.pixelSize: root.pixelSize
        font.weight: root.weight
        font.letterSpacing: root.spacing
        color: "white"
    }
    Rectangle {
        id: fill
        anchors.fill: parent
        visible: false
        layer.enabled: true
        gradient: Gradient {
            GradientStop { position: 0.0; color: root.topColor }
            GradientStop { position: 0.48; color: Qt.lighter(root.bottomColor, 1.6) }
            GradientStop { position: 0.52; color: root.bottomColor }
            GradientStop { position: 1.0; color: Qt.lighter(root.bottomColor, 1.3) }
        }
    }
    MultiEffect {
        anchors.fill: parent
        source: fill
        autoPaddingEnabled: false
        maskEnabled: true
        maskSource: label
    }
}
