import QtQuick

// Typing with a controller: a grid of keys the d-pad moves over. A types, Y deletes, START (or OK) finishes, B leaves
// without changing anything. A keyboard types into it directly. Options: keepCase (letters as typed, with a SHIFT key:
// names and passwords) and secret (shown as dots: passwords).
Item {
    id: root
    anchors.fill: parent
    visible: false
    property string title
    property string text
    property int maxLength: 24
    property bool digitsOnly: false
    property bool keepCase: false
    property bool secret: false
    property bool shifted: false
    property var done: null   // function(text)
    property int row: 0
    property int col: 0

    readonly property var rows: digitsOnly
        ? [["1", "2", "3", "4", "5", "6", "7", "8", "9", "0"], [".", ":", "DEL", "OK"]]
        : [["1", "2", "3", "4", "5", "6", "7", "8", "9", "0"],
           ["Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P"],
           ["A", "S", "D", "F", "G", "H", "J", "K", "L", "-"],
           ["Z", "X", "C", "V", "B", "N", "M", ".", ":", "_"],
           keepCase ? ["SHIFT", "SPACE", "DEL", "OK"] : ["SPACE", "DEL", "OK"]]

    function keyText(k) { return keepCase && k.length === 1 && !shifted ? k.toLowerCase() : k }
    function open(title, text, digitsOnly, maxLength, done, options) {
        const o = options || {}
        root.title = title
        root.text = text
        root.digitsOnly = digitsOnly
        root.maxLength = maxLength
        root.done = done
        root.keepCase = !!o.keepCase
        root.secret = !!o.secret
        root.shifted = false
        row = 0
        col = 0
        visible = true
    }
    function close() { visible = false }
    function press(k) {
        if (k === "DEL") text = text.slice(0, -1)
        else if (k === "OK") { const f = done; close(); if (f) f(text) }
        else if (k === "SHIFT") shifted = !shifted
        else if (k === "SPACE") { if (text.length < maxLength) text += " " }
        else if (text.length < maxLength) text += keyText(k)
    }
    function nav(a) {
        const r = rows[row]
        if (a === "left") col = (col + r.length - 1) % r.length
        else if (a === "right") col = (col + 1) % r.length
        else if (a === "up") { row = (row + rows.length - 1) % rows.length; col = Math.min(col, rows[row].length - 1) }
        else if (a === "down") { row = (row + 1) % rows.length; col = Math.min(col, rows[row].length - 1) }
        else if (a === "accept") press(r[col])
        else if (a === "option") press("DEL")
        else if (a === "start") press("OK")
        else if (a === "back") close()
        return true
    }
    function typed(t, key) {
        if (key === Qt.Key_Backspace) { press("DEL"); return }
        if (key === Qt.Key_Return || key === Qt.Key_Enter) { press("OK"); return }
        if (digitsOnly && !/^[0-9.:]$/.test(t)) return
        if (/^[ -~]$/.test(t) && text.length < maxLength) text += digitsOnly || keepCase ? t : t.toUpperCase()
    }

    Rectangle { anchors.fill: parent; color: "#d0000000" }
    Rectangle {
        anchors.centerIn: parent
        width: Theme.px(1080)
        height: column.implicitHeight + Theme.px(70)
        radius: Theme.px(12)
        color: Theme.panel
        border.color: Theme.gold
        border.width: Theme.px(2)
        Column {
            id: column
            anchors.centerIn: parent
            spacing: Theme.px(18)
            Text {
                text: root.title.toUpperCase()
                color: Theme.dim
                font.family: Theme.font
                font.pixelSize: Theme.px(22)
                font.weight: Font.Bold
                font.letterSpacing: Theme.px(2)
            }
            Rectangle {
                width: Theme.px(980)
                height: Theme.px(70)
                radius: Theme.px(6)
                color: "#0b0908"
                border.color: Theme.line
                Text {
                    anchors.left: parent.left
                    anchors.leftMargin: Theme.px(20)
                    anchors.verticalCenter: parent.verticalCenter
                    text: (root.secret ? "•".repeat(root.text.length) : root.text) + "▏"
                    color: Theme.text
                    font.family: Theme.font
                    font.pixelSize: Theme.px(34)
                    font.weight: Font.DemiBold
                    font.letterSpacing: Theme.px(2)
                }
            }
            Repeater {
                model: root.rows
                Row {
                    id: keyRow
                    required property var modelData
                    required property int index
                    spacing: Theme.px(10)
                    anchors.horizontalCenter: parent ? parent.horizontalCenter : undefined
                    Repeater {
                        model: keyRow.modelData
                        Rectangle {
                            required property string modelData
                            required property int index
                            readonly property bool sel: root.row === keyRow.index && root.col === index
                            width: modelData.length > 1 ? Theme.px(190) : Theme.px(86)
                            height: Theme.px(70)
                            radius: Theme.px(6)
                            color: sel ? Theme.red : (modelData === "SHIFT" && root.shifted ? "#3a2a12" : "#1c1614")
                            border.color: sel ? Theme.gold : Theme.line
                            border.width: Theme.px(2)
                            Text {
                                anchors.centerIn: parent
                                text: root.keyText(parent.modelData)
                                color: "white"
                                font.family: Theme.font
                                font.pixelSize: Theme.px(26)
                                font.weight: Font.Bold
                            }
                        }
                    }
                }
            }
            Row {
                spacing: Theme.px(28)
                anchors.horizontalCenter: parent.horizontalCenter
                Repeater {
                    model: [{a: "accept", t: "Type"}, {a: "option", t: "Delete"}, {a: "start", t: "Done"},
                            {a: "back", t: "Cancel"}]
                    Row {
                        required property var modelData
                        spacing: Theme.px(8)
                        ButtonGlyph { action: parent.modelData.a; size: Theme.px(30) }
                        Text {
                            text: parent.modelData.t.toUpperCase()
                            color: Theme.dim
                            font.family: Theme.font
                            font.pixelSize: Theme.px(18)
                            font.weight: Font.DemiBold
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                }
            }
        }
    }
}
