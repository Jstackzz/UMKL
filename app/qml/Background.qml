import QtQuick
import QtQuick.Effects
import QtQuick.Particles
import QtQuick.Shapes

// The menus' backdrop: dark stone with a slow red haze, rising embers and a vignette. Everything is drawn, nothing
// is a picture from the games.
Item {
    id: bg

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#140e0b" }
            GradientStop { position: 0.6; color: "#080605" }
            GradientStop { position: 1.0; color: "#020101" }
        }
    }

    Item {
        id: haze
        anchors.fill: parent
        layer.enabled: true
        layer.effect: MultiEffect {
            blurEnabled: true
            blur: 1.0
            blurMax: 64
        }
        Repeater {
            model: 5
            Rectangle {
                required property int index
                width: bg.width * (0.30 + 0.06 * index)
                height: width
                radius: width / 2
                color: index % 2 ? "#4a0b0b" : "#3a1a08"
                opacity: 0.42
                x: bg.width * (index * 0.21 - 0.1)
                y: bg.height * (0.15 + 0.18 * (index % 3))
                SequentialAnimation on x {
                    loops: Animation.Infinite
                    NumberAnimation {
                        to: bg.width * (index * 0.21 - 0.02)
                        duration: 17000 + index * 3100
                        easing.type: Easing.InOutSine
                    }
                    NumberAnimation {
                        to: bg.width * (index * 0.21 - 0.12)
                        duration: 19000 + index * 2700
                        easing.type: Easing.InOutSine
                    }
                }
                SequentialAnimation on opacity {
                    loops: Animation.Infinite
                    NumberAnimation { to: 0.22; duration: 9000 + index * 1300; easing.type: Easing.InOutSine }
                    NumberAnimation { to: 0.45; duration: 8000 + index * 1700; easing.type: Easing.InOutSine }
                }
            }
        }
    }

    ParticleSystem {
        id: embers
        anchors.fill: parent
    }
    Emitter {
        system: embers
        anchors.bottom: parent.bottom
        width: parent.width
        height: Theme.px(40)
        emitRate: 9
        lifeSpan: 11000
        lifeSpanVariation: 4000
        velocity: AngleDirection {
            angle: 270
            angleVariation: 14
            magnitude: Theme.px(60)
            magnitudeVariation: Theme.px(40)
        }
        acceleration: AngleDirection {
            angle: 0
            angleVariation: 360
            magnitude: Theme.px(5)
        }
    }
    ItemParticle {
        system: embers
        fade: true
        delegate: Rectangle {
            width: Theme.px(2 + Math.random() * 3)
            height: width
            radius: width / 2
            color: Math.random() > 0.5 ? "#ff7c2e" : "#ffc060"
            opacity: 0.35 + Math.random() * 0.5
        }
    }

    Shape {
        anchors.fill: parent
        preferredRendererType: Shape.CurveRenderer
        ShapePath {
            strokeWidth: 0
            strokeColor: "transparent"
            fillGradient: RadialGradient {
                centerX: bg.width / 2
                centerY: bg.height * 0.45
                centerRadius: Math.max(bg.width, bg.height) * 0.72
                focalX: centerX
                focalY: centerY
                GradientStop { position: 0.45; color: "#00000000" }
                GradientStop { position: 1.0; color: "#e6000000" }
            }
            startX: 0; startY: 0
            PathLine { x: bg.width; y: 0 }
            PathLine { x: bg.width; y: bg.height }
            PathLine { x: 0; y: bg.height }
            PathLine { x: 0; y: 0 }
        }
    }
}
