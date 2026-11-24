import QtQuick

// One line of a settings page. kind: "choice" (left and right go through values), "slider" (0 to 100), "text"
// (A opens the on-screen keyboard), "action" (A does it), "info" (only shows).
Item {
    id: root
    property string label
    property string kind: "choice"
    property var values: []        // choice: display texts
    property int choice: 0         // choice: the shown one
    property int amount: 0         // slider
    property string value          // text, info
    property bool current: false
    signal changed(int delta)
    signal activated()

    width: parent ? parent.width : Theme.px(1100)
    height: Theme.px(70)

    Rectangle {
        anchors.fill: parent
        radius: Theme.px(6)
        color: root.current ? "#2a1714" : "transparent"
        border.color: root.current ? Theme.gold : "transparent"
        border.width: Theme.px(2)
    }
    Text {
        anchors.left: parent.left
        anchors.leftMargin: Theme.px(26)
        anchors.verticalCenter: parent.verticalCenter
        text: root.label.toUpperCase()
        color: root.current ? "white" : Theme.text
        font.family: Theme.font
        font.pixelSize: Theme.px(23)
        font.weight: Font.DemiBold
        font.letterSpacing: Theme.px(2)
    }
    Row {
        anchors.right: parent.right
        anchors.rightMargin: Theme.px(26)
        anchors.verticalCenter: parent.verticalCenter
        spacing: Theme.px(16)
        Text {
            visible: root.kind === "choice" || root.kind === "slider"
            text: "◀"
            color: root.current ? Theme.gold : "transparent"
            font.pixelSize: Theme.px(18)
            anchors.verticalCenter: parent.verticalCenter
        }
        Item {
            visible: root.kind === "slider"
            width: Theme.px(300)
            height: Theme.px(12)
            anchors.verticalCenter: parent.verticalCenter
            Rectangle { anchors.fill: parent; radius: height / 2; color: "#2b2420" }
            Rectangle {
                width: parent.width * root.amount / 100
                height: parent.height
                radius: height / 2
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0; color: Theme.redDeep }
                    GradientStop { position: 1; color: Theme.red }
                }
            }
        }
        Text {
            text: root.kind === "choice" ? (root.values[root.choice] || "")
                : root.kind === "slider" ? root.amount + "%"
                : root.kind === "action" ? "▶"
                : root.value
            color: root.kind === "info" ? Theme.dim : Theme.gold
            font.family: Theme.font
            font.pixelSize: Theme.px(23)
            font.weight: Font.Bold
            font.letterSpacing: Theme.px(1)
            elide: Text.ElideMiddle
            width: Math.min(implicitWidth, Theme.px(760))
            anchors.verticalCenter: parent.verticalCenter
        }
        Text {
            visible: root.kind === "choice" || root.kind === "slider"
            text: "▶"
            color: root.current ? Theme.gold : "transparent"
            font.pixelSize: Theme.px(18)
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    function nav(a) {
        if (kind === "choice" || kind === "slider") {
            if (a === "left") { changed(-1); return true }
            if (a === "right") { changed(1); return true }
        }
        if (a === "accept" && (kind === "text" || kind === "action" || kind === "choice")) {
            if (kind === "choice") changed(1)
            else activated()
            return true
        }
        return false
    }
}
