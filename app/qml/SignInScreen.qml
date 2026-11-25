import QtQuick
import QtQuick.Controls

// The account on the MK3D server (the same accounts as the MK3D launcher's), the way that launcher's account window
// has it: CREATE ACCOUNT (a name, the password twice) or SIGN IN, a row switching between them. Once signed in, this
// screen gives way to the one it was opened for (next, with nextProps). welcome: opened by itself at the first PRESS
// START on a PC that never signed in (Main.qml, leaveTitle), on CREATE ACCOUNT; B is "later" there, on to next all the
// same.
Item {
    id: root
    property var app
    property string next: ""
    property var nextProps: ({})
    property bool welcome: false
    property bool creating: false  // set as it opens: a PC that never signed in makes an account, anyone else signs in
    property int row: 0
    property string name: online.lastName
    property string password: ""
    property string repeat: ""
    property bool remember: true
    readonly property var rowIds: creating ? ["name", "password", "repeat", "remember", "submit", "toggle"]
                                           : ["name", "password", "remember", "submit", "toggle"]
    readonly property var prompts: [{action: "accept", text: "Select"},
                                    {action: "back", text: welcome ? "Later" : "Back"}]

    Component.onCompleted: creating = welcome || online.newHere

    function rowOf(id) { return rowIds.indexOf(id) }
    // B: back to where it was opened from; at a first start, on to the home screen
    function leave() {
        if (!root.welcome) { root.app.pop(); return }
        root.app.toast("Sign in or make an account any time from a game's ONLINE")
        root.app.replace(root.next, root.nextProps)
    }
    function nav(a) {
        if (online.waiting) { if (a === "back") leave(); return }
        if (a === "up") { row = (row + rowIds.length - 1) % rowIds.length; return }
        if (a === "down") { row = (row + 1) % rowIds.length; return }
        if (a === "back") { leave(); return }
        const r = rowItems.itemAt(row)
        if (r && r.nav(a)) return
        if (a === "start") go()
    }
    // CREATE ACCOUNT or SIGN IN, keeping the name and password typed; the cursor goes to what is still missing
    function setMode(create) {
        root.creating = create
        root.repeat = ""
        online.clearError()
        root.row = root.name.length < 3 ? rowOf("name") : root.password.length < 6 ? rowOf("password")
                 : create ? rowOf("repeat") : rowOf("submit")
    }
    // the server's rules for names and passwords, checked here first
    function go() {
        if (!/^[A-Za-z0-9_-]{3,16}$/.test(root.name)) {
            root.app.toast("A name is 3 to 16 letters, digits, - or _")
            root.row = rowOf("name")
            return
        }
        if (root.password.length < 6) {
            root.app.toast("A password is at least 6 characters")
            root.row = rowOf("password")
            return
        }
        if (root.creating && root.repeat !== root.password) {
            root.app.toast("The two passwords differ: type it again")
            root.repeat = ""
            root.row = rowOf("repeat")
            return
        }
        if (root.creating) online.createAccount(root.name, root.password, root.remember)
        else online.signIn(root.name, root.password, root.remember)
    }
    // the on-screen keyboard for a field; once something is typed, the cursor moves on to the next thing to do
    function edit(id) {
        if (id === "name")
            root.app.keyboard("Name (3 to 16: letters, digits, - and _)", root.name, false, 16, (t) => {
                root.name = t.trim()
                if (root.name.length) root.row = rowOf("password")
            }, {keepCase: true})
        else if (id === "password")
            root.app.keyboard("Password (at least 6 characters)", root.password, false, 64, (t) => {
                root.password = t
                if (t.length) root.row = root.creating ? rowOf("repeat") : rowOf("submit")
            }, {keepCase: true, secret: true})
        else if (id === "repeat")
            root.app.keyboard("The same password again", root.repeat, false, 64, (t) => {
                root.repeat = t
                if (t.length) root.row = rowOf("submit")
            }, {keepCase: true, secret: true})
    }
    Connections {
        target: online
        function onSignedIn() {
            if (root.StackView.status !== StackView.Active) return
            root.password = ""
            root.repeat = ""
            if (root.welcome) root.app.toast("Signed in as " + online.account)
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
            text: root.welcome ? "WELCOME TO MK ARCADE ONLINE" : "MK3D SERVER  ·  ONLINE"
            color: Theme.dim
            font.family: Theme.font
            font.pixelSize: Theme.px(22)
            font.weight: Font.Bold
            font.letterSpacing: Theme.px(4)
        }
        GoldText {
            text: root.creating ? "CREATE ACCOUNT" : "SIGN IN"
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
                model: root.rowIds
                SettingRow {
                    required property string modelData
                    required property int index
                    current: root.row === index
                    label: ({name: "Name", password: "Password", repeat: "Repeat password", remember: "Remember me",
                             submit: root.creating ? "Create account" : "Sign in",
                             toggle: root.creating ? "I already have an account" : "Create a new account"})[modelData]
                    kind: modelData === "remember" ? "choice"
                        : modelData === "submit" || modelData === "toggle" ? "action" : "text"
                    values: ["No", "Yes"]
                    choice: root.remember ? 1 : 0
                    value: modelData === "name" ? (root.name.length ? root.name : "TYPE YOUR NAME")
                         : modelData === "password" ? (root.password.length ? "•".repeat(root.password.length)
                                                                            : "TYPE YOUR PASSWORD")
                         : modelData === "repeat" ? (root.repeat.length ? "•".repeat(root.repeat.length)
                                                                        : "TYPE IT AGAIN")
                         : ""
                    onChanged: (d) => { if (modelData === "remember") root.remember = !root.remember }
                    onActivated: {
                        if (modelData === "submit") root.go()
                        else if (modelData === "toggle") root.setMode(!root.creating)
                        else root.edit(modelData)
                    }
                }
            }
            Text {
                width: parent.width
                topPadding: Theme.px(14)
                wrapMode: Text.WordWrap
                text: online.waiting ? (root.creating ? "MAKING YOUR ACCOUNT..." : "SIGNING IN...")
                    : online.error !== "" ? online.error
                    : online.state === "offline"
                      ? "The server cannot be reached right now." + (root.welcome ? " Choose Later to play offline." : "")
                    : root.creating
                      ? (root.welcome
                         ? "Welcome! To play online you need an account: choose a name and a password. Have one "
                           + "already, here or in MK3D Online? Choose I ALREADY HAVE AN ACCOUNT. Not now? Choose "
                           + "Later: you can do this from any game's ONLINE."
                         : "Choose a name and a password. An MK3D Online account works here too: choose I ALREADY "
                           + "HAVE AN ACCOUNT.")
                    : "Your MK3D Online account works here too." + (root.welcome ? " Not now? Choose Later." : "")
                color: online.error !== "" ? Theme.bad : Theme.dim
                font.family: Theme.font
                font.pixelSize: Theme.px(20)
            }
        }
    }
}
