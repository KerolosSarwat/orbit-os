import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

ApplicationWindow {
    id: root
    width: 1440
    height: 920
    minimumWidth: 1120
    minimumHeight: 740
    visible: true
    title: "ORBIT — Computing, without friction"
    color: "#0B1020"

    property string activePage: "Overview"
    property string toastText: ""
    readonly property string activeWorkspace: orbit.activeWorkspace
    readonly property var pageNames: ["Overview", "Workspaces", "Applications", "Files", "Settings"]

    function notify(message) {
        toastText = message
        toastTimer.restart()
    }

    function openPage(page) {
        activePage = page
        commandPalette.close()
    }

    function launchApp(name) {
        var result = ""
        if (name === "Browser")
            result = orbit.launchBrowser()
        else if (name === "Files")
            result = orbit.openFolder("home")
        else if (name === "Terminal")
            result = orbit.launchTerminal()
        else if (name === "Settings") {
            openPage("Settings")
            result = "ORBIT Settings opened."
        }
        notify(result)
    }

    function switchWorkspace(name) {
        orbit.setActiveWorkspace(name)
        notify("Switched to " + name + " space. Selection is saved on this computer.")
    }

    Shortcut { sequence: "Ctrl+Space"; onActivated: commandPalette.open() }
    Shortcut { sequence: "Ctrl+K"; onActivated: commandPalette.open() }
    Shortcut {
        sequence: "Escape"
        onActivated: {
            commandPalette.close()
            quickPanel.close()
        }
    }

    Timer {
        id: toastTimer
        interval: 2800
        onTriggered: root.toastText = ""
    }

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#10182B" }
            GradientStop { position: 0.58; color: "#0B1020" }
            GradientStop { position: 1.0; color: "#080D18" }
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.preferredWidth: 238
            Layout.fillHeight: true
            color: "#0B1120"
            border.color: "#1C2940"
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.leftMargin: 18
                anchors.rightMargin: 18
                anchors.topMargin: 24
                anchors.bottomMargin: 18
                spacing: 0

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 11
                    Rectangle {
                        width: 38
                        height: 38
                        radius: 13
                        gradient: Gradient {
                            GradientStop { position: 0; color: "#91D7F5" }
                            GradientStop { position: 1; color: "#8278E8" }
                        }
                        Text {
                            anchors.centerIn: parent
                            text: "O"
                            color: "#101729"
                            font.pixelSize: 23
                            font.weight: Font.Black
                        }
                    }
                    ColumnLayout {
                        spacing: 1
                        Text { text: "ORBIT"; color: "#F3F6FC"; font.pixelSize: 17; font.weight: Font.Bold; font.letterSpacing: 2.4 }
                        Text { text: "ALPHA 0.2  •  FUNCTIONAL"; color: "#71819B"; font.pixelSize: 8; font.weight: Font.DemiBold; font.letterSpacing: 1.0 }
                    }
                }

                Item { Layout.preferredHeight: 38 }
                Text {
                    text: "YOUR SPACE"
                    color: "#64738D"
                    font.pixelSize: 9
                    font.weight: Font.Bold
                    font.letterSpacing: 1.8
                    leftPadding: 9
                }
                Item { Layout.preferredHeight: 10 }

                Repeater {
                    model: [
                        { label: "Overview", glyph: "◫" },
                        { label: "Workspaces", glyph: "▦" },
                        { label: "Applications", glyph: "▤" },
                        { label: "Files", glyph: "▱" },
                        { label: "Settings", glyph: "⚙" }
                    ]
                    delegate: Button {
                        required property var modelData
                        Layout.fillWidth: true
                        Layout.preferredHeight: 43
                        Layout.bottomMargin: 5
                        hoverEnabled: true
                        flat: true
                        onClicked: root.openPage(modelData.label)
                        background: Rectangle {
                            radius: 11
                            color: root.activePage === modelData.label ? "#1C2A42" : (parent.hovered ? "#141F32" : "transparent")
                            border.width: root.activePage === modelData.label ? 1 : 0
                            border.color: "#2B4261"
                        }
                        contentItem: RowLayout {
                            spacing: 12
                            Text {
                                text: modelData.glyph
                                color: root.activePage === modelData.label ? "#9DDCF5" : "#8796AC"
                                font.pixelSize: 17
                                Layout.preferredWidth: 23
                                horizontalAlignment: Text.AlignHCenter
                            }
                            Text {
                                text: modelData.label
                                color: root.activePage === modelData.label ? "#F4F7FC" : "#A1AEC1"
                                font.pixelSize: 12
                                font.weight: root.activePage === modelData.label ? Font.DemiBold : Font.Medium
                                Layout.fillWidth: true
                            }
                            Rectangle {
                                visible: root.activePage === modelData.label
                                width: 5
                                height: 5
                                radius: 3
                                color: "#8BD4F1"
                            }
                        }
                    }
                }

                Item { Layout.fillHeight: true }

                Rectangle {
                    Layout.fillWidth: true
                    radius: 14
                    color: "#111D30"
                    border.color: "#22324A"
                    implicitHeight: 126
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 13
                        spacing: 7
                        RowLayout {
                            Text { text: "✦"; color: "#BCA8F8"; font.pixelSize: 14 }
                            Text { text: "Your current space"; color: "#DDE5F2"; font.pixelSize: 11; font.weight: Font.DemiBold }
                        }
                        Text {
                            text: root.activeWorkspace
                            color: "#9DDCF5"
                            font.pixelSize: 16
                            font.weight: Font.DemiBold
                        }
                        Button {
                            text: "Switch space  →"
                            flat: true
                            onClicked: root.openPage("Workspaces")
                            contentItem: Text { text: parent.text; color: "#A9DDF4"; font.pixelSize: 10; font.weight: Font.DemiBold; verticalAlignment: Text.AlignVCenter }
                            background: Rectangle { color: "transparent" }
                        }
                    }
                }

                Item { Layout.preferredHeight: 15 }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 9
                    Rectangle {
                        width: 29
                        height: 29
                        radius: 10
                        color: "#26344A"
                        Text { anchors.centerIn: parent; text: "O"; color: "#DDE8F7"; font.pixelSize: 12; font.weight: Font.Bold }
                    }
                    ColumnLayout {
                        spacing: 1
                        Text { text: "Local session"; color: "#D7E0EE"; font.pixelSize: 10; font.weight: Font.DemiBold }
                        Text { text: "ORBIT Alpha 0.2"; color: "#6D7D96"; font.pixelSize: 9 }
                    }
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 70
                color: "#0B1120"
                border.color: "#1C2940"
                border.width: 1
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 30
                    anchors.rightMargin: 30
                    Text { text: root.activePage; color: "#F1F5FC"; font.pixelSize: 14; font.weight: Font.DemiBold }
                    Text { text: " / "; color: "#43516A"; font.pixelSize: 13 }
                    Text { text: root.activeWorkspace + " space"; color: "#7889A2"; font.pixelSize: 11 }
                    Item { Layout.fillWidth: true }
                    Button {
                        Layout.preferredWidth: 240
                        Layout.preferredHeight: 37
                        onClicked: commandPalette.open()
                        background: Rectangle { radius: 10; color: "#131E31"; border.color: "#273750" }
                        contentItem: RowLayout {
                            Text { text: "⌕"; color: "#90A2BB"; font.pixelSize: 20 }
                            Text { text: "Search apps or actions…"; color: "#8292AA"; font.pixelSize: 10; Layout.fillWidth: true }
                            Rectangle {
                                radius: 5
                                color: "#202D43"
                                border.color: "#354660"
                                implicitWidth: 37
                                implicitHeight: 21
                                Text { anchors.centerIn: parent; text: "Ctrl K"; color: "#A6B5C9"; font.pixelSize: 8 }
                            }
                        }
                    }
                    Button {
                        Layout.preferredWidth: 38
                        Layout.preferredHeight: 38
                        onClicked: quickPanel.open()
                        background: Rectangle { radius: 11; color: parent.hovered ? "#1A2940" : "#131E31"; border.color: "#273750" }
                        contentItem: Text { text: "☷"; color: "#B9C8D9"; font.pixelSize: 17; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                    }
                    Rectangle { width: 1; height: 25; color: "#26334A" }
                    ColumnLayout {
                        spacing: 1
                        Text { text: Qt.formatDate(new Date(), "dddd").toUpperCase(); color: "#8292AA"; font.pixelSize: 8; font.weight: Font.Bold; font.letterSpacing: 1.2; horizontalAlignment: Text.AlignRight; Layout.fillWidth: true }
                        Text { text: Qt.formatDate(new Date(), "MMM dd, yyyy"); color: "#E0E8F3"; font.pixelSize: 11; font.weight: Font.DemiBold; horizontalAlignment: Text.AlignRight; Layout.fillWidth: true }
                    }
                }
            }

            StackLayout {
                id: pageStack
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: Math.max(0, root.pageNames.indexOf(root.activePage))

                ScrollView {
                    clip: true
                    ColumnLayout {
                        width: parent.width
                        spacing: 20
                        Item { Layout.preferredHeight: 10 }
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.leftMargin: 28
                            Layout.rightMargin: 28
                            Layout.preferredHeight: 190
                            radius: 20
                            clip: true
                            gradient: Gradient {
                                GradientStop { position: 0; color: "#1C3150" }
                                GradientStop { position: 0.55; color: "#192946" }
                                GradientStop { position: 1; color: "#2A2850" }
                            }
                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 30
                                anchors.rightMargin: 30
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 10
                                    Text { text: "A CALMER WAY TO COMPUTE"; color: "#9FCBE9"; font.pixelSize: 9; font.weight: Font.Bold; font.letterSpacing: 2.1 }
                                    Text { text: "Good to see you.\nLet's make room for what matters."; color: "#F3F6FC"; font.pixelSize: 25; font.weight: Font.DemiBold; lineHeight: 1.15 }
                                    Text { text: "Your " + root.activeWorkspace + " space is ready."; color: "#A9BBD2"; font.pixelSize: 11 }
                                    RowLayout {
                                        spacing: 10
                                        Button {
                                            text: "Open command center"
                                            onClicked: commandPalette.open()
                                            contentItem: Text { text: parent.text; color: "#122039"; font.pixelSize: 10; font.weight: Font.DemiBold; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                            background: Rectangle { radius: 9; color: "#B8E4F7" }
                                            Layout.preferredHeight: 35
                                            Layout.preferredWidth: 160
                                        }
                                        Button {
                                            text: "Manage spaces"
                                            onClicked: root.openPage("Workspaces")
                                            contentItem: Text { text: parent.text; color: "#D5E4F3"; font.pixelSize: 10; font.weight: Font.DemiBold; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                            background: Rectangle { radius: 9; color: "#FFFFFF"; opacity: 0.08; border.color: "#FFFFFF"; border.width: 1 }
                                            Layout.preferredHeight: 35
                                            Layout.preferredWidth: 125
                                        }
                                    }
                                }
                                Item {
                                    Layout.preferredWidth: 220
                                    Layout.fillHeight: true
                                    Text {
                                        anchors.centerIn: parent
                                        text: "◌"
                                        color: "#A8DFF7"
                                        font.pixelSize: 128
                                        opacity: 0.7
                                    }
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.leftMargin: 28
                            Layout.rightMargin: 28
                            Text { text: "System pulse"; color: "#EFF3FA"; font.pixelSize: 15; font.weight: Font.DemiBold }
                            Item { Layout.fillWidth: true }
                            Text { text: "LIVE · " + systemMonitor.storageRoot; color: "#7188A4"; font.pixelSize: 8; font.weight: Font.Bold; font.letterSpacing: 1 }
                        }

                        RowLayout {
                            visible: orbit.showSystemMetrics
                            Layout.fillWidth: true
                            Layout.leftMargin: 28
                            Layout.rightMargin: 28
                            spacing: 12
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 112
                                radius: 14
                                color: "#111B2C"
                                border.color: "#202F46"
                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 15
                                    spacing: 6
                                    Text { text: "PROCESSOR"; color: "#7F93AD"; font.pixelSize: 8; font.weight: Font.Bold; font.letterSpacing: 1.2 }
                                    Text { text: systemMonitor.cpuPercent < 0 ? "Measuring…" : systemMonitor.cpuPercent + "%"; color: "#EAF3FB"; font.pixelSize: 23; font.weight: Font.DemiBold }
                                    Text { text: "Total CPU utilization"; color: "#7D8FA7"; font.pixelSize: 9 }
                                }
                            }
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 112
                                radius: 14
                                color: "#111B2C"
                                border.color: "#202F46"
                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 15
                                    spacing: 6
                                    Text { text: "MEMORY"; color: "#7F93AD"; font.pixelSize: 8; font.weight: Font.Bold; font.letterSpacing: 1.2 }
                                    Text { text: systemMonitor.memoryTotalBytes > 0 ? systemMonitor.memoryPercent + "%" : "Unavailable"; color: "#EAF3FB"; font.pixelSize: 23; font.weight: Font.DemiBold }
                                    Text { text: systemMonitor.memoryTotalBytes > 0 ? systemMonitor.formatBytes(systemMonitor.memoryUsedBytes) + " / " + systemMonitor.formatBytes(systemMonitor.memoryTotalBytes) : "Memory metrics unavailable"; color: "#7D8FA7"; font.pixelSize: 9 }
                                }
                            }
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 112
                                radius: 14
                                color: "#111B2C"
                                border.color: "#202F46"
                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 15
                                    spacing: 6
                                    Text { text: "SYSTEM DISK"; color: "#7F93AD"; font.pixelSize: 8; font.weight: Font.Bold; font.letterSpacing: 1.2 }
                                    Text { text: systemMonitor.storageTotalBytes > 0 ? systemMonitor.storagePercent + "%" : "Unavailable"; color: "#EAF3FB"; font.pixelSize: 23; font.weight: Font.DemiBold }
                                    Text { text: systemMonitor.storageTotalBytes > 0 ? systemMonitor.formatBytes(systemMonitor.storageAvailableBytes) + " available" : "Disk metrics unavailable"; color: "#7D8FA7"; font.pixelSize: 9 }
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.leftMargin: 28
                            Layout.rightMargin: 28
                            Text { text: "Quick launch"; color: "#EFF3FA"; font.pixelSize: 15; font.weight: Font.DemiBold }
                            Item { Layout.fillWidth: true }
                            Button {
                                text: "All applications  →"
                                flat: true
                                onClicked: root.openPage("Applications")
                                contentItem: Text { text: parent.text; color: "#91CDE9"; font.pixelSize: 10; font.weight: Font.DemiBold }
                                background: Rectangle { color: "transparent" }
                            }
                        }

                        GridLayout {
                            Layout.fillWidth: true
                            Layout.leftMargin: 28
                            Layout.rightMargin: 28
                            columns: 4
                            columnSpacing: 10
                            Repeater {
                                model: [
                                    { name: "Browser", description: "Default web browser", glyph: "◉", accent: "#75C8F2" },
                                    { name: "Files", description: "Open your home folder", glyph: "▱", accent: "#D9B879" },
                                    { name: "Terminal", description: "System terminal", glyph: ">_", accent: "#9DD6B0" },
                                    { name: "Settings", description: "ORBIT preferences", glyph: "⚙", accent: "#B9A4F5" }
                                ]
                                delegate: Button {
                                    required property var modelData
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 88
                                    hoverEnabled: true
                                    onClicked: root.launchApp(modelData.name)
                                    background: Rectangle { radius: 13; color: parent.hovered ? "#192840" : "#111B2C"; border.color: parent.hovered ? "#3C5778" : "#202E45" }
                                    contentItem: ColumnLayout {
                                        spacing: 4
                                        Text { text: modelData.glyph; color: modelData.accent; font.pixelSize: 20 }
                                        Text { text: modelData.name; color: "#E8EEF7"; font.pixelSize: 10; font.weight: Font.DemiBold }
                                        Text { text: modelData.description; color: "#7789A2"; font.pixelSize: 8; elide: Text.ElideRight; Layout.fillWidth: true }
                                    }
                                }
                            }
                        }
                        Item { Layout.preferredHeight: 18 }
                    }
                }

                ScrollView {
                    clip: true
                    ColumnLayout {
                        width: parent.width
                        spacing: 18
                        Item { Layout.preferredHeight: 10 }
                        ColumnLayout {
                            Layout.leftMargin: 28
                            Layout.rightMargin: 28
                            spacing: 5
                            Text { text: "Choose your context"; color: "#F2F5FA"; font.pixelSize: 23; font.weight: Font.DemiBold }
                            Text { text: "ORBIT remembers your selected space on this computer."; color: "#8798B0"; font.pixelSize: 11 }
                        }
                        GridLayout {
                            Layout.fillWidth: true
                            Layout.leftMargin: 28
                            Layout.rightMargin: 28
                            columns: 3
                            columnSpacing: 14
                            rowSpacing: 14
                            Repeater {
                                model: [
                                    { name: "Focus", description: "Deep work with fewer distractions.", glyph: "◈", accent: "#8BD6EE" },
                                    { name: "Development", description: "Code, terminals and engineering tools.", glyph: "⌘", accent: "#AFA0F6" },
                                    { name: "Personal", description: "Your everyday tasks and personal projects.", glyph: "⌂", accent: "#9FD9B1" }
                                ]
                                delegate: Button {
                                    required property var modelData
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 190
                                    onClicked: root.switchWorkspace(modelData.name)
                                    background: Rectangle {
                                        radius: 16
                                        color: orbit.activeWorkspace === modelData.name ? "#1A2D47" : "#111B2C"
                                        border.width: orbit.activeWorkspace === modelData.name ? 2 : 1
                                        border.color: orbit.activeWorkspace === modelData.name ? "#5B88A9" : "#26364F"
                                    }
                                    contentItem: ColumnLayout {
                                        anchors.fill: parent
                                        spacing: 10
                                        Text { text: modelData.glyph; color: modelData.accent; font.pixelSize: 32 }
                                        Text { text: modelData.name; color: "#EDF3FA"; font.pixelSize: 16; font.weight: Font.DemiBold }
                                        Text { text: modelData.description; color: "#899AB1"; font.pixelSize: 10; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                                        Item { Layout.fillHeight: true }
                                        Text {
                                            text: orbit.activeWorkspace === modelData.name ? "✓ CURRENT SPACE" : "SELECT SPACE"
                                            color: orbit.activeWorkspace === modelData.name ? "#9DDCF5" : "#7187A1"
                                            font.pixelSize: 8
                                            font.weight: Font.Bold
                                            font.letterSpacing: 1.1
                                        }
                                    }
                                }
                            }
                        }
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.leftMargin: 28
                            Layout.rightMargin: 28
                            radius: 12
                            color: "#111B2C"
                            border.color: "#202F46"
                            implicitHeight: 66
                            Text {
                                anchors.fill: parent
                                anchors.margins: 15
                                verticalAlignment: Text.AlignVCenter
                                text: "Note: Alpha 0.2 spaces are persistent ORBIT work contexts inside this application. Native Windows/Linux virtual desktop integration comes in a later system-session milestone."
                                color: "#8A9CB3"
                                font.pixelSize: 9
                                wrapMode: Text.WordWrap
                            }
                        }
                        Item { Layout.preferredHeight: 18 }
                    }
                }

                ScrollView {
                    clip: true
                    ColumnLayout {
                        width: parent.width
                        spacing: 18
                        Item { Layout.preferredHeight: 10 }
                        ColumnLayout {
                            Layout.leftMargin: 28
                            Layout.rightMargin: 28
                            spacing: 5
                            Text { text: "Applications"; color: "#F2F5FA"; font.pixelSize: 23; font.weight: Font.DemiBold }
                            Text { text: "Launch tools using the applications configured on your computer."; color: "#8798B0"; font.pixelSize: 11 }
                        }
                        GridLayout {
                            Layout.fillWidth: true
                            Layout.leftMargin: 28
                            Layout.rightMargin: 28
                            columns: 2
                            columnSpacing: 14
                            rowSpacing: 14
                            Repeater {
                                model: [
                                    { name: "Browser", description: "Open DuckDuckGo in your default browser.", glyph: "◉", accent: "#75C8F2", action: "Browser" },
                                    { name: "Files", description: "Open your home folder in the system file manager.", glyph: "▱", accent: "#D9B879", action: "Files" },
                                    { name: "Terminal", description: "Launch an installed terminal application.", glyph: ">_", accent: "#9DD6B0", action: "Terminal" },
                                    { name: "Settings", description: "Configure ORBIT preferences.", glyph: "⚙", accent: "#B9A4F5", action: "Settings" }
                                ]
                                delegate: Button {
                                    required property var modelData
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 112
                                    onClicked: root.launchApp(modelData.action)
                                    background: Rectangle { radius: 14; color: parent.hovered ? "#192840" : "#111B2C"; border.color: parent.hovered ? "#3C5778" : "#202E45" }
                                    contentItem: RowLayout {
                                        spacing: 14
                                        Text { text: modelData.glyph; color: modelData.accent; font.pixelSize: 28 }
                                        ColumnLayout {
                                            Layout.fillWidth: true
                                            spacing: 5
                                            Text { text: modelData.name; color: "#E8EEF7"; font.pixelSize: 13; font.weight: Font.DemiBold }
                                            Text { text: modelData.description; color: "#8394AC"; font.pixelSize: 9; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                                        }
                                        Text { text: "↗"; color: "#7188A4"; font.pixelSize: 15 }
                                    }
                                }
                            }
                        }
                        Item { Layout.preferredHeight: 18 }
                    }
                }

                ScrollView {
                    clip: true
                    ColumnLayout {
                        width: parent.width
                        spacing: 18
                        Item { Layout.preferredHeight: 10 }
                        ColumnLayout {
                            Layout.leftMargin: 28
                            Layout.rightMargin: 28
                            spacing: 5
                            Text { text: "Your files"; color: "#F2F5FA"; font.pixelSize: 23; font.weight: Font.DemiBold }
                            Text { text: "Choose a location to open it in your system file manager."; color: "#8798B0"; font.pixelSize: 11 }
                        }
                        GridLayout {
                            Layout.fillWidth: true
                            Layout.leftMargin: 28
                            Layout.rightMargin: 28
                            columns: 2
                            columnSpacing: 12
                            rowSpacing: 12
                            Repeater {
                                model: [
                                    { id: "home", name: "Home", glyph: "⌂" },
                                    { id: "desktop", name: "Desktop", glyph: "▣" },
                                    { id: "documents", name: "Documents", glyph: "▤" },
                                    { id: "downloads", name: "Downloads", glyph: "↓" },
                                    { id: "pictures", name: "Pictures", glyph: "▧" },
                                    { id: "music", name: "Music", glyph: "♫" },
                                    { id: "videos", name: "Videos", glyph: "▷" }
                                ]
                                delegate: Button {
                                    required property var modelData
                                    Layout.fillWidth: true
                                    Layout.preferredHeight: 94
                                    onClicked: root.notify(orbit.openFolder(modelData.id))
                                    background: Rectangle { radius: 13; color: parent.hovered ? "#192840" : "#111B2C"; border.color: parent.hovered ? "#3C5778" : "#202E45" }
                                    contentItem: RowLayout {
                                        spacing: 12
                                        Text { text: modelData.glyph; color: "#D9B879"; font.pixelSize: 24 }
                                        ColumnLayout {
                                            Layout.fillWidth: true
                                            spacing: 4
                                            Text { text: modelData.name; color: "#E8EEF7"; font.pixelSize: 11; font.weight: Font.DemiBold }
                                            Text { text: orbit.folderPath(modelData.id); color: "#7789A2"; font.pixelSize: 8; elide: Text.ElideMiddle; Layout.fillWidth: true }
                                        }
                                        Text { text: "↗"; color: "#657B96"; font.pixelSize: 13 }
                                    }
                                }
                            }
                        }
                        Item { Layout.preferredHeight: 18 }
                    }
                }

                ScrollView {
                    clip: true
                    ColumnLayout {
                        width: parent.width
                        spacing: 18
                        Item { Layout.preferredHeight: 10 }
                        ColumnLayout {
                            Layout.leftMargin: 28
                            Layout.rightMargin: 28
                            spacing: 5
                            Text { text: "Settings"; color: "#F2F5FA"; font.pixelSize: 23; font.weight: Font.DemiBold }
                            Text { text: "Preferences are saved locally on this computer."; color: "#8798B0"; font.pixelSize: 11 }
                        }
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.leftMargin: 28
                            Layout.rightMargin: 28
                            radius: 14
                            color: "#111B2C"
                            border.color: "#202F46"
                            implicitHeight: 104
                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 18
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 5
                                    Text { text: "Show system metrics"; color: "#E8EEF7"; font.pixelSize: 12; font.weight: Font.DemiBold }
                                    Text { text: "Display live CPU, memory and storage information on Overview."; color: "#8394AC"; font.pixelSize: 9; wrapMode: Text.WordWrap; Layout.fillWidth: true }
                                }
                                Switch {
                                    checked: orbit.showSystemMetrics
                                    onToggled: orbit.setShowSystemMetrics(checked)
                                }
                            }
                        }
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.leftMargin: 28
                            Layout.rightMargin: 28
                            radius: 14
                            color: "#111B2C"
                            border.color: "#202F46"
                            implicitHeight: 112
                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 18
                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 5
                                    Text { text: "Metrics refresh interval"; color: "#E8EEF7"; font.pixelSize: 12; font.weight: Font.DemiBold }
                                    Text { text: "Choose how often ORBIT samples system usage."; color: "#8394AC"; font.pixelSize: 9 }
                                }
                                ComboBox {
                                    Layout.preferredWidth: 150
                                    model: ["1 second", "2 seconds", "5 seconds"]
                                    currentIndex: orbit.metricsRefreshIntervalMs === 1000 ? 0
                                                : (orbit.metricsRefreshIntervalMs === 5000 ? 2 : 1)
                                    onActivated: orbit.setMetricsRefreshIntervalMs([1000, 2000, 5000][currentIndex])
                                }
                            }
                        }
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.leftMargin: 28
                            Layout.rightMargin: 28
                            radius: 14
                            color: "#111B2C"
                            border.color: "#202F46"
                            implicitHeight: 108
                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 18
                                spacing: 5
                                Text { text: "Current workspace"; color: "#E8EEF7"; font.pixelSize: 12; font.weight: Font.DemiBold }
                                Text { text: root.activeWorkspace; color: "#9DDCF5"; font.pixelSize: 16; font.weight: Font.DemiBold }
                                Text { text: "This choice is restored the next time you run ORBIT."; color: "#8394AC"; font.pixelSize: 9 }
                            }
                        }
                        Rectangle {
                            Layout.fillWidth: true
                            Layout.leftMargin: 28
                            Layout.rightMargin: 28
                            radius: 12
                            color: "#162238"
                            border.color: "#2B4261"
                            implicitHeight: 62
                            Text {
                                anchors.fill: parent
                                anchors.margins: 15
                                verticalAlignment: Text.AlignVCenter
                                text: "Alpha 0.2 settings are stored using the local Qt settings store. Hardware controls, startup integration and OS-wide preferences are not included yet."
                                color: "#8A9CB3"
                                font.pixelSize: 9
                                wrapMode: Text.WordWrap
                            }
                        }
                        Item { Layout.preferredHeight: 18 }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 32
                color: "#0A101D"
                border.color: "#1A263A"
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 20
                    anchors.rightMargin: 20
                    Text { text: "●  ORBIT services: local desktop integration"; color: "#6F839D"; font.pixelSize: 8 }
                    Item { Layout.fillWidth: true }
                    Text { text: "ALPHA 0.2.0  |  RUNNING AS AN APP"; color: "#647690"; font.pixelSize: 8; font.weight: Font.Bold; font.letterSpacing: 0.7 }
                }
            }
        }
    }

    Popup {
        id: commandPalette
        width: Math.min(560, root.width - 50)
        height: 390
        x: (root.width - width) / 2
        y: 92
        modal: true
        focus: true
        padding: 16
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        background: Rectangle { radius: 17; color: "#121D30"; border.color: "#405675"; border.width: 1 }
        contentItem: ColumnLayout {
            spacing: 11
            Text { text: "ORBIT COMMAND"; color: "#90CFEF"; font.pixelSize: 9; font.weight: Font.Bold; font.letterSpacing: 1.8 }
            TextField {
                id: commandInput
                Layout.fillWidth: true
                placeholderText: "Try: browser, files, terminal, settings…"
                color: "#EDF3FB"
                placeholderTextColor: "#72849E"
                background: Rectangle { radius: 10; color: "#0B1424"; border.color: "#344B68" }
                onAccepted: {
                    var query = text.trim().toLowerCase()
                    if (query.indexOf("browser") >= 0 || query.indexOf("web") >= 0)
                        root.launchApp("Browser")
                    else if (query.indexOf("file") >= 0 || query.indexOf("home") >= 0)
                        root.launchApp("Files")
                    else if (query.indexOf("terminal") >= 0 || query.indexOf("shell") >= 0)
                        root.launchApp("Terminal")
                    else if (query.indexOf("setting") >= 0)
                        root.launchApp("Settings")
                    else
                        root.notify("No matching ORBIT action. Try browser, files, terminal or settings.")
                    commandPalette.close()
                    text = ""
                }
            }
            Text { text: "ACTIONS"; color: "#687D98"; font.pixelSize: 8; font.weight: Font.Bold; font.letterSpacing: 1.3 }
            Repeater {
                model: ["Browser", "Files", "Terminal", "Settings", "Workspaces"]
                delegate: Button {
                    required property string modelData
                    Layout.fillWidth: true
                    Layout.preferredHeight: 38
                    onClicked: {
                        if (modelData === "Workspaces")
                            root.openPage("Workspaces")
                        else
                            root.launchApp(modelData)
                        commandPalette.close()
                    }
                    background: Rectangle { radius: 8; color: parent.hovered ? "#1D2D46" : "transparent" }
                    contentItem: RowLayout {
                        Text { text: "⌘"; color: "#8CBBD9"; font.pixelSize: 13 }
                        Text { text: "Open " + modelData; color: "#CFD9E7"; font.pixelSize: 10; Layout.fillWidth: true }
                        Text { text: "↵"; color: "#6E819B"; font.pixelSize: 10 }
                    }
                }
            }
            Item { Layout.fillHeight: true }
            Text { text: "ENTER to run  ·  ESC to close"; color: "#60738D"; font.pixelSize: 8 }
        }
        onOpened: commandInput.forceActiveFocus()
        onClosed: commandInput.text = ""
    }

    Popup {
        id: quickPanel
        width: 320
        height: 280
        x: root.width - width - 22
        y: 78
        modal: true
        focus: true
        padding: 16
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        background: Rectangle { radius: 17; color: "#121D30"; border.color: "#354B68" }
        contentItem: ColumnLayout {
            spacing: 12
            RowLayout {
                Text { text: "System pulse"; color: "#F0F4FA"; font.pixelSize: 13; font.weight: Font.DemiBold }
                Item { Layout.fillWidth: true }
                Text { text: "LIVE"; color: "#8BCFA6"; font.pixelSize: 8; font.weight: Font.Bold }
            }
            Text {
                text: "CPU: " + (systemMonitor.cpuPercent < 0 ? "Measuring…" : systemMonitor.cpuPercent + "%")
                color: "#C9D6E5"
                font.pixelSize: 10
            }
            Text {
                text: "Memory: " + (systemMonitor.memoryTotalBytes > 0 ? systemMonitor.formatBytes(systemMonitor.memoryUsedBytes) + " / " + systemMonitor.formatBytes(systemMonitor.memoryTotalBytes) : "Unavailable")
                color: "#C9D6E5"
                font.pixelSize: 10
            }
            Text {
                text: "Disk available: " + (systemMonitor.storageTotalBytes > 0 ? systemMonitor.formatBytes(systemMonitor.storageAvailableBytes) : "Unavailable")
                color: "#C9D6E5"
                font.pixelSize: 10
            }
            Rectangle { Layout.fillWidth: true; height: 1; color: "#263750" }
            Button {
                text: "Open Settings"
                Layout.fillWidth: true
                onClicked: { root.openPage("Settings"); quickPanel.close() }
            }
            Text { text: "Hardware controls will be connected in a later OS integration milestone."; color: "#6F829B"; font.pixelSize: 8; wrapMode: Text.WordWrap; Layout.fillWidth: true }
        }
    }

    Rectangle {
        visible: root.toastText.length > 0
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 48
        width: Math.min(560, root.width - 60)
        height: 42
        radius: 12
        color: "#23344D"
        border.color: "#57718F"
        z: 100
        Text {
            anchors.centerIn: parent
            width: parent.width - 24
            text: root.toastText
            color: "#EDF4FB"
            font.pixelSize: 10
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideRight
        }
    }
}
