import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

ApplicationWindow {
    id: root
    width: 1440
    height: 900
    minimumWidth: 1120
    minimumHeight: 720
    visible: true
    title: "ORBIT — Computing, without friction"
    color: "#0B1020"

    property string activePage: "Overview"
    property string activeWorkspace: "Focus"
    property string toastText: ""
    property var navItems: [
        { label: "Overview", glyph: "◫" },
        { label: "Workspaces", glyph: "▦" },
        { label: "Applications", glyph: "▤" },
        { label: "Files", glyph: "▱" },
        { label: "Settings", glyph: "⚙" }
    ]
    property var appItems: [
        { name: "Browser", detail: "Web and research", glyph: "◉", accent: "#75C8F2" },
        { name: "Files", detail: "Your documents", glyph: "▱", accent: "#D9B879" },
        { name: "Terminal", detail: "Developer tools", glyph: ">_", accent: "#9DD6B0" },
        { name: "Settings", detail: "Make it yours", glyph: "⚙", accent: "#B9A4F5" }
    ]

    function notify(message) {
        toastText = message
        toastTimer.restart()
    }

    function openPage(page) {
        activePage = page
        if (page !== "Overview")
            notify(page + " is part of the next prototype increment")
    }

    Shortcut {
        sequence: "Ctrl+Space"
        onActivated: commandPalette.open()
    }

    Shortcut {
        sequence: "Ctrl+K"
        onActivated: commandPalette.open()
    }

    Shortcut {
        sequence: "Escape"
        onActivated: {
            commandPalette.close()
            quickPanel.close()
        }
    }

    Timer {
        id: toastTimer
        interval: 2600
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

        // Primary navigation rail
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
                        width: 38; height: 38; radius: 13
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
                        Text { text: "ALPHA 0.1  •  PREVIEW"; color: "#71819B"; font.pixelSize: 8; font.weight: Font.DemiBold; font.letterSpacing: 1.1 }
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
                    model: root.navItems
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
                                width: 5; height: 5; radius: 3
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
                            Text { text: "A little more focus"; color: "#DDE5F2"; font.pixelSize: 11; font.weight: Font.DemiBold }
                        }
                        Text {
                            text: "Your workspace should work the way you do."
                            color: "#8292AA"
                            font.pixelSize: 10
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                        Button {
                            text: "Explore Spaces  →"
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
                    Rectangle { width: 29; height: 29; radius: 10; color: "#26344A"; Text { anchors.centerIn: parent; text: "K"; color: "#DDE8F7"; font.pixelSize: 12; font.weight: Font.Bold } }
                    ColumnLayout {
                        spacing: 1
                        Text { text: "Local session"; color: "#D7E0EE"; font.pixelSize: 10; font.weight: Font.DemiBold }
                        Text { text: "Prototype user"; color: "#6D7D96"; font.pixelSize: 9 }
                    }
                    Item { Layout.fillWidth: true }
                    Text { text: "•••"; color: "#75849A"; font.pixelSize: 13 }
                }
            }
        }

        // Main content area
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // Top bar
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
                    Text {
                        text: root.activePage
                        color: "#F1F5FC"
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                    }
                    Text { text: " / "; color: "#43516A"; font.pixelSize: 13 }
                    Text { text: "Personal computer"; color: "#7889A2"; font.pixelSize: 11 }
                    Item { Layout.fillWidth: true }
                    Button {
                        Layout.preferredWidth: 240
                        Layout.preferredHeight: 37
                        onClicked: commandPalette.open()
                        background: Rectangle { radius: 10; color: "#131E31"; border.color: "#273750" }
                        contentItem: RowLayout {
                            Text { text: "⌕"; color: "#90A2BB"; font.pixelSize: 20 }
                            Text { text: "Search or run a command…"; color: "#8292AA"; font.pixelSize: 10; Layout.fillWidth: true }
                            Rectangle { radius: 5; color: "#202D43"; border.color: "#354660"; implicitWidth: 37; implicitHeight: 21; Text { anchors.centerIn: parent; text: "Ctrl K"; color: "#A6B5C9"; font.pixelSize: 8 } }
                        }
                    }
                    Button {
                        Layout.preferredWidth: 38; Layout.preferredHeight: 38
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

            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                ScrollBar.vertical.policy: ScrollBar.AsNeeded

                ColumnLayout {
                    width: parent.width
                    spacing: 22
                    anchors.margins: 30

                    // Welcome / hero panel
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 205
                        radius: 20
                        clip: true
                        gradient: Gradient {
                            GradientStop { position: 0; color: "#1C3150" }
                            GradientStop { position: 0.55; color: "#192946" }
                            GradientStop { position: 1; color: "#2A2850" }
                        }
                        Rectangle { width: 260; height: 260; radius: 130; color: "#4B77A0"; opacity: 0.10; x: parent.width - 230; y: -100 }
                        Rectangle { width: 180; height: 180; radius: 90; color: "#B09AF5"; opacity: 0.10; x: parent.width - 145; y: 110 }
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 30
                            anchors.rightMargin: 30
                            spacing: 20
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 10
                                Text { text: "A CALMER WAY TO COMPUTE"; color: "#9FCBE9"; font.pixelSize: 9; font.weight: Font.Bold; font.letterSpacing: 2.1 }
                                Text {
                                    text: root.activePage === "Overview" ? "Good to see you.\nLet's make room for what matters." : root.activePage + " is taking shape."
                                    color: "#F3F6FC"
                                    font.pixelSize: 25
                                    font.weight: Font.DemiBold
                                    lineHeight: 1.15
                                }
                                Text { text: "A focused workspace, with everything in its place."; color: "#A9BBD2"; font.pixelSize: 11 }
                                Item { Layout.preferredHeight: 1 }
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
                                        flat: true
                                        onClicked: root.openPage("Workspaces")
                                        contentItem: Text { text: parent.text; color: "#D5E4F3"; font.pixelSize: 10; font.weight: Font.DemiBold; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                                        background: Rectangle { radius: 9; color: "#FFFFFF"; opacity: 0.08; border.color: "#FFFFFF"; border.width: 1 }
                                        Layout.preferredHeight: 35
                                        Layout.preferredWidth: 125
                                    }
                                }
                            }
                            Item {
                                Layout.preferredWidth: 250
                                Layout.fillHeight: true
                                Rectangle {
                                    anchors.centerIn: parent
                                    width: 190; height: 150; radius: 22
                                    color: "#FFFFFF"; opacity: 0.06
                                    border.color: "#D9E9FA"; border.width: 1
                                }
                                Rectangle {
                                    anchors.centerIn: parent
                                    width: 150; height: 116; radius: 18
                                    color: "#B9DDF0"; opacity: 0.09
                                    border.color: "#CBE7F8"; border.width: 1
                                }
                                Rectangle {
                                    anchors.centerIn: parent
                                    width: 112; height: 82; radius: 15
                                    color: "#0D172A"; opacity: 0.55
                                    border.color: "#A9D8F1"; border.width: 1
                                    Column {
                                        anchors.centerIn: parent
                                        spacing: 7
                                        Text { anchors.horizontalCenter: parent.horizontalCenter; text: "◌"; color: "#A8DFF7"; font.pixelSize: 31 }
                                        Text { anchors.horizontalCenter: parent.horizontalCenter; text: "IN YOUR ORBIT"; color: "#D4E8F7"; font.pixelSize: 7; font.weight: Font.Bold; font.letterSpacing: 1.3 }
                                    }
                                }
                            }
                        }
                    }

                    // Workspace section
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: "Your spaces"; color: "#EFF3FA"; font.pixelSize: 15; font.weight: Font.DemiBold }
                        Item { Layout.fillWidth: true }
                        Button {
                            text: "View all  →"
                            flat: true
                            onClicked: root.openPage("Workspaces")
                            contentItem: Text { text: parent.text; color: "#91CDE9"; font.pixelSize: 10; font.weight: Font.DemiBold }
                            background: Rectangle { color: "transparent" }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 13
                        Repeater {
                            model: [
                                { name: "Focus", desc: "Deep work, fewer distractions", glyph: "◈", count: "03 APPS", c: "#8BD6EE" },
                                { name: "Development", desc: "Code, terminal and tools", glyph: "⌘", count: "05 APPS", c: "#AFA0F6" },
                                { name: "Personal", desc: "Life outside the workday", glyph: "⌂", count: "02 APPS", c: "#9FD9B1" }
                            ]
                            delegate: Rectangle {
                                required property var modelData
                                Layout.fillWidth: true
                                Layout.preferredHeight: 126
                                radius: 15
                                color: root.activeWorkspace === modelData.name ? "#182944" : "#111B2D"
                                border.color: root.activeWorkspace === modelData.name ? "#496887" : "#202F46"
                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 15
                                    spacing: 7
                                    RowLayout {
                                        Layout.fillWidth: true
                                        Text { text: modelData.glyph; color: modelData.c; font.pixelSize: 21 }
                                        Item { Layout.fillWidth: true }
                                        Text { text: modelData.count; color: "#687D98"; font.pixelSize: 8; font.weight: Font.Bold; font.letterSpacing: 1 }
                                    }
                                    Text { text: modelData.name; color: "#EAF0F8"; font.pixelSize: 12; font.weight: Font.DemiBold }
                                    Text { text: modelData.desc; color: "#8292A8"; font.pixelSize: 9; elide: Text.ElideRight; Layout.fillWidth: true }
                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: {
                                            root.activeWorkspace = modelData.name
                                            root.notify(modelData.name + " workspace selected (preview)")
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // Applications and system status
                    RowLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignTop
                        spacing: 18

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 13
                            RowLayout {
                                Layout.fillWidth: true
                                Text { text: "Quick launch"; color: "#EFF3FA"; font.pixelSize: 15; font.weight: Font.DemiBold }
                                Item { Layout.fillWidth: true }
                                Text { text: "DEMO APPLICATIONS"; color: "#657792"; font.pixelSize: 8; font.weight: Font.Bold; font.letterSpacing: 1.1 }
                            }
                            GridLayout {
                                Layout.fillWidth: true
                                columns: 2
                                rowSpacing: 10
                                columnSpacing: 10
                                Repeater {
                                    model: root.appItems
                                    delegate: Button {
                                        required property var modelData
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 76
                                        hoverEnabled: true
                                        onClicked: root.notify(modelData.name + " launcher is a UI demo in Alpha 0.1")
                                        background: Rectangle {
                                            radius: 13
                                            color: parent.hovered ? "#192840" : "#111B2C"
                                            border.color: parent.hovered ? "#3C5778" : "#202E45"
                                        }
                                        contentItem: RowLayout {
                                            spacing: 11
                                            Rectangle {
                                                width: 38; height: 38; radius: 12
                                                color: modelData.accent; opacity: 0.15
                                                border.color: modelData.accent; border.width: 1
                                                Text { anchors.centerIn: parent; text: modelData.glyph; color: modelData.accent; font.pixelSize: 18; font.weight: Font.DemiBold }
                                            }
                                            ColumnLayout {
                                                spacing: 4
                                                Text { text: modelData.name; color: "#E8EEF7"; font.pixelSize: 11; font.weight: Font.DemiBold }
                                                Text { text: modelData.detail; color: "#7789A2"; font.pixelSize: 9 }
                                            }
                                            Item { Layout.fillWidth: true }
                                            Text { text: "↗"; color: "#657B96"; font.pixelSize: 13 }
                                        }
                                    }
                                }
                            }
                        }

                        Rectangle {
                            Layout.preferredWidth: 270
                            Layout.preferredHeight: 184
                            radius: 15
                            color: "#111B2C"
                            border.color: "#202F46"
                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 16
                                spacing: 12
                                RowLayout {
                                    Text { text: "System pulse"; color: "#EAF0F8"; font.pixelSize: 12; font.weight: Font.DemiBold }
                                    Item { Layout.fillWidth: true }
                                    Rectangle { width: 6; height: 6; radius: 3; color: "#8BCFA6" }
                                    Text { text: "PREVIEW"; color: "#7F91AA"; font.pixelSize: 8; font.weight: Font.Bold; font.letterSpacing: 1 }
                                }
                                Repeater {
                                    model: [
                                        { label: "Processor", value: "—", note: "Live metrics not connected" },
                                        { label: "Memory", value: "—", note: "Live metrics not connected" },
                                        { label: "Storage", value: "—", note: "Live metrics not connected" }
                                    ]
                                    delegate: ColumnLayout {
                                        required property var modelData
                                        Layout.fillWidth: true
                                        spacing: 3
                                        RowLayout {
                                            Layout.fillWidth: true
                                            Text { text: modelData.label; color: "#AAB8CA"; font.pixelSize: 9 }
                                            Item { Layout.fillWidth: true }
                                            Text { text: modelData.value; color: "#D9E3F0"; font.pixelSize: 9; font.weight: Font.DemiBold }
                                        }
                                        Text { text: modelData.note; color: "#62748D"; font.pixelSize: 8 }
                                    }
                                }
                            }
                        }
                    }

                    Item { Layout.preferredHeight: 6 }
                }
            }

            // Footer / prototype status
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 32
                color: "#0A101D"
                border.color: "#1A263A"
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 20
                    anchors.rightMargin: 20
                    Text { text: "●  ORBIT services: UI preview only"; color: "#6F839D"; font.pixelSize: 8 }
                    Item { Layout.fillWidth: true }
                    Text { text: "ALPHA 0.1.0  |  NOT A SYSTEM SESSION"; color: "#647690"; font.pixelSize: 8; font.weight: Font.Bold; font.letterSpacing: 0.7 }
                }
            }
        }
    }

    // Command palette: local demo actions only, no host commands are executed.
    Popup {
        id: commandPalette
        width: Math.min(560, root.width - 50)
        height: 360
        x: (root.width - width) / 2
        y: 92
        modal: true
        focus: true
        padding: 16
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        background: Rectangle { radius: 17; color: "#121D30"; border.color: "#405675"; border.width: 1 }
        contentItem: ColumnLayout {
            spacing: 12
            Text { text: "ORBIT COMMAND"; color: "#90CFEF"; font.pixelSize: 9; font.weight: Font.Bold; font.letterSpacing: 1.8 }
            TextField {
                id: commandInput
                Layout.fillWidth: true
                placeholderText: "Search apps, files, settings, or actions…"
                color: "#EDF3FB"
                placeholderTextColor: "#72849E"
                focus: true
                background: Rectangle { radius: 10; color: "#0B1424"; border.color: "#344B68" }
                onAccepted: {
                    root.notify(text.length ? "Command received: " + text : "Type a command to search")
                    commandPalette.close()
                    text = ""
                }
            }
            Text { text: "SUGGESTED ACTIONS"; color: "#687D98"; font.pixelSize: 8; font.weight: Font.Bold; font.letterSpacing: 1.3 }
            Repeater {
                model: ["Open Settings", "Switch workspace", "Show applications", "View system information"]
                delegate: Button {
                    required property string modelData
                    Layout.fillWidth: true
                    Layout.preferredHeight: 39
                    onClicked: {
                        root.notify(modelData + " — command preview")
                        commandPalette.close()
                    }
                    background: Rectangle { radius: 8; color: parent.hovered ? "#1D2D46" : "transparent" }
                    contentItem: RowLayout {
                        Text { text: "⌘"; color: "#8CBBD9"; font.pixelSize: 13 }
                        Text { text: modelData; color: "#CFD9E7"; font.pixelSize: 10; Layout.fillWidth: true }
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

    // Settings are deliberately simulated until ORBIT system services exist.
    Popup {
        id: quickPanel
        width: 310
        height: 330
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
                Text { text: "Quick settings"; color: "#F0F4FA"; font.pixelSize: 13; font.weight: Font.DemiBold }
                Item { Layout.fillWidth: true }
                Text { text: "PREVIEW"; color: "#6E85A0"; font.pixelSize: 8; font.weight: Font.Bold }
            }
            GridLayout {
                Layout.fillWidth: true
                columns: 2
                rowSpacing: 9
                columnSpacing: 9
                Repeater {
                    model: ["Wi-Fi", "Bluetooth", "Night light", "Do not disturb"]
                    delegate: Button {
                        required property string modelData
                        property bool enabledState: modelData === "Wi-Fi"
                        Layout.fillWidth: true
                        Layout.preferredHeight: 67
                        onClicked: { enabledState = !enabledState; root.notify(modelData + " toggle is a preview only") }
                        background: Rectangle { radius: 11; color: parent.enabledState ? "#1D3850" : "#182337"; border.color: parent.enabledState ? "#416B8D" : "#293A54" }
                        contentItem: ColumnLayout {
                            Text { text: "●"; color: parent.parent.enabledState ? "#9ADCF4" : "#74869E"; font.pixelSize: 13 }
                            Text { text: modelData; color: "#D9E3F0"; font.pixelSize: 9; font.weight: Font.DemiBold }
                        }
                    }
                }
            }
            Rectangle { Layout.fillWidth: true; height: 1; color: "#263750" }
            Text { text: "Display brightness"; color: "#B5C2D3"; font.pixelSize: 9 }
            Slider { Layout.fillWidth: true; from: 0; to: 100; value: 72 }
            Text { text: "Sound level"; color: "#B5C2D3"; font.pixelSize: 9 }
            Slider { Layout.fillWidth: true; from: 0; to: 100; value: 58 }
            Text { text: "Controls are visual placeholders; hardware integration comes later."; color: "#6F829B"; font.pixelSize: 8; wrapMode: Text.WordWrap; Layout.fillWidth: true }
        }
    }

    // Small transient confirmation surface.
    Rectangle {
        visible: root.toastText.length > 0
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 48
        width: Math.min(500, root.width - 60)
        height: 42
        radius: 12
        color: "#23344D"
        border.color: "#57718F"
        z: 100
        Text { anchors.centerIn: parent; width: parent.width - 24; text: root.toastText; color: "#EDF4FB"; font.pixelSize: 10; horizontalAlignment: Text.AlignHCenter; elide: Text.ElideRight }
    }
}
