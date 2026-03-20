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
    property color panelBackground: darkTheme ? "#191919" : themeData.backgroundColor
    property color headerBackground: darkTheme ? "#191919" : themeData.backgroundColor
    property color edgeColor: darkTheme ? "#444444" : "#bfbfbf"
    property color titleColor: darkTheme ? "#d4d4d4" : "#1a1a1a"
    property color mutedColor: darkTheme ? "#9a9a9a" : "#848484"
    property color accentColor: darkTheme ? "#5b94f5" : "#448ac9"
    property color accentHoverColor: darkTheme ? "#7aa8f7" : "#336ea2"
    property color accentPressedColor: darkTheme ? "#9bbcf8" : "#27557d"
    property color selectedColor: darkTheme ? "#243140" : "#dae9ef"
    property color searchBackground: darkTheme ? "#191919" : themeData.backgroundColor
    property color searchBorderColor: darkTheme ? "#444444" : "#cdcdcd"
    property color hoverBackground: darkTheme ? "#2a2a2a" : "#cfcfcf"
    property color secondaryTextColor: darkTheme ? "#9a9a9a" : "#8e9296"
    property color treeHoverBackground: darkTheme ? "#233445" : "#b4d0e9"
    property color treeSelectedBackground: darkTheme ? "#243140" : "#448ac9"
    property color noteCardBackground: darkTheme ? "#191919" : themeData.backgroundColor
    property bool editorSyncing: false
    property int selectedTreeRow: -1
    property var editorFlickable: editorScroll.contentItem
    property bool pendingPermanentDelete: false

    readonly property string iconGear: "\uf013"
    readonly property string iconPlus: "\uf067"
    readonly property string iconSearch: "\uf002"
    readonly property string iconCloseSmall: "\uf00d"
    readonly property string iconMaterialMore: "\ue5d3"
    readonly property string iconMaterialKanban: "\ueb7f"
    readonly property string iconMaterialArticle: "\uef42"
    readonly property string iconMaterialPaneOpen: "\uec73"
    readonly property string iconMaterialPaneClosed: "\ue31c"

    function isAllNotesContext() {
        return !Notes.AppBackend.currentContextIsTrash && !Notes.AppBackend.currentContextIsTag && Notes.AppBackend.listLabel1 === "All Notes"
    }

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

    function noteDateText(noteDateTime) {
        if (!noteDateTime) {
            return ""
        }

        var currentDate = new Date()
        var noteDate = new Date(noteDateTime)
        if (isNaN(noteDate.getTime())) {
            return ""
        }

        var dayMs = 24 * 60 * 60 * 1000
        var noteDay = new Date(noteDate.getFullYear(), noteDate.getMonth(), noteDate.getDate())
        var currentDay = new Date(currentDate.getFullYear(), currentDate.getMonth(), currentDate.getDate())
        var dayDiff = Math.round((currentDay.getTime() - noteDay.getTime()) / dayMs)

        if (dayDiff === 0) {
            return Qt.formatTime(noteDate, "h:mm AP")
        }
        if (dayDiff === 1) {
            return qsTr("Yesterday")
        }
        if (dayDiff >= 2 && dayDiff <= 7) {
            return Qt.locale("en_US").dayName(noteDate.getDay(), Locale.LongFormat)
        }
        return Qt.formatDate(noteDate, "M/d/yy")
    }

    function editorSidePaddingForWidth(width) {
        if (width < 355) {
            return 15
        }
        if (width < 515) {
            return 40
        }
        if (width < 755) {
            return 50
        }
        if (width < 775) {
            return 60
        }
        if (width < 800) {
            return 70
        }
        return 80
    }

    function treeIconText(itemType) {
        if (itemType === 1) {
            return fontIconLoader.icons.fa_folder
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
            var pos = anchorItem.mapToItem(root.contentItem, 0, anchorItem.height + 4)
            var popupWidth = editorSettingsPopup.width > 0 ? editorSettingsPopup.width : editorSettingsContent.width
            editorSettingsPopup.x = Math.max(12, Math.round(pos.x - popupWidth + anchorItem.width))
            editorSettingsPopup.y = Math.round(pos.y)
        }

        editorSettingsPopup.open()
    }

    function openGlobalMenu(anchorItem) {
        if (!anchorItem) {
            return
        }

        var pos = anchorItem.mapToItem(root.contentItem, 0, anchorItem.height + 4)
        globalMenu.popup(Math.round(pos.x), Math.round(pos.y))
    }

    function openNoteContextMenu(anchorItem, X, Y) {
        var pos = anchorItem.mapToItem(root.contentItem, X, Y)
        noteContextMenu.x = Math.max(8, Math.round(pos.x))
        noteContextMenu.y = Math.max(8, Math.round(pos.y))
        noteContextMenu.open()
    }

    function requestDeleteCurrentNote() {
        if (Notes.AppBackend.currentContextIsTrash) {
            pendingPermanentDelete = true
            deleteDialog.open()
            return
        }

        Notes.AppBackend.moveCurrentNoteToTrash()
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

    Dialog {
        id: deleteDialog
        modal: true
        width: 400
        title: qsTr("Delete Note Permanently")
        standardButtons: Dialog.Yes | Dialog.No

        onAccepted: {
            if (pendingPermanentDelete) {
                Notes.AppBackend.moveCurrentNoteToTrash()
            }
            pendingPermanentDelete = false
        }
        onRejected: pendingPermanentDelete = false

        contentItem: ColumnLayout {
            spacing: 12
            width: parent.width

            Text {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                color: root.titleColor
                font.family: Notes.AppBackend.displayFontFamily
                text: qsTr("Are you sure you want to delete this note permanently? It will not be recoverable.")
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
        parent: Overlay.overlay
        width: editorSettingsContent.width
        height: editorSettingsContent.height
        x: root.width - width - 20
        y: 72
        padding: 0
        modal: false
        focus: true
        z: 1000
        clip: false
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent
        background: Rectangle {
            color: "transparent"
        }

        onClosed: Notes.AppBackend.setEditorSettingsFromQuickViewVisibility(false)

        contentItem: EditorSettings {
            id: editorSettingsContent
            extraWidthPadding: 0
            extraHeightPadding: 0
        }
    }

    Menu {
        id: globalMenu

        MenuItem {
            text: qsTr("Editor Settings")
            onTriggered: root.toggleEditorSettings(globalSettingsButton)
        }

        MenuItem {
            visible: updateBackend.enabled
            text: qsTr("Check For Updates")
            onTriggered: updateBackend.checkForUpdates(true)
        }

        MenuItem {
            text: qsTr("About")
            onTriggered: aboutDialog.open()
        }
    }

    Popup {
        id: noteContextMenu
        parent: Overlay.overlay
        width: 196
        padding: 0
        modal: false
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        background: Rectangle {
            color: root.darkTheme ? "#232323" : "#ffffff"
            border.color: root.darkTheme ? "#3f3f3f" : "#c7c7c7"
            border.width: 1
            radius: 4
        }

        contentItem: Column {
            spacing: 0

            ContextMenuItem {
                visible: Notes.AppBackend.currentContextIsTrash
                text: qsTr("Restore Note")
                onTriggered: {
                    noteContextMenu.close()
                    Notes.AppBackend.restoreCurrentNote()
                }
            }

            ContextMenuItem {
                text: Notes.AppBackend.currentContextIsTrash ? qsTr("Delete Note") : qsTr("Move To Trash")
                onTriggered: {
                    noteContextMenu.close()
                    root.requestDeleteCurrentNote()
                }
            }

            ContextMenuSeparator {
                visible: Notes.AppBackend.currentContextAllowsPinning && (pinNoteMenuItem.visible || unpinNoteMenuItem.visible)
            }

            ContextMenuItem {
                id: pinNoteMenuItem
                visible: Notes.AppBackend.currentContextAllowsPinning && !Notes.AppBackend.currentNotePinned
                text: qsTr("Pin Note")
                onTriggered: {
                    noteContextMenu.close()
                    Notes.AppBackend.setCurrentNotePinned(true)
                }
            }

            ContextMenuItem {
                id: unpinNoteMenuItem
                visible: Notes.AppBackend.currentContextAllowsPinning && Notes.AppBackend.currentNotePinned
                text: qsTr("Unpin Note")
                onTriggered: {
                    noteContextMenu.close()
                    Notes.AppBackend.setCurrentNotePinned(false)
                }
            }

            ContextMenuSeparator {
                visible: !Notes.AppBackend.currentContextIsTrash && Notes.AppBackend.canCreateNotes
            }

            ContextMenuItem {
                visible: !Notes.AppBackend.currentContextIsTrash && Notes.AppBackend.canCreateNotes
                text: qsTr("New Note")
                onTriggered: {
                    noteContextMenu.close()
                    Notes.AppBackend.createNewNote()
                }
            }
        }
    }

    component ToolGlyphButton: Item {
        id: buttonRoot
        property string glyph: ""
        property string fontFamily: fontIconLoader.fa_solid
        property color glyphColor: root.accentColor
        property bool enabled: true
        property bool hoverEnabled: true
        property int pointSize: Notes.AppBackend.platformName === "Apple" ? 18 : 15
        property int buttonWidth: 33
        property int buttonHeight: 25
        property bool hovered: false
        signal clicked

        implicitWidth: buttonWidth
        implicitHeight: buttonHeight
        width: buttonWidth
        height: buttonHeight

        Text {
            anchors.centerIn: parent
            text: buttonRoot.glyph
            font.family: buttonRoot.fontFamily
            font.pointSize: buttonRoot.pointSize
            color: !buttonRoot.enabled ? Qt.rgba(buttonRoot.glyphColor.r, buttonRoot.glyphColor.g, buttonRoot.glyphColor.b, 0.35)
                                      : buttonMouse.pressed ? root.accentPressedColor
                                                            : buttonRoot.hovered ? root.accentHoverColor
                                                                                 : buttonRoot.glyphColor
        }

        MouseArea {
            id: buttonMouse
            anchors.fill: parent
            enabled: buttonRoot.enabled
            hoverEnabled: buttonRoot.hoverEnabled
            cursorShape: Qt.PointingHandCursor
            onEntered: buttonRoot.hovered = true
            onExited: buttonRoot.hovered = false
            onClicked: buttonRoot.clicked()
        }
    }

    component VerticalDivider: Rectangle {
        implicitWidth: 1
        color: root.edgeColor
    }

    component ContextMenuSeparator: Rectangle {
        implicitWidth: 196
        implicitHeight: 1
        color: root.edgeColor
    }

    component ContextMenuItem: Rectangle {
        id: contextMenuItem
        property string text: ""
        property bool enabled: true
        property bool hovered: false
        signal triggered

        implicitWidth: 196
        implicitHeight: 30
        color: contextMenuItem.hovered && contextMenuItem.enabled ? (root.darkTheme ? "#253445" : "#d8eaf5") : "transparent"

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 12
            anchors.right: parent.right
            anchors.rightMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            text: contextMenuItem.text
            color: contextMenuItem.enabled ? root.titleColor : root.mutedColor
            font.family: Notes.AppBackend.displayFontFamily
            font.pointSize: 10
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }

        HoverHandler {
            onHoveredChanged: contextMenuItem.hovered = hovered
        }

        TapHandler {
            acceptedButtons: Qt.LeftButton
            enabled: contextMenuItem.enabled
            onTapped: contextMenuItem.triggered()
        }
    }

    component NoteSectionHeader: Item {
        id: sectionRoot
        property string text: ""
        property bool collapsible: false
        property bool collapsed: false

        implicitHeight: 25
        implicitWidth: parent ? parent.width : 0

        Text {
            anchors.left: parent.left
            anchors.leftMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            text: sectionRoot.text
            color: root.secondaryTextColor
            font.family: Notes.AppBackend.displayFontFamily
            font.pointSize: 10
            font.bold: true
        }

        Text {
            visible: sectionRoot.collapsible
            anchors.right: parent.right
            anchors.rightMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            text: sectionRoot.collapsed ? fontIconLoader.icons.fa_chevron_right : fontIconLoader.icons.fa_chevron_down
            font.family: fontIconLoader.fa_solid
            font.pointSize: 8
            color: root.accentColor
        }
    }

    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal

        handle: Rectangle {
            implicitWidth: 0
            implicitHeight: 1
            color: "transparent"
        }

        Rectangle {
            visible: !Notes.AppBackend.folderTreeCollapsed
            color: root.panelBackground
            SplitView.minimumWidth: 185
            SplitView.preferredWidth: 185

            RowLayout {
                anchors.fill: parent
                spacing: 0

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 0

                    Item {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 3
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 25
                        spacing: 0

                        Item {
                            Layout.fillWidth: true
                        }

                        ToolGlyphButton {
                            id: globalSettingsButton
                            glyph: root.iconGear
                            fontFamily: fontIconLoader.fa_solid
                            glyphColor: root.accentColor
                            onClicked: root.openGlobalMenu(globalSettingsButton)
                        }

                        Item {
                            Layout.preferredWidth: 3
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 2
                    }

                    TreeView {
                        id: treeView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        model: Notes.AppBackend.treeModel
                        columnWidthProvider: function() { return width }

                        delegate: Item {
                            id: treeDelegate
                            required property TreeView treeView
                            required property int row
                            required property int depth
                            required property bool hasChildren
                            required property bool expanded
                            required property var model
                            readonly property bool isSection: treeDelegate.model.itemType === 3 || treeDelegate.model.itemType === 4
                            readonly property bool isFolderItem: treeDelegate.model.itemType === 5
                            readonly property bool isTagItem: treeDelegate.model.itemType === 7
                            readonly property bool isTopButton: treeDelegate.model.itemType === 1 || treeDelegate.model.itemType === 2
                            property bool hovered: false
                            width: treeView.width
                            implicitHeight: 25

                            Rectangle {
                                anchors.fill: parent
                                color: treeDelegate.isSection ? "transparent"
                                                              : root.selectedTreeRow === treeDelegate.row ? root.treeSelectedBackground
                                                                                                          : treeDelegate.hovered ? root.treeHoverBackground
                                                                                                                                : "transparent"
                            }

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: treeDelegate.isSection ? 5
                                                                         : treeDelegate.isFolderItem ? 10
                                                                                                     : (treeDelegate.isTopButton || treeDelegate.isTagItem) ? 22
                                                                                                                                                               : 15 + treeDelegate.depth * 16
                                anchors.rightMargin: 8
                                spacing: 5

                                Text {
                                    visible: treeDelegate.hasChildren && treeDelegate.isFolderItem
                                    text: treeDelegate.expanded ? fontIconLoader.icons.fa_chevron_down : fontIconLoader.icons.fa_chevron_right
                                    font.family: fontIconLoader.fa_solid
                                    font.pointSize: 7
                                    color: root.selectedTreeRow === treeDelegate.row ? "#ffffff" : root.mutedColor

                                    TapHandler {
                                        onTapped: treeDelegate.treeView.toggleExpanded(treeDelegate.row)
                                    }
                                }

                                Item {
                                    visible: !treeDelegate.hasChildren || !treeDelegate.isFolderItem
                                    implicitWidth: treeDelegate.isSection ? 0 : 8
                                }

                                Image {
                                    visible: treeDelegate.isFolderItem
                                    source: "qrc:/images/folder.png"
                                    sourceSize.width: 16
                                    sourceSize.height: 16
                                    fillMode: Image.PreserveAspectFit
                                    Layout.preferredWidth: 16
                                    Layout.preferredHeight: 16
                                }

                                Text {
                                    visible: !treeDelegate.isFolderItem && !treeDelegate.isSection && root.treeIconText(treeDelegate.model.itemType) !== ""
                                    text: root.treeIconText(treeDelegate.model.itemType)
                                    font.family: fontIconLoader.fa_solid
                                    font.pointSize: treeDelegate.isTopButton ? 11 : 10
                                    color: root.selectedTreeRow === treeDelegate.row ? "#ffffff"
                                                                                     : treeDelegate.model.itemType === 2 ? root.mutedColor : root.accentColor
                                }

                                Rectangle {
                                    visible: treeDelegate.isTagItem
                                    implicitWidth: 9
                                    implicitHeight: 9
                                    radius: 4.5
                                    color: treeDelegate.model.tagColor ? treeDelegate.model.tagColor : root.accentColor
                                    opacity: root.selectedTreeRow === treeDelegate.row ? 0.9 : 1.0
                                }

                                Text {
                                    Layout.fillWidth: true
                                    text: treeDelegate.model.displayText ? treeDelegate.model.displayText : ""
                                    color: treeDelegate.isSection ? root.mutedColor
                                                                  : root.selectedTreeRow === treeDelegate.row ? "#ffffff" : root.titleColor
                                    font.family: Notes.AppBackend.displayFontFamily
                                    font.pointSize: treeDelegate.isSection ? 9 : 10
                                    font.bold: treeDelegate.isSection || treeDelegate.isFolderItem || treeDelegate.isTopButton
                                    elide: Text.ElideRight
                                }

                                Text {
                                    visible: !treeDelegate.isSection && treeDelegate.model.childCount !== undefined
                                    Layout.preferredWidth: 22
                                    text: treeDelegate.model.childCount !== undefined ? treeDelegate.model.childCount : ""
                                    color: root.selectedTreeRow === treeDelegate.row ? "#ffffff" : root.mutedColor
                                    font.family: Notes.AppBackend.displayFontFamily
                                    font.pointSize: 9
                                    font.bold: true
                                    horizontalAlignment: Text.AlignRight
                                    verticalAlignment: Text.AlignVCenter
                                }

                                Text {
                                    visible: treeDelegate.isSection
                                    Layout.preferredWidth: 18
                                    text: root.iconPlus
                                    font.family: fontIconLoader.fa_solid
                                    font.pointSize: 10
                                    color: root.accentColor
                                    horizontalAlignment: Text.AlignRight
                                    verticalAlignment: Text.AlignVCenter

                                    TapHandler {
                                        onTapped: {
                                            if (treeDelegate.model.itemType === 3) {
                                                Notes.AppBackend.addNewFolder()
                                            } else if (treeDelegate.model.itemType === 4) {
                                                Notes.AppBackend.addNewTag()
                                            }
                                        }
                                    }
                                }
                            }

                            TapHandler {
                                enabled: !treeDelegate.isSection
                                onTapped: {
                                    root.selectedTreeRow = treeDelegate.row
                                    Notes.AppBackend.activateTreeItem(treeDelegate.model.itemType ? treeDelegate.model.itemType : 0,
                                                                      treeDelegate.model.nodeId !== undefined ? treeDelegate.model.nodeId : -1)
                                }
                            }

                            HoverHandler {
                                enabled: !treeDelegate.isSection && treeDelegate.model.itemType !== 2
                                onHoveredChanged: treeDelegate.hovered = hovered
                            }
                        }
                    }
                }

                VerticalDivider {
                    Layout.fillHeight: true
                }
            }
        }

        Rectangle {
            visible: !Notes.AppBackend.noteListCollapsed
            color: root.panelBackground
            SplitView.minimumWidth: 185
            SplitView.preferredWidth: 185

            RowLayout {
                anchors.fill: parent
                spacing: 0

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 0

                    Item {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 0
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 35
                        spacing: 0

                        Item {
                            Layout.preferredWidth: 2
                        }

                        ToolGlyphButton {
                            glyph: Notes.AppBackend.folderTreeCollapsed ? root.iconMaterialPaneClosed : root.iconMaterialPaneOpen
                            fontFamily: fontIconLoader.mt_symbols
                            buttonWidth: 33
                            buttonHeight: 35
                            pointSize: Notes.AppBackend.platformName === "Apple" ? 21 : 18
                            onClicked: Notes.AppBackend.folderTreeCollapsed ? Notes.AppBackend.expandFolderTree()
                                                                            : Notes.AppBackend.collapseFolderTree()
                        }

                        Item {
                            Layout.preferredWidth: 10
                        }

                        Text {
                            Layout.fillWidth: true
                            text: Notes.AppBackend.listLabel1
                            color: root.titleColor
                            font.family: Notes.AppBackend.displayFontFamily
                            font.pointSize: Notes.AppBackend.platformName === "Apple" ? 13 : 10
                            font.bold: true
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight
                        }

                        Item {
                            Layout.preferredWidth: 10
                        }

                        Text {
                            Layout.preferredWidth: 40
                            Layout.alignment: Qt.AlignVCenter
                            text: Notes.AppBackend.listLabel2
                            color: root.mutedColor
                            font.family: Notes.AppBackend.displayFontFamily
                            font.pointSize: Notes.AppBackend.platformName === "Apple" ? 13 : 10
                            horizontalAlignment: Text.AlignRight
                        }

                        Item {
                            Layout.preferredWidth: 12
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 2
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 25
                        spacing: 0

                        Item {
                            Layout.preferredWidth: 10
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 25
                            radius: 3
                            color: root.searchBackground
                            border.width: searchField.activeFocus ? 2 : 1
                            border.color: searchField.activeFocus ? "#3d9bda" : root.searchBorderColor

                            Text {
                                anchors.left: parent.left
                                anchors.leftMargin: 6
                                anchors.verticalCenter: parent.verticalCenter
                                text: root.iconSearch
                                font.family: fontIconLoader.fa_solid
                                font.pointSize: 9
                                color: root.mutedColor
                            }

                            TextInput {
                                id: searchField
                                anchors.fill: parent
                                anchors.leftMargin: 21
                                anchors.rightMargin: 19
                                verticalAlignment: TextInput.AlignVCenter
                                clip: true
                                selectByMouse: true
                                color: root.titleColor
                                font.family: Notes.AppBackend.displayFontFamily
                                font.pointSize: Notes.AppBackend.platformName === "Apple" ? 12 : 10
                                onTextChanged: Notes.AppBackend.searchText = text
                            }

                            Text {
                                anchors.left: parent.left
                                anchors.leftMargin: 21
                                anchors.verticalCenter: parent.verticalCenter
                                visible: searchField.text.length === 0
                                text: qsTr("Search")
                                color: root.mutedColor
                                font.family: Notes.AppBackend.displayFontFamily
                                font.pointSize: Notes.AppBackend.platformName === "Apple" ? 12 : 10
                            }

                            Text {
                                anchors.right: parent.right
                                anchors.rightMargin: 7
                                anchors.verticalCenter: parent.verticalCenter
                                visible: searchField.text.length > 0
                                text: root.iconCloseSmall
                                font.family: fontIconLoader.fa_solid
                                font.pointSize: 9
                                color: root.mutedColor

                                TapHandler {
                                    onTapped: {
                                        searchField.text = ""
                                        Notes.AppBackend.clearSearch()
                                    }
                                }
                            }
                        }

                        Item {
                            Layout.preferredWidth: 10
                        }

                        ToolGlyphButton {
                            glyph: root.iconPlus
                            fontFamily: fontIconLoader.fa_solid
                            buttonWidth: 33
                            buttonHeight: 25
                            pointSize: Notes.AppBackend.platformName === "Apple" ? 18 : 13
                            enabled: Notes.AppBackend.canCreateNotes
                            onClicked: Notes.AppBackend.createNewNote()
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 9
                    }

                    ListView {
                        id: noteList
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: 0
                        model: Notes.AppBackend.noteModel

                        delegate: Item {
                            id: noteDelegate
                            required property var model
                            required property int index
                            property bool hovered: false
                            property bool selected: noteDelegate.model.noteId === Notes.AppBackend.noteEditor.currentNoteId
                            property bool showPinnedHeader: Notes.AppBackend.currentContextAllowsPinning && Notes.AppBackend.noteRowStartsPinnedSection(noteDelegate.index)
                            property bool showNotesHeader: Notes.AppBackend.currentContextAllowsPinning && Notes.AppBackend.noteListHasPinnedNotes() && Notes.AppBackend.noteRowStartsNotesSection(noteDelegate.index)
                            property bool showFolderName: root.isAllNotesContext() && noteDelegate.model.noteParentName
                            property int rowGap: noteDelegate.index > 0 ? 4 : 0
                            property int cardHeight: showFolderName ? 90 : 70
                            width: noteList.width
                            height: rowGap + cardHeight + (showPinnedHeader ? 25 : 0) + (showNotesHeader ? 35 : 0)

                            NoteSectionHeader {
                                id: pinnedHeader
                                visible: noteDelegate.showPinnedHeader
                                y: noteDelegate.rowGap
                                width: parent.width
                                text: qsTr("Pinned")
                                collapsible: true
                                collapsed: false
                            }

                            NoteSectionHeader {
                                id: notesHeader
                                visible: noteDelegate.showNotesHeader
                                y: noteDelegate.rowGap + (noteDelegate.showPinnedHeader ? 25 : 10)
                                width: parent.width
                                text: qsTr("Notes")
                            }

                            Rectangle {
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.top: parent.top
                                anchors.topMargin: noteDelegate.rowGap + (noteDelegate.showPinnedHeader ? 25 : 0) + (noteDelegate.showNotesHeader ? 25 : 0)
                                height: noteDelegate.cardHeight
                                color: noteDelegate.selected ? root.selectedColor : noteDelegate.hovered ? root.hoverBackground : root.noteCardBackground
                            }

                            Column {
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.top: parent.top
                                anchors.topMargin: noteDelegate.rowGap + (noteDelegate.showPinnedHeader ? 25 : 0) + (noteDelegate.showNotesHeader ? 25 : 0) + 10
                                anchors.bottom: parent.bottom
                                anchors.leftMargin: 20
                                anchors.rightMargin: 20
                                spacing: 0

                                RowLayout {
                                    width: parent.width
                                    spacing: 4

                                    Text {
                                        Layout.fillWidth: true
                                        text: noteDelegate.model.noteFullTitle ? noteDelegate.model.noteFullTitle : qsTr("Untitled")
                                        color: root.titleColor
                                        font.family: Notes.AppBackend.displayFontFamily
                                        font.pointSize: Notes.AppBackend.platformName === "Apple" ? 12 : 10
                                        font.bold: !noteDelegate.selected
                                        elide: Text.ElideRight
                                    }

                                    Text {
                                        visible: noteDelegate.model.noteIsPinned
                                        text: fontIconLoader.icons.fa_thumbtack
                                        font.family: fontIconLoader.fa_solid
                                        font.pointSize: 9
                                        color: root.accentColor
                                    }
                                }

                                Text {
                                    width: parent.width
                                    text: root.noteDateText(noteDelegate.model.noteLastModificationDateTime)
                                    color: root.titleColor
                                    font.family: Notes.AppBackend.displayFontFamily
                                    font.pointSize: Notes.AppBackend.platformName === "Apple" ? 11 : 9
                                    elide: Text.ElideRight
                                    topPadding: 2
                                }

                                Text {
                                    width: parent.width
                                    text: root.notePreviewText(noteDelegate.model.noteContent,
                                                               noteDelegate.model.noteFullTitle,
                                                               noteDelegate.model.noteParentName)
                                    color: root.secondaryTextColor
                                    font.family: Notes.AppBackend.displayFontFamily
                                    font.pointSize: Notes.AppBackend.platformName === "Apple" ? 11 : 9
                                    elide: Text.ElideRight
                                    topPadding: 5
                                }

                                RowLayout {
                                    visible: noteDelegate.showFolderName
                                    width: parent.width
                                    spacing: 4

                                    Image {
                                        source: "qrc:/images/folder.png"
                                        sourceSize.width: 16
                                        sourceSize.height: 16
                                        fillMode: Image.PreserveAspectFit
                                        Layout.preferredWidth: 16
                                        Layout.preferredHeight: 16
                                    }

                                    Text {
                                        Layout.fillWidth: true
                                        text: noteDelegate.model.noteParentName ? noteDelegate.model.noteParentName : ""
                                        color: root.secondaryTextColor
                                        font.family: Notes.AppBackend.displayFontFamily
                                        font.pointSize: Notes.AppBackend.platformName === "Apple" ? 11 : 9
                                        elide: Text.ElideRight
                                    }
                                }
                            }

                            Rectangle {
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.bottom: parent.bottom
                                anchors.leftMargin: 20
                                anchors.rightMargin: 20
                                height: 1
                                color: root.edgeColor
                                visible: noteDelegate.index < noteList.count - 1
                            }

                            TapHandler {
                                acceptedButtons: Qt.LeftButton
                                onTapped: Notes.AppBackend.selectNoteRow(noteDelegate.index)
                            }

                            TapHandler {
                                acceptedButtons: Qt.RightButton
                                onTapped: function(eventPoint) {
                                    Notes.AppBackend.selectNoteRow(noteDelegate.index)
                                    var pos = eventPoint.position
                                    root.openNoteContextMenu(noteDelegate, pos.x, pos.y)
                                }
                            }

                            HoverHandler {
                                onHoveredChanged: noteDelegate.hovered = hovered
                            }
                        }
                    }
                }

                VerticalDivider {
                    Layout.fillHeight: true
                }
            }
        }

        Rectangle {
            color: root.panelBackground
            SplitView.fillWidth: true

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 5
                }

                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 25

                    RowLayout {
                        anchors.fill: parent
                        spacing: 0

                        Item {
                            Layout.preferredWidth: 24
                        }

                        ToolGlyphButton {
                            buttonWidth: 33
                            buttonHeight: 25
                            glyph: root.iconMaterialArticle
                            fontFamily: fontIconLoader.mt_symbols
                            pointSize: Notes.AppBackend.platformName === "Apple" ? 20 : 17
                            glyphColor: Notes.AppBackend.kanbanVisible ? root.mutedColor : root.accentColor
                            enabled: true
                            hoverEnabled: Notes.AppBackend.kanbanVisible
                            onClicked: {
                                if (Notes.AppBackend.kanbanVisible) {
                                    Notes.AppBackend.setKanbanVisibility(false)
                                }
                            }
                        }

                        Item {
                            Layout.preferredWidth: 15
                        }

                        ToolGlyphButton {
                            buttonWidth: 33
                            buttonHeight: 25
                            glyph: root.iconMaterialKanban
                            fontFamily: fontIconLoader.mt_symbols
                            pointSize: Notes.AppBackend.platformName === "Apple" ? 20 : 17
                            glyphColor: Notes.AppBackend.kanbanVisible ? root.accentColor : root.mutedColor
                            enabled: !Notes.AppBackend.currentContextIsTrash
                            hoverEnabled: !Notes.AppBackend.currentContextIsTrash && !Notes.AppBackend.kanbanVisible
                            onClicked: {
                                if (!Notes.AppBackend.kanbanVisible) {
                                    Notes.AppBackend.setKanbanVisibility(true)
                                }
                            }
                        }

                        Item {
                            Layout.preferredWidth: 40
                        }

                        Item {
                            Layout.fillWidth: true

                            Text {
                                anchors.centerIn: parent
                                text: Notes.AppBackend.noteEditor.currentNoteId === -1 ? "" : Notes.AppBackend.noteEditor.editorDateLabel
                                color: root.mutedColor
                                font.family: Notes.AppBackend.displayFontFamily
                                font.pointSize: Notes.AppBackend.platformName === "Apple" ? 12 : 9
                                font.bold: true
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }

                        Item {
                            Layout.preferredWidth: 40
                        }

                        ToolGlyphButton {
                            id: editorMenuButton
                            glyph: root.iconMaterialMore
                            fontFamily: fontIconLoader.mt_symbols
                            pointSize: Notes.AppBackend.platformName === "Apple" ? 23 : 20
                            onClicked: root.toggleEditorSettings(editorMenuButton)
                        }

                        Item {
                            Layout.preferredWidth: 24
                        }
                    }
                }

                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 6
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
                            implicitHeight: Math.max(editorArea.implicitHeight + 62, editorScroll.availableHeight)
                            readonly property int editorSidePadding: root.editorSidePaddingForWidth(width)

                            TextArea {
                                id: editorArea
                                width: Notes.AppBackend.textFullWidth ? Math.max(0, editorCanvas.width - editorCanvas.editorSidePadding * 2)
                                                                      : Math.min(Math.max(0, editorCanvas.width - editorCanvas.editorSidePadding * 2),
                                                                                 Notes.AppBackend.textColumnWidth)
                                anchors.horizontalCenter: parent.horizontalCenter
                                anchors.top: parent.top
                                anchors.topMargin: 6
                                wrapMode: TextArea.Wrap
                                selectByMouse: true
                                persistentSelection: true
                                readOnly: Notes.AppBackend.noteEditor.readOnly
                                placeholderText: qsTr("Start writing...")
                                placeholderTextColor: root.mutedColor
                                color: root.titleColor
                                topPadding: 10
                                bottomPadding: 2
                                leftPadding: 0
                                rightPadding: 0
                                font.family: Notes.AppBackend.editorFontFamily
                                font.pointSize: Notes.AppBackend.editorFontPointSize
                                textFormat: TextEdit.PlainText
                                background: Rectangle {
                                    color: "transparent"
                                }

                                Component.onCompleted: Notes.AppBackend.noteEditor.attachTextDocument(textDocument)

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

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: tagFlow.children.length > 0 && !Notes.AppBackend.kanbanVisible ? Math.min(80, Math.max(33, tagFlow.implicitHeight + 10)) : 0
                    color: root.headerBackground
                    Layout.maximumHeight: 80
                    visible: height > 0

                    Flickable {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 10
                        contentWidth: width
                        contentHeight: tagFlow.implicitHeight
                        clip: true
                        flickableDirection: Flickable.VerticalFlick

                        Flow {
                            id: tagFlow
                            width: parent.width
                            spacing: 5

                            Repeater {
                                id: tagRepeater
                                model: Notes.AppBackend.noteEditor.tagListModel

                                delegate: Rectangle {
                                    id: tagDelegate
                                    required property var model
                                    radius: 10
                                    color: root.darkTheme ? "#4c5561" : "#daebf8"
                                    implicitHeight: 20
                                    implicitWidth: 5 + 12 + 5 + tagLabel.implicitWidth + 7

                                    Rectangle {
                                        width: 12
                                        height: 12
                                        radius: 6
                                        anchors.left: parent.left
                                        anchors.leftMargin: 5
                                        anchors.verticalCenter: parent.verticalCenter
                                        color: tagDelegate.model.tagColor ? tagDelegate.model.tagColor : root.accentColor
                                    }

                                    Text {
                                        id: tagLabel
                                        anchors.left: parent.left
                                        anchors.leftMargin: 22
                                        anchors.right: parent.right
                                        anchors.rightMargin: 7
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: tagDelegate.model.tagName ? tagDelegate.model.tagName : ""
                                        color: root.titleColor
                                        font.family: Notes.AppBackend.displayFontFamily
                                        font.pointSize: 9
                                        verticalAlignment: Text.AlignVCenter
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
