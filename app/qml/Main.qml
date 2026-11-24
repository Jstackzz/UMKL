import QtQuick
import QtQuick.Controls
import QtQuick.Window
import MKArcade.Native

// The app's one window: the backdrop, a stack of screens driven by the controller (or keyboard), and the overlays
// (on-screen keyboard, toasts, quit). Screens implement nav(action) and list their button prompts.
Window {
    id: win
    width: 1600
    height: 900
    minimumWidth: 960
    minimumHeight: 540
    visible: true
    visibility: appSettings.fullscreen ? Window.FullScreen : Window.Windowed
    title: "MK Arcade Online"
    color: Theme.bg

    property int artRevision: 0
    property string wantPreview: ""
    property bool quitting: false
    readonly property bool inGame: stack.currentItem && stack.currentItem.objectName === "game"

    Binding { target: Theme; property: "s"; value: Math.min(win.width / 1920, win.height / 1080) }

    // --- navigation helpers for the screens
    function push(name, props) {
        const comp = {settings: settingsComp, online: onlineComp, direct: directComp, signin: signInComp,
                      match: matchComp, lobbies: lobbiesComp, lobby: lobbyComp, board: boardComp}[name]
        props.app = win
        stack.push(comp, props)
    }
    function pop() {
        if (stack.depth > 1) stack.pop()
    }
    // this screen gives way to another (sign-in to what it was for)
    function replace(name, props) {
        const comp = {match: matchComp, lobbies: lobbiesComp, lobby: lobbyComp, board: boardComp}[name]
        props.app = win
        stack.replace(comp, props)
    }
    function goHome() { stack.push(homeComp, {app: win}) }
    function toast(t) { toastItem.show(t) }
    // options: {keepCase: true} for names, {keepCase: true, secret: true} for passwords
    function keyboard(title, text, digits, max, done, options) { osk.open(title, text, digits, max, done, options) }
    function confirmQuit() { quitting = true; quitIndex = 1 }
    function preview(set) {
        wantPreview = set
        previewTimer.restart()
    }
    function startGame(set, mode) {
        previewTimer.stop()
        emu.play(set, mode)
        stack.push(gameComp, {app: win, set: set, mode: mode})
    }
    function startNetGame(set, side, port, peer, delay) {
        previewTimer.stop()
        emu.playNet(set, side, port, peer, delay)
        stack.push(gameComp, {app: win, set: set, mode: "net"})
    }
    function leaveGame() {
        emu.stop()
        input.gameMode = false
        stack.pop()
    }

    Timer {
        id: previewTimer
        interval: 380
        onTriggered: {
            if (win.inGame) return
            if (win.wantPreview === "") { if (emu.mode === "preview") emu.stop(); return }
            if (emu.mode === "preview" && emu.set === win.wantPreview) return
            emu.play(win.wantPreview, "preview")
        }
    }
    Connections {
        target: emu
        function onArtChanged(set) { win.artRevision++ }
        // an online match the server set up: its game comes up wherever the player is, and goes once it is over
        function onLinkStarted(set) {
            previewTimer.stop()
            osk.close()
            if (!win.inGame) stack.push(gameComp, {app: win, set: set, mode: "link"})
        }
        function onLinkParked() {
            if (win.inGame && stack.currentItem.mode === "link") {
                input.gameMode = false
                stack.pop()
            }
        }
    }
    Connections {
        target: online
        function onNotice(text) { win.toast(text) }
    }
    Connections {
        target: input
        function onNav(a) {
            if (osk.visible) { osk.nav(a); return }
            if (win.quitting) { win.quitNav(a); return }
            if (found.visible) { found.nav(a); return }
            if (stack.currentItem && stack.currentItem.nav) stack.currentItem.nav(a)
        }
        function onPauseRequested() {
            if (stack.currentItem && stack.currentItem.pauseRequested) stack.currentItem.pauseRequested()
        }
    }

    property int quitIndex: 1
    function quitNav(a) {
        if (a === "left" || a === "right") quitIndex = 1 - quitIndex
        else if (a === "back") quitting = false
        else if (a === "accept" || a === "start") {
            if (quitIndex === 0) Qt.quit()
            else quitting = false
        }
    }

    Item {
        id: root
        anchors.fill: parent
        focus: true
        Keys.onPressed: (e) => {
            if (e.key === Qt.Key_F11) { appSettings.fullscreen = !appSettings.fullscreen; e.accepted = true; return }
            if (osk.visible && !input.gameMode && e.text.length > 0 && !/^[\s]$/.test(e.text) &&
                    e.key !== Qt.Key_Escape && e.key !== Qt.Key_Return && e.key !== Qt.Key_Enter) {
                osk.typed(e.text, e.key)
                e.accepted = true
                return
            }
            if (input.keyEvent(e.key, true, e.isAutoRepeat)) { e.accepted = true; return }
        }
        Keys.onReleased: (e) => {
            if (!e.isAutoRepeat && input.keyEvent(e.key, false, false)) e.accepted = true
        }

        Background {
            anchors.fill: parent
            visible: !win.inGame
        }
        Backdrop {
            anchors.fill: parent
            active: !win.inGame
        }

        StackView {
            id: stack
            anchors.fill: parent
            focus: false
            initialItem: titleComp
            pushEnter: Transition {
                ParallelAnimation {
                    NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 260; easing.type: Easing.OutQuad }
                    NumberAnimation { property: "scale"; from: 1.04; to: 1; duration: 320; easing.type: Easing.OutCubic }
                }
            }
            pushExit: Transition {
                NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 200 }
            }
            popEnter: Transition {
                ParallelAnimation {
                    NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 260 }
                    NumberAnimation { property: "scale"; from: 0.97; to: 1; duration: 300; easing.type: Easing.OutCubic }
                }
            }
            popExit: Transition {
                NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 180 }
            }
        }

        PromptBar {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            visible: !win.inGame && !osk.visible
            items: stack.currentItem && stack.currentItem.prompts ? stack.currentItem.prompts : []
        }

        MatchFound { id: found }
        Toast { id: toastItem }
        OnScreenKeyboard { id: osk }

        // quit?
        Rectangle {
            anchors.fill: parent
            visible: win.quitting
            color: "#c8000000"
            Rectangle {
                anchors.centerIn: parent
                width: Theme.px(640)
                height: Theme.px(280)
                radius: Theme.px(12)
                color: Theme.panel
                border.color: Theme.gold
                border.width: Theme.px(2)
                Column {
                    anchors.centerIn: parent
                    spacing: Theme.px(34)
                    GoldText {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "QUIT TO WINDOWS?"
                        pixelSize: Theme.px(46)
                        weight: Font.Black
                        spacing: Theme.px(4)
                    }
                    Row {
                        anchors.horizontalCenter: parent.horizontalCenter
                        spacing: Theme.px(24)
                        ModeButton { label: "QUIT"; current: win.quitIndex === 0 }
                        ModeButton { label: "STAY"; current: win.quitIndex === 1 }
                    }
                }
            }
        }
    }

    Component { id: titleComp; TitleScreen { app: win } }
    Component { id: homeComp; HomeScreen {} }
    Component { id: onlineComp; OnlineScreen {} }
    Component { id: directComp; DirectScreen {} }
    Component { id: settingsComp; SettingsScreen {} }
    Component { id: signInComp; SignInScreen {} }
    Component { id: matchComp; MatchScreen {} }
    Component { id: lobbiesComp; LobbiesScreen {} }
    Component { id: lobbyComp; LobbyRoomScreen {} }
    Component { id: boardComp; LeaderboardScreen {} }
    Component { id: gameComp; GameScreen { objectName: "game" } }
}
