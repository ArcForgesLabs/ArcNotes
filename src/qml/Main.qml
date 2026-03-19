pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import nuttyartist.notes 1.0 as Notes

ApplicationWindow {
    id: root
    visible: true
    width: Notes.AppBackend.initialWindowWidth
    height: Notes.AppBackend.initialWindowHeight
    x: Notes.AppBackend.initialWindowX
    y: Notes.AppBackend.initialWindowY
    title: "ArcNotes"
    flags: Notes.AppBackend.alwaysOnTop ? Qt.Window | Qt.WindowStaysOnTopHint : Qt.Window
    color: themeData.backgroundColor
    Material.theme: darkTheme ? Material.Dark : Material.Light
    Material.accent: root.accentColor
    Material.background: themeData.backgroundColor

    property var themeData: Notes.AppBackend.themeData
    property bool darkTheme: themeData.theme === "Dark"
    property color panelBackground: darkTheme ? "#202020" : "#ffffff"
    property color headerBackground: darkTheme ? "#171717" : "#fbfbfa"
    property color edgeColor: darkTheme ? "#2f2f2f" : "#e8e3da"
    property color titleColor: darkTheme ? "#ececec" : "#2f2c26"
    property color mutedColor: darkTheme ? "#8f8f8f" : "#7b7568"
    property color accentColor: darkTheme ? "#5b94f5" : "#2383e2"
    property color selectedColor: darkTheme ? "#253245" : "#e7f0ff"
    property color searchBackground: darkTheme ? "#181818" : "#f5f5f4"
    property color hoverBackground: darkTheme ? "#2a2a2a" : "#f1f1ef"
    property color noteCardBackground: darkTheme ? "#242424" : "#ffffff"
    property bool editorSyncing: false
    property int selectedTreeRow: -1
    property var editorFlickable: editorScroll.contentItem

    function cleanTextLine(line) {
        if (line === undefined || line === null) {
            return ""
        }

        var trimmed = String(line).replace(/\r/g, "").trim()
        trimmed = trimmed.replace(/^#+\s*/, "")
        trimmed = trimmed.replace(/^[-*]\s*\[[xX ]\]\s*/, "")
        return trimmed.trim()
    }

    function notePreviewText(noteContent, noteTitle, parentName) {
        var cleanedTitle = cleanTextLine(noteTitle)
        if (noteContent !== undefined && noteContent !== null) {
            var lines = String(noteContent).split(/\r?\n/)
            var skippedTitle = false

            for (var i = 0; i < lines.length; ++i) {
                var line = cleanTextLine(lines[i])
                if (line.length === 0) {
                    continue
                }

                if (!skippedTitle && cleanedTitle.length > 0 && line === cleanedTitle) {
                    skippedTitle = true
                    continue
                }

                return line
            }
        }

        return parentName ? parentName : qsTr("No additional text")
    }

    function treeIconText(itemType) {
        if (itemType === 1) {
            return fontIconLoader.icons.fa_note_sticky
        }
        if (itemType === 2) {
            return fontIconLoader.icons.fa_trash
        }
        if (itemType === 5) {
            return fontIconLoader.icons.fa_folder
        }
        return ""
    }

    function toggleEditorSettings(anchorItem) {
        if (editorSettingsPopup.opened) {
            editorSettingsPopup.close()
            return
        }

        Notes.AppBackend.setEditorSettingsFromQuickViewVisibility(true)
        Notes.AppBackend.showEditorSettings()

        if (anchorItem) {
            var pos = anchorItem.mapToItem(root.contentItem, 0, anchorItem.height + 8)
            editorSettingsPopup.x = Math.max(12, Math.round(pos.x - editorSettingsPopup.width + anchorItem.width))
            editorSettingsPopup.y = Math.round(pos.y)
        }

        editorSettingsPopup.open()
    }

    Notes.UpdateBackend {
        id: updateBackend
    }

    FontIconLoader {
        id: fontIconLoader
    }

    Component.onCompleted: {
        searchField.text = Notes.AppBackend.searchText
        editorArea.text = Notes.AppBackend.noteEditor.currentText
        Notes.AppBackend.publishState()
        Notes.AppBackend.updateWindowMetrics(width, height, x, y)
        updateBackend.checkForUpdates(false)
    }

    onWidthChanged: Notes.AppBackend.updateWindowMetrics(width, height, x, y)
    onHeightChanged: Notes.AppBackend.updateWindowMetrics(width, height, x, y)
    onXChanged: Notes.AppBackend.updateWindowMetrics(width, height, x, y)
    onYChanged: Notes.AppBackend.updateWindowMetrics(width, height, x, y)

    Shortcut {
        sequence: "F10"
        onActivated: Notes.AppBackend.fireToggleEditorSettingsShortcut()
    }

    Connections {
        target: Notes.AppBackend

        function onErrorOccurred(data) {
            errorDialog.title = data.title
            errorDialogText.text = data.content
            errorDialog.open()
        }

        function onToggleEditorSettingsKeyboardShorcutFired() {
            root.toggleEditorSettings(editorMenuButton)
        }

        function onSearchTextChanged() {
            if (searchField.text !== Notes.AppBackend.searchText) {
                searchField.text = Notes.AppBackend.searchText
            }
        }
    }

    Connections {
        target: Notes.AppBackend.noteEditor

        function onCurrentTextChanged() {
            if (editorArea.text === Notes.AppBackend.noteEditor.currentText) {
                return
            }
            root.editorSyncing = true
            editorArea.text = Notes.AppBackend.noteEditor.currentText
            root.editorSyncing = false
        }

        function onScrollBarPositionChanged() {
            if (root.editorFlickable && Math.abs(root.editorFlickable.contentY - Notes.AppBackend.noteEditor.scrollBarPosition) > 1) {
                root.editorFlickable.contentY = Notes.AppBackend.noteEditor.scrollBarPosition
            }
        }
    }

    Connections {
        target: updateBackend

        function onDialogVisibleChanged() {
            if (updateBackend.dialogVisible) {
                updaterDialog.open()
            } else {
                updaterDialog.close()
            }
        }
    }

    Dialog {
        id: errorDialog
        modal: true
        width: 420
        title: qsTr("ArcNotes")

        contentItem: ColumnLayout {
            spacing: 12
            width: parent.width

            Text {
                id: errorDialogText
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                color: root.titleColor
                font.family: Notes.AppBackend.displayFontFamily
            }
        }
    }

    AboutDialog {
        id: aboutDialog
        x: Math.round((root.width - width) / 2)
        y: Math.round((root.height - height) / 2)
        displayFontFamily: Notes.AppBackend.displayFontFamily
        themeName: root.themeData.theme
        applicationName: Qt.application.name ? Qt.application.name : "ArcNotes"
        applicationVersion: Qt.application.version ? Qt.application.version : "0.0.0"
    }

    UpdaterDialog {
        id: updaterDialog
        x: Math.round((root.width - width) / 2)
        y: Math.round((root.height - height) / 2)
        displayFontFamily: Notes.AppBackend.displayFontFamily
        themeName: root.themeData.theme
        titleText: updateBackend.titleText
        installedVersion: updateBackend.installedVersion
        availableVersion: updateBackend.availableVersion
        changelogHtml: updateBackend.changelogHtml
        showProgressControls: updateBackend.showProgressControls
        progressIndeterminate: updateBackend.progressIndeterminate
        progressValue: updateBackend.progressValue
        downloadLabelText: updateBackend.downloadLabelText
        timeRemainingText: updateBackend.timeRemainingText
        dontNotifyAutomatically: updateBackend.dontNotifyAutomatically
        updateButtonEnabled: updateBackend.updateButtonEnabled

        onUpdateRequested: updateBackend.requestUpdate()
        onCloseRequested: updateBackend.closeDialog()
        onDontNotifyAutomaticallyToggled: function(checked) {
            updateBackend.dontNotifyAutomatically = checked
        }
        onClosed: {
            if (updateBackend.dialogVisible) {
                updateBackend.closeDialog()
            }
        }
    }

    Popup {
        id: editorSettingsPopup
        x: root.width - width - 20
        y: 72
        padding: 0
        modal: false
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent
        background: Rectangle {
            color: "transparent"
        }

        onClosed: Notes.AppBackend.setEditorSettingsFromQuickViewVisibility(false)

        contentItem: EditorSettings {
            extraWidthPadding: 0
            extraHeightPadding: 0
        }
    }

    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal

        handle: Rectangle {
            implicitWidth: 1
            implicitHeight: 1
            color: root.edgeColor
        }

        Rectangle {
            visible: !Notes.AppBackend.folderTreeCollapsed
            SplitView.preferredWidth: 228
            color: root.panelBackground

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 48
                    color: root.panelBackground
                    border.color: root.edgeColor
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 14
                        anchors.rightMargin: 10
                        spacing: 8

                        Image {
                            source: "qrc:/images/notes_system_tray_icon.png"
                            sourceSize.width: 18
                            sourceSize.height: 18
                            Layout.preferredWidth: 18
                            Layout.preferredHeight: 18
                            fillMode: Image.PreserveAspectFit
                        }

                        Text {
                            Layout.fillWidth: true
                            text: qsTr("ArcNotes")
                            color: root.titleColor
                            font.family: Notes.AppBackend.displayFontFamily
                            font.pointSize: 11
                            font.bold: true
                            elide: Text.ElideRight
                        }

                        IconButton {
                            icon: fontIconLoader.icons.fa_bell
                            visible: updateBackend.enabled
                            themeData: root.themeData
                            themeColor: root.titleColor
                            platform: Notes.AppBackend.platformName
                            onClicked: updateBackend.checkForUpdates(true)
                        }

                        IconButton {
                            icon: fontIconLoader.icons.fa_circle_info
                            themeData: root.themeData
                            themeColor: root.titleColor
                            platform: Notes.AppBackend.platformName
                            onClicked: aboutDialog.open()
                        }

                        IconButton {
                            icon: fontIconLoader.icons.fa_gear
                            themeData: root.themeData
                            themeColor: root.titleColor
                            platform: Notes.AppBackend.platformName
                            onClicked: root.toggleEditorSettings(editorMenuButton)
                        }
                    }
                }

                TreeView {
                    id: treeView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: Notes.AppBackend.treeModel
                    columnWidthProvider: function(column) { return width }

                    delegate: Item {
                        id: treeDelegate
                        required property TreeView treeView
                        required property int row
                        required property int depth
                        required property bool hasChildren
                        required property bool expanded
                        required property int column
                        required property var model
                        implicitHeight: (treeDelegate.model.itemType === 3 || treeDelegate.model.itemType === 4) ? 28 : 36
                        width: treeView.width

                        Rectangle {
                            anchors.fill: parent
                            anchors.leftMargin: 6
                            anchors.rightMargin: 6
                            anchors.topMargin: 1
                            anchors.bottomMargin: 1
                            radius: 8
                            color: root.selectedTreeRow === treeDelegate.row ? root.selectedColor : "transparent"

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 10 + treeDelegate.depth * 16
                                anchors.rightMargin: 12
                                spacing: 8

                                Text {
                                    visible: treeDelegate.hasChildren && treeDelegate.model.itemType !== 3 && treeDelegate.model.itemType !== 4
                                    text: treeDelegate.expanded ? fontIconLoader.icons.fa_chevron_down : fontIconLoader.icons.fa_chevron_right
                                    color: root.mutedColor
                                    font.family: fontIconLoader.fa_solid
                                    font.pointSize: 8
                                    verticalAlignment: Text.AlignVCenter

                                    TapHandler {
                                        onTapped: treeDelegate.treeView.toggleExpanded(treeDelegate.row)
                                    }
                                }

                                Item {
                                    visible: !treeDelegate.hasChildren || treeDelegate.model.itemType === 3 || treeDelegate.model.itemType === 4
                                    implicitWidth: 8
                                }

                                Text {
                                    visible: treeIconText(treeDelegate.model.itemType) !== ""
                                    text: treeIconText(treeDelegate.model.itemType)
                                    color: treeDelegate.model.itemType === 2 ? root.mutedColor : root.accentColor
                                    font.family: fontIconLoader.fa_solid
                                    font.pointSize: 10
                                    verticalAlignment: Text.AlignVCenter
                                }

                                Rectangle {
                                    visible: treeDelegate.model.itemType === 7
                                    implicitWidth: 8
                                    implicitHeight: 8
                                    radius: 4
                                    color: treeDelegate.model.tagColor ? treeDelegate.model.tagColor : root.accentColor
                                }

                                Text {
                                    Layout.fillWidth: true
                                    text: treeDelegate.model.displayText ? treeDelegate.model.displayText : ""
                                    color: treeDelegate.model.itemType === 3 || treeDelegate.model.itemType === 4 ? root.mutedColor : root.titleColor
                                    font.family: Notes.AppBackend.displayFontFamily
                                    font.pointSize: treeDelegate.model.itemType === 3 || treeDelegate.model.itemType === 4 ? 10 : 11
                                    font.bold: treeDelegate.model.itemType === 3 || treeDelegate.model.itemType === 4
                                    elide: Text.ElideRight
                                }

                                Text {
                                    visible: treeDelegate.model.childCount !== undefined && treeDelegate.model.childCount > 0
                                    text: treeDelegate.model.childCount !== undefined ? treeDelegate.model.childCount : ""
                                    color: root.mutedColor
                                    font.family: Notes.AppBackend.displayFontFamily
                                    font.pointSize: 10
                                }
                            }
                        }

                        TapHandler {
                            enabled: treeDelegate.model.itemType !== 3 && treeDelegate.model.itemType !== 4
                            onTapped: {
                                root.selectedTreeRow = treeDelegate.row
                                Notes.AppBackend.activateTreeItem(treeDelegate.model.itemType ? treeDelegate.model.itemType : 0,
                                                                  treeDelegate.model.nodeId !== undefined ? treeDelegate.model.nodeId : -1)
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            visible: !Notes.AppBackend.noteListCollapsed
            SplitView.preferredWidth: 292
            color: root.panelBackground

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 48
                        color: root.panelBackground
                        border.color: root.edgeColor
                        border.width: 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 12
                            spacing: 10

                            IconButton {
                                icon: Notes.AppBackend.folderTreeCollapsed ? fontIconLoader.icons.fa_arrow_right_to_line
                                                                           : fontIconLoader.icons.fa_arrow_left_to_line
                                themeData: root.themeData
                                themeColor: Notes.AppBackend.folderTreeCollapsed ? root.mutedColor : root.titleColor
                                platform: Notes.AppBackend.platformName
                                onClicked: Notes.AppBackend.folderTreeCollapsed ? Notes.AppBackend.expandFolderTree()
                                                                                : Notes.AppBackend.collapseFolderTree()
                            }

                            Text {
                                Layout.fillWidth: true
                                text: Notes.AppBackend.listLabel1
                                color: root.titleColor
                                font.family: Notes.AppBackend.displayFontFamily
                                font.pointSize: 11
                                font.bold: true
                                elide: Text.ElideRight
                            }

                            Text {
                                text: Notes.AppBackend.listLabel2
                                color: root.mutedColor
                                font.family: Notes.AppBackend.displayFontFamily
                                font.pointSize: 10
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 44
                        color: root.panelBackground
                        border.color: root.edgeColor
                        border.width: 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 10
                            spacing: 8

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 30
                                radius: 8
                                color: root.searchBackground
                                border.color: root.edgeColor
                                border.width: 1

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.leftMargin: 10
                                    anchors.rightMargin: 8
                                    spacing: 8

                                    Text {
                                        text: fontIconLoader.icons.fa_magnifying_glass
                                        color: root.mutedColor
                                        font.family: fontIconLoader.fa_solid
                                        font.pointSize: 10
                                    }

                                    Item {
                                        Layout.fillWidth: true
                                        Layout.fillHeight: true

                                        TextInput {
                                            id: searchField
                                            anchors.fill: parent
                                            verticalAlignment: TextInput.AlignVCenter
                                            color: root.titleColor
                                            font.family: Notes.AppBackend.displayFontFamily
                                            font.pointSize: 10
                                            selectByMouse: true
                                            clip: true
                                            onTextChanged: Notes.AppBackend.searchText = text
                                        }

                                        Text {
                                            anchors.fill: parent
                                            verticalAlignment: Text.AlignVCenter
                                            text: qsTr("Search")
                                            color: root.mutedColor
                                            font.family: Notes.AppBackend.displayFontFamily
                                            font.pointSize: 10
                                            visible: searchField.text.length === 0
                                        }
                                    }

                                    Text {
                                        visible: searchField.text.length > 0
                                        text: fontIconLoader.icons.fa_xmark
                                        color: root.mutedColor
                                        font.family: fontIconLoader.fa_solid
                                        font.pointSize: 10

                                        TapHandler {
                                            onTapped: {
                                                searchField.text = ""
                                                Notes.AppBackend.clearSearch()
                                            }
                                        }
                                    }
                                }
                            }

                            IconButton {
                                icon: fontIconLoader.icons.fa_plus
                                enabled: Notes.AppBackend.canCreateNotes
                                themeData: root.themeData
                                themeColor: root.accentColor
                                platform: Notes.AppBackend.platformName
                                onClicked: Notes.AppBackend.createNewNote()
                            }
                        }
                }

                ListView {
                        id: noteList
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: 8
                        model: Notes.AppBackend.noteModel
                        topMargin: 10
                        bottomMargin: 10
                        leftMargin: 10
                        rightMargin: 10

                        delegate: Item {
                            id: noteDelegate
                            required property var model
                            required property int index
                            width: noteList.width - 20
                            height: 84

                            Rectangle {
                                anchors.fill: parent
                                radius: 12
                                color: noteDelegate.model.noteId === Notes.AppBackend.noteEditor.currentNoteId ? root.selectedColor
                                                                                                               : root.noteCardBackground
                                border.color: noteDelegate.model.noteId === Notes.AppBackend.noteEditor.currentNoteId ? root.accentColor
                                                                                                                        : root.edgeColor
                                border.width: 1

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: 12
                                    spacing: 4

                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 8

                                        Text {
                                            Layout.fillWidth: true
                                            text: noteDelegate.model.noteFullTitle ? noteDelegate.model.noteFullTitle : qsTr("Untitled")
                                            color: root.titleColor
                                            font.family: Notes.AppBackend.displayFontFamily
                                            font.pointSize: 11
                                            font.bold: true
                                            elide: Text.ElideRight
                                        }

                                        Text {
                                            text: noteDelegate.model.noteLastModificationDateTime ? noteDelegate.model.noteLastModificationDateTime.toLocaleString(Qt.locale(), Locale.ShortFormat) : ""
                                            color: root.mutedColor
                                            font.family: Notes.AppBackend.displayFontFamily
                                            font.pointSize: 9
                                        }
                                    }

                                    Text {
                                        Layout.fillWidth: true
                                        text: root.notePreviewText(noteDelegate.model.noteContent,
                                                                   noteDelegate.model.noteFullTitle,
                                                                   noteDelegate.model.noteParentName)
                                        color: root.mutedColor
                                        font.family: Notes.AppBackend.displayFontFamily
                                        font.pointSize: 10
                                        elide: Text.ElideRight
                                    }

                                    Text {
                                        Layout.fillWidth: true
                                        text: noteDelegate.model.noteParentName ? noteDelegate.model.noteParentName : ""
                                        color: root.mutedColor
                                        font.family: Notes.AppBackend.displayFontFamily
                                        font.pointSize: 9
                                        elide: Text.ElideRight
                                        visible: text.length > 0
                                    }
                                }
                            }

                            TapHandler {
                                onTapped: Notes.AppBackend.selectNoteRow(noteDelegate.index)
                            }
                        }
                }
            }
        }

        Rectangle {
            SplitView.fillWidth: true
            color: root.panelBackground

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 48
                        color: root.panelBackground
                        border.color: root.edgeColor
                        border.width: 1

                        Item {
                            anchors.fill: parent

                            Row {
                                anchors.left: parent.left
                                anchors.leftMargin: 14
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: 6

                                IconButton {
                                    icon: Notes.AppBackend.folderTreeCollapsed ? fontIconLoader.icons.fa_arrow_right_to_line
                                                                               : fontIconLoader.icons.fa_arrow_left_to_line
                                    themeData: root.themeData
                                    themeColor: Notes.AppBackend.folderTreeCollapsed ? root.mutedColor : root.titleColor
                                    platform: Notes.AppBackend.platformName
                                    onClicked: Notes.AppBackend.folderTreeCollapsed ? Notes.AppBackend.expandFolderTree()
                                                                                    : Notes.AppBackend.collapseFolderTree()
                                }

                                IconButton {
                                    icon: Notes.AppBackend.noteListCollapsed ? fontIconLoader.icons.fa_arrow_right_to_line
                                                                             : fontIconLoader.icons.fa_arrow_left_to_line
                                    themeData: root.themeData
                                    themeColor: Notes.AppBackend.noteListCollapsed ? root.mutedColor : root.titleColor
                                    platform: Notes.AppBackend.platformName
                                    onClicked: Notes.AppBackend.noteListCollapsed ? Notes.AppBackend.expandNoteList()
                                                                                  : Notes.AppBackend.collapseNoteList()
                                }

                                IconButton {
                                    icon: fontIconLoader.icons.mt_article
                                    iconFontFamily: fontIconLoader.mt_symbols
                                    themeData: root.themeData
                                    themeColor: Notes.AppBackend.kanbanVisible ? root.mutedColor : root.accentColor
                                    platform: Notes.AppBackend.platformName
                                    onClicked: Notes.AppBackend.setKanbanVisibility(false)
                                }

                                IconButton {
                                    icon: fontIconLoader.icons.mt_view_kanban
                                    iconFontFamily: fontIconLoader.mt_symbols
                                    themeData: root.themeData
                                    themeColor: Notes.AppBackend.kanbanVisible ? root.accentColor : root.mutedColor
                                    platform: Notes.AppBackend.platformName
                                    onClicked: Notes.AppBackend.setKanbanVisibility(true)
                                }
                            }

                            Text {
                                anchors.centerIn: parent
                                text: Notes.AppBackend.noteEditor.currentNoteId === -1 ? qsTr("No note selected")
                                                                                       : Notes.AppBackend.noteEditor.editorDateLabel
                                color: root.mutedColor
                                font.family: Notes.AppBackend.displayFontFamily
                                font.pointSize: 10
                                font.bold: true
                            }

                            Row {
                                anchors.right: parent.right
                                anchors.rightMargin: 14
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: 6

                                IconButton {
                                    id: editorMenuButton
                                    icon: fontIconLoader.icons.fa_ellipsis_h
                                    themeData: root.themeData
                                    themeColor: root.accentColor
                                    platform: Notes.AppBackend.platformName
                                    onClicked: root.toggleEditorSettings(editorMenuButton)
                                }
                            }
                        }
                }

                Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: tagRepeater.count > 0 && !Notes.AppBackend.kanbanVisible ? 36 : 0
                        color: root.headerBackground
                        border.color: root.edgeColor
                        border.width: tagRepeater.count > 0 && !Notes.AppBackend.kanbanVisible ? 1 : 0
                        visible: height > 0

                        Flickable {
                            anchors.fill: parent
                            anchors.leftMargin: 14
                            anchors.rightMargin: 14
                            contentWidth: tagRow.width
                            contentHeight: parent.height
                            flickableDirection: Flickable.HorizontalFlick
                            clip: true

                            Row {
                                id: tagRow
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: 8

                                Repeater {
                                    id: tagRepeater
                                    model: Notes.AppBackend.noteEditor.tagListModel

                                    delegate: Rectangle {
                                        id: tagDelegate
                                        required property var model
                                        radius: 10
                                        color: tagDelegate.model.tagColor ? tagDelegate.model.tagColor : root.accentColor
                                        opacity: 0.16
                                        implicitHeight: 22
                                        implicitWidth: tagLabel.implicitWidth + 16

                                        Text {
                                            id: tagLabel
                                            anchors.centerIn: parent
                                            text: tagDelegate.model.tagName ? tagDelegate.model.tagName : ""
                                            color: root.titleColor
                                            font.family: Notes.AppBackend.displayFontFamily
                                            font.pointSize: 9
                                        }
                                    }
                                }
                            }
                        }
                }

                Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        ScrollView {
                            id: editorScroll
                            anchors.fill: parent
                            visible: !Notes.AppBackend.kanbanVisible
                            clip: true

                            contentWidth: availableWidth

                            Connections {
                                target: root.editorFlickable ? root.editorFlickable : null

                                function onContentYChanged() {
                                    Notes.AppBackend.noteEditor.setScrollBarPosition(Math.round(root.editorFlickable.contentY))
                                }
                            }

                        Item {
                            id: editorCanvas
                            width: editorScroll.availableWidth
                            implicitHeight: editorArea.implicitHeight + 56

                            TextArea {
                                id: editorArea
                                width: Notes.AppBackend.textFullWidth ? editorCanvas.width - 64
                                                                      : Math.min(editorCanvas.width - 64, Notes.AppBackend.textColumnWidth)
                                anchors.horizontalCenter: parent.horizontalCenter
                                anchors.top: parent.top
                                anchors.topMargin: 28
                                wrapMode: TextArea.Wrap
                                selectByMouse: true
                                readOnly: Notes.AppBackend.noteEditor.readOnly
                                persistentSelection: true
                                placeholderText: qsTr("Start writing…")
                                placeholderTextColor: root.mutedColor
                                color: root.titleColor
                                padding: 0
                                font.family: Notes.AppBackend.editorFontFamily
                                font.pointSize: Notes.AppBackend.editorFontPointSize
                                background: Rectangle {
                                        color: "transparent"
                                    }

                                    onTextChanged: {
                                        if (!root.editorSyncing) {
                                            Notes.AppBackend.noteEditor.setCurrentText(text)
                                        }
                                    }
                                }
                            }
                        }

                        KanbanBoard {
                            anchors.fill: parent
                            visible: Notes.AppBackend.kanbanVisible
                        }
                }
            }
        }
    }
}
