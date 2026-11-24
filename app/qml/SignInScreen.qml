import QtQuick
import QtQuick.Controls

// Sign in to the MK3D server (or make an account there): the same accounts as the MK3D launcher's. Once signed in,
// this screen gives way to the one it was opened for (next, with nextProps).
Item {
    id: root
    property var app
    property string next: ""
    property var nextProps: ({})
    property int row: 0
    property string name: online.lastName
    property string password: ""
    property bool remember: true
    readonly property int rows: 5
    readonly property var prompts: [{action: "accept", text: "Select"}, {action: "back", text: "Back"}]

    function nav(a) {
        if (online.waiting) { if (a === "back") app.pop(); return }
        if (a === "up") { row = (row + rows - 1) % rows; return }
        if (a === "down") { row = (row + 1) % rows; return }
        if (a === "back") { app.pop(); return }
        const r = rowItems.itemAt(row)
        if (r && r.nav(a)) return
        if (a === "start") go(false)
    }
    function go(create) {
        if (root.name.length < 3) { app.toast("Type your name first"); row = 0; return }
        if (root.password.length < 6) { app.toast("A password is at least 6 characters"); row = 1; return }
        if (create) online.createAccount(root.name, root.password, root.remember)
        else online.signIn(root.name, root.password, root.remember)
    }
    Connections {
        target: online
        function onSignedIn() {
            if (root.StackView.status !== StackView.Active) return
            root.password = ""
            if (root.next !== "") root.app.replace(root.next, root.nextProps)
            else root.app.pop()
        }
    }
    StackView.onActivated: {
        online.clearError()
        online.connectServer()
    }

    Column {
        anchors.left: parent.left
        anchors.leftMargin: Theme.px(96)
        anchors.top: parent.top
        anchors.topMargin: Theme.px(70)
        spacing: Theme.px(6)
        Text {
            text: "MK3D SERVER  ·  ONLINE"
            color: Theme.dim
            font.family: Theme.font
            font.pixelSize: Theme.px(22)
            font.weight: Font.Bold
            font.letterSpacing: Theme.px(4)
        }
        GoldText {
            text: "SIGN IN"
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
        anchors.verticalCenterOffset: Theme.px(40)
        width: Theme.px(1100)
        height: form.implicitHeight + Theme.px(60)
        radius: Theme.px(12)
        color: "#e0110d0c"
        border.color: Theme.line
        Column {
            id: form
            anchors.centerIn: parent
            width: parent.width - Theme.px(60)
            spacing: Theme.px(8)
            Repeater {
                id: rowItems
                model: 5
                SettingRow {
                    required property int index
                    current: root.row === index
                    label: ["Name", "Password", "Remember me", "Sign in", "Create account"][index]
                    kind: ["text", "text", "choice", "action", "action"][index]
                    values: ["No", "Yes"]
                    choice: root.remember ? 1 : 0
                    value: index === 0 ? (root.name.length ? root.name : "TYPE YOUR NAME")
                         : index === 1 ? (root.password.length ? "•".repeat(root.password.length) : "TYPE YOUR PASSWORD")
                         : ""
                    onChanged: (d) => { if (index === 2) root.remember = !root.remember }
                    onActivated: {
                        if (index === 0)
                            root.app.keyboard("Name (3 to 16: letters, digits, - and _)", root.name, false, 16,
                                              (t) => root.name = t.trim(), {keepCase: true})
                        else if (index === 1)
                            root.app.keyboard("Password", root.password, false, 64, (t) => root.password = t,
                                              {keepCase: true, secret: true})
                        else root.go(index === 4)
                    }
                }
            }
            Text {
                width: parent.width
                topPadding: Theme.px(14)
                wrapMode: Text.WordWrap
                text: online.waiting ? "SIGNING IN..."
                    : online.error !== "" ? online.error
                    : online.state === "offline" ? "The server cannot be reached right now."
                    : "Your MK3D account works here too. New? Type a name and a password, then CREATE ACCOUNT."
                color: online.error !== "" ? Theme.bad : Theme.dim
                font.family: Theme.font
                font.pixelSize: Theme.px(20)
            }
        }
    }
}
