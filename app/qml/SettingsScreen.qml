import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs

// Settings in tabs (LB / RB): display, sound, controls, online and the ROM library.
Item {
    id: root
    property var app
    property int tab: 0
    property int row: 0
    readonly property var tabs: ["DISPLAY", "SOUND", "CONTROLS", "ONLINE", "LIBRARY"]
    readonly property var prompts: [{action: "tabLeft", text: "Tab"}, {action: "tabRight", text: "Tab"},
                                    {action: "dpad", text: "Change"}, {action: "back", text: "Back"}]
    readonly property var scaleModes: ["fit", "stretch", "pixel"]

    readonly property var rowsByTab: [
        [
            {label: "Window", kind: "choice", values: ["Full screen", "Window"], get: () => appSettings.fullscreen ? 0 : 1,
             set: (i) => appSettings.fullscreen = (i === 0)},
            {label: "Picture", kind: "choice", values: ["4:3, as the arcade monitor", "Stretch to the screen", "Whole pixels"],
             get: () => Math.max(0, root.scaleModes.indexOf(appSettings.scaleMode)), set: (i) => appSettings.scaleMode = root.scaleModes[i]},
            {label: "Filter", kind: "choice", values: ["Sharp", "Smooth"], get: () => appSettings.smooth ? 1 : 0,
             set: (i) => appSettings.smooth = (i === 1)}
        ],
        [
            {label: "Game volume", kind: "slider", get: () => appSettings.volume, set: (v) => appSettings.volume = v},
            {label: "Menu attract volume", kind: "slider", get: () => appSettings.menuVolume, set: (v) => appSettings.menuVolume = v}
        ],
        [
            {label: "Controllers", kind: "info", value: () => input.padCount ? input.padNames.join(", ") : "None, the keyboard plays"},
            {label: "Punches", kind: "info", value: () => "High: X / □     Low: A / ✕     Keyboard: U, J"},
            {label: "Kicks", kind: "info", value: () => "High: Y / △     Low: B / ○     Keyboard: O, L"},
            {label: "Block · Run", kind: "info", value: () => "Block: LB or RB     Run: LT or RT     Keyboard: I, K"},
            {label: "Start · Pause", kind: "info", value: () => "START inserts a coin and joins     BACK / VIEW or Esc pauses"}
        ],
        [
            {label: "Account", kind: "info", value: () => online.state === "signedIn" ? online.account
                                                        : "Not signed in (ONLINE on a game's menu)"},
            {label: "Region", kind: "choice",
             values: ["North America", "South America", "Europe", "Middle East", "Africa", "Asia", "Oceania"],
             get: () => Math.max(0, ["na", "sa", "eu", "me", "af", "as", "oc"].indexOf(appSettings.region)),
             set: (i) => appSettings.region = ["na", "sa", "eu", "me", "af", "as", "oc"][i]},
            {label: "Highest ping to search with", kind: "choice", values: ["Any", "80 ms", "120 ms", "160 ms", "200 ms"],
             get: () => Math.max(0, [0, 80, 120, 160, 200].indexOf(appSettings.maxPing)),
             set: (i) => appSettings.maxPing = [0, 80, 120, 160, 200][i]},
            {label: "Search again after a series", kind: "choice", values: ["No", "Yes"],
             get: () => appSettings.searchAgain ? 1 : 0, set: (i) => appSettings.searchAgain = i === 1},
            {label: "Hide my IP address from opponents", kind: "choice", values: ["No", "Yes (through the server)"],
             get: () => appSettings.hideIp ? 1 : 0, set: (i) => appSettings.hideIp = i === 1},
            {label: "Input delay", kind: "choice", values: ["0 frames", "1 frame", "2 frames", "3 frames", "4 frames", "5 frames", "6 frames"],
             get: () => appSettings.inputDelay, set: (i) => appSettings.inputDelay = i},
            {label: "Game port (UDP)", kind: "text", value: () => String(appSettings.netPort),
             edit: () => root.app.keyboard("UDP port", String(appSettings.netPort), true, 5,
                                           (t) => { const p = parseInt(t); if (p > 1023 && p < 65536) appSettings.netPort = p })},
            {label: "Server", kind: "info", value: () => appSettings.server + "   ·   " +
                                                       (online.state === "offline" ? "offline" : online.onlineCount + " online")}
        ],
        [
            {label: "ROM folder", kind: "text", value: () => library.romDir, edit: () => folderDialog.open()},
            {label: "Check ROM sets again", kind: "action", act: () => { library.refresh(); root.app.toast(library.readyCount + " of 4 games ready") }},
            {label: "Mortal Kombat", kind: "info", value: () => library.games.length > 0 ? library.games[0].status : ""},
            {label: "Mortal Kombat II", kind: "info", value: () => library.games.length > 1 ? library.games[1].status : ""},
            {label: "Mortal Kombat 3", kind: "info", value: () => library.games.length > 2 ? library.games[2].status : ""},
            {label: "Ultimate MK3", kind: "info", value: () => library.games.length > 3 ? library.games[3].status : ""}
        ]
    ]
    readonly property var rows: rowsByTab[tab]

    function nav(a) {
        if (a === "tabLeft") { tab = (tab + tabs.length - 1) % tabs.length; row = 0; return }
        if (a === "tabRight") { tab = (tab + 1) % tabs.length; row = 0; return }
        if (a === "back") { app.pop(); return }
        if (a === "up") { row = (row + rows.length - 1) % rows.length; return }
        if (a === "down") { row = (row + 1) % rows.length; return }
        const it = list.itemAt(row)
        if (it) it.nav(a)
    }

    FolderDialog {
        id: folderDialog
        title: "Your ROM folder (mk.zip, mk2.zip, mk3.zip, umk3.zip)"
        onAccepted: {
            let p = decodeURIComponent(selectedFolder.toString())
            p = p.replace(/^file:\/{3}/, "")
            appSettings.romDir = p
        }
    }

    Column {
        anchors.left: parent.left
        anchors.leftMargin: Theme.px(96)
        anchors.top: parent.top
        anchors.topMargin: Theme.px(70)
        spacing: Theme.px(22)
        GoldText {
            text: "SETTINGS"
            pixelSize: Theme.px(84)
            weight: Font.Black
            spacing: Theme.px(8)
        }
        Row {
            spacing: Theme.px(14)
            ButtonGlyph { action: "tabLeft"; size: Theme.px(34); anchors.verticalCenter: parent.verticalCenter }
            Repeater {
                model: root.tabs
                Rectangle {
                    required property string modelData
                    required property int index
                    width: tabText.implicitWidth + Theme.px(44)
                    height: Theme.px(52)
                    radius: Theme.px(6)
                    color: index === root.tab ? Theme.red : "#1a1412"
                    border.color: index === root.tab ? Theme.gold : Theme.line
                    Text {
                        id: tabText
                        anchors.centerIn: parent
                        text: parent.modelData
                        color: "white"
                        font.family: Theme.font
                        font.pixelSize: Theme.px(21)
                        font.weight: Font.Bold
                        font.letterSpacing: Theme.px(2)
                    }
                }
            }
            ButtonGlyph { action: "tabRight"; size: Theme.px(34); anchors.verticalCenter: parent.verticalCenter }
        }
    }

    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        y: Theme.px(330)
        width: Theme.px(1500)
        height: rowsColumn.implicitHeight + Theme.px(50)
        radius: Theme.px(12)
        color: "#e0110d0c"
        border.color: Theme.line
        Column {
            id: rowsColumn
            anchors.centerIn: parent
            width: parent.width - Theme.px(50)
            spacing: Theme.px(6)
            Repeater {
                id: list
                model: root.rows
                SettingRow {
                    required property var modelData
                    required property int index
                    current: index === root.row
                    label: modelData.label
                    kind: modelData.kind
                    values: modelData.values || []
                    choice: modelData.get && modelData.kind === "choice" ? modelData.get() : 0
                    amount: modelData.get && modelData.kind === "slider" ? modelData.get() : 0
                    value: modelData.value ? modelData.value() : ""
                    onChanged: (d) => {
                        if (modelData.kind === "choice")
                            modelData.set((modelData.get() + d + modelData.values.length) % modelData.values.length)
                        else if (modelData.kind === "slider")
                            modelData.set(Math.max(0, Math.min(100, modelData.get() + d * 5)))
                    }
                    onActivated: {
                        if (modelData.edit) modelData.edit()
                        else if (modelData.act) modelData.act()
                    }
                }
            }
        }
    }
}
