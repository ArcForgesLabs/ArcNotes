pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
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

    property var themeData: Notes.AppBackend.themeData
    property bool darkTheme: themeData.theme === "Dark"
    property color panelBackground: darkTheme ? "#202020" : "#ffffff"
    property color headerBackground: darkTheme ? "#171717" : "#faf7f2"
    property color edgeColor: darkTheme ? "#2f2f2f" : "#e7e2d7"
    property color titleColor: darkTheme ? "#ececec" : "#2f2c26"
    property color mutedColor: darkTheme ? "#8f8f8f" : "#7b7568"
    property color accentColor: darkTheme ? "#5b94f5" : "#2383e2"
    property color selectedColor: darkTheme ? "#253245" : "#e7f0ff"
    property bool editorSyncing: false
    property var editorFlickable: editorScroll.contentItem

    Component.onCompleted: {
        searchField.text = Notes.AppBackend.searchText
        editorArea.text = Notes.AppBackend.noteEditor.currentText
        Notes.AppBackend.publishState()
        Notes.AppBackend.updateWindowMetrics(width, height, x, y)
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
            if (editorSettingsPopup.opened) {
                editorSettingsPopup.close()
            } else {
                Notes.AppBackend.setEditorSettingsFromQuickViewVisibility(true)
                Notes.AppBackend.showEditorSettings()
                editorSettingsPopup.open()
            }
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

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 64
            color: root.headerBackground
            border.color: root.edgeColor
            border.width: 1

            RowLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 10

                Button {
                    text: "+"
                    enabled: Notes.AppBackend.canCreateNotes
                    onClicked: Notes.AppBackend.createNewNote()
                }

                Button {
                    text: Notes.AppBackend.folderTreeCollapsed ? qsTr("Folders") : qsTr("Hide Folders")
                    onClicked: Notes.AppBackend.folderTreeCollapsed ? Notes.AppBackend.expandFolderTree()
                                                                    : Notes.AppBackend.collapseFolderTree()
                }

                Button {
                    text: Notes.AppBackend.noteListCollapsed ? qsTr("Notes") : qsTr("Hide Notes")
                    onClicked: Notes.AppBackend.noteListCollapsed ? Notes.AppBackend.expandNoteList()
                                                                  : Notes.AppBackend.collapseNoteList()
                }

                Button {
                    text: Notes.AppBackend.kanbanVisible ? qsTr("Text") : qsTr("Board")
                    onClicked: Notes.AppBackend.setKanbanVisibility(!Notes.AppBackend.kanbanVisible)
                }

                TextField {
                    id: searchField
                    Layout.fillWidth: true
                    placeholderText: qsTr("Search notes")
                    font.family: Notes.AppBackend.displayFontFamily
                    onTextEdited: Notes.AppBackend.searchText = text
                }

                Button {
                    text: qsTr("Clear")
                    visible: searchField.text.length > 0
                    onClicked: {
                        searchField.text = ""
                        Notes.AppBackend.clearSearch()
                    }
                }

                Button {
                    text: qsTr("Settings")
                    onClicked: {
                        if (editorSettingsPopup.opened) {
                            editorSettingsPopup.close()
                        } else {
                            Notes.AppBackend.setEditorSettingsFromQuickViewVisibility(true)
                            Notes.AppBackend.showEditorSettings()
                            editorSettingsPopup.open()
                        }
                    }
                }

                Button {
                    text: qsTr("About")
                    onClicked: aboutDialog.open()
                }
            }
        }

        SplitView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Horizontal

            Pane {
                visible: !Notes.AppBackend.folderTreeCollapsed
                SplitView.preferredWidth: 270
                padding: 0
                background: Rectangle {
                    color: root.panelBackground
                    border.color: root.edgeColor
                    border.width: 1
                }

                TreeView {
                    id: treeView
                    anchors.fill: parent
                    clip: true
                    model: Notes.AppBackend.treeModel
                    property int currentRow: -1
                    columnWidthProvider: function(column) { return width }

                    delegate: Rectangle {
                        id: treeDelegate
                        required property TreeView treeView
                        required property int row
                        required property int depth
                        required property bool hasChildren
                        required property bool expanded
                        required property int column
                        required property var model
                        implicitHeight: (treeDelegate.model.itemType === 3 || treeDelegate.model.itemType === 4) ? 30 : 38
                        color: treeDelegate.treeView.currentRow === treeDelegate.row ? root.selectedColor : "transparent"

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 10 + treeDelegate.depth * 18
                            anchors.rightMargin: 10
                            spacing: 8

                            ToolButton {
                                visible: treeDelegate.hasChildren && treeDelegate.model.itemType !== 3 && treeDelegate.model.itemType !== 4
                                text: treeDelegate.expanded ? "▾" : "▸"
                                onClicked: treeDelegate.treeView.toggleExpanded(treeDelegate.row)
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
                                font.bold: treeDelegate.model.itemType === 3 || treeDelegate.model.itemType === 4
                                elide: Text.ElideRight
                            }

                            Text {
                                visible: treeDelegate.model.childCount !== undefined && treeDelegate.model.childCount > 0
                                text: treeDelegate.model.childCount
                                color: root.mutedColor
                                font.family: Notes.AppBackend.displayFontFamily
                            }
                        }

                        TapHandler {
                            enabled: treeDelegate.model.itemType !== 3 && treeDelegate.model.itemType !== 4
                            onTapped: {
                                treeDelegate.treeView.currentRow = treeDelegate.row
                                Notes.AppBackend.activateTreeItem(treeDelegate.model.itemType ? treeDelegate.model.itemType : 0,
                                                                  treeDelegate.model.nodeId !== undefined ? treeDelegate.model.nodeId : -1)
                            }
                        }
                    }
                }
            }

            Pane {
                visible: !Notes.AppBackend.noteListCollapsed
                SplitView.preferredWidth: 340
                padding: 0
                background: Rectangle {
                    color: root.panelBackground
                    border.color: root.edgeColor
                    border.width: 1
                }

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 54
                        color: root.headerBackground
                        border.color: root.edgeColor
                        border.width: 1

                        Column {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.leftMargin: 14
                            spacing: 2

                            Text {
                                text: Notes.AppBackend.listLabel1
                                color: root.titleColor
                                font.family: Notes.AppBackend.displayFontFamily
                                font.bold: true
                            }

                            Text {
                                text: Notes.AppBackend.listLabel2 + qsTr(" notes")
                                color: root.mutedColor
                                font.family: Notes.AppBackend.displayFontFamily
                            }
                        }
                    }

                    ListView {
                        id: noteList
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: 2
                        model: Notes.AppBackend.noteModel

                        delegate: Rectangle {
                            id: noteDelegate
                            required property var model
                            required property int index
                            width: noteList.width
                            height: 78
                            radius: 10
                            color: noteDelegate.model.noteId === Notes.AppBackend.noteEditor.currentNoteId ? root.selectedColor : "transparent"
                            border.color: noteDelegate.model.noteId === Notes.AppBackend.noteEditor.currentNoteId ? root.accentColor : "transparent"
                            border.width: noteDelegate.model.noteId === Notes.AppBackend.noteEditor.currentNoteId ? 1 : 0

                            Column {
                                anchors.fill: parent
                                anchors.margins: 12
                                spacing: 4

                                Text {
                                    width: parent.width
                                    text: noteDelegate.model.noteFullTitle ? noteDelegate.model.noteFullTitle : qsTr("Untitled")
                                    color: root.titleColor
                                    font.family: Notes.AppBackend.displayFontFamily
                                    font.bold: true
                                    elide: Text.ElideRight
                                }

                                Text {
                                    width: parent.width
                                    text: noteDelegate.model.noteParentName ? noteDelegate.model.noteParentName : ""
                                    color: root.mutedColor
                                    font.family: Notes.AppBackend.displayFontFamily
                                    elide: Text.ElideRight
                                }

                                Text {
                                    width: parent.width
                                    text: noteDelegate.model.noteLastModificationDateTime ? noteDelegate.model.noteLastModificationDateTime.toLocaleString() : ""
                                    color: root.mutedColor
                                    font.family: Notes.AppBackend.displayFontFamily
                                    elide: Text.ElideRight
                                }
                            }

                            TapHandler {
                                onTapped: Notes.AppBackend.selectNoteRow(noteDelegate.index)
                            }
                        }
                    }
                }
            }

            Pane {
                SplitView.fillWidth: true
                padding: 0
                background: Rectangle {
                    color: root.panelBackground
                    border.color: root.edgeColor
                    border.width: 1
                }

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 0

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 64
                        color: root.headerBackground
                        border.color: root.edgeColor
                        border.width: 1

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 18
                            anchors.rightMargin: 18
                            spacing: 12

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2

                                Text {
                                    text: Notes.AppBackend.noteEditor.currentNoteId === -1 ? qsTr("No note selected") : Notes.AppBackend.listLabel1
                                    color: root.titleColor
                                    font.family: Notes.AppBackend.displayFontFamily
                                    font.bold: true
                                }

                                Text {
                                    text: Notes.AppBackend.noteEditor.editorDateLabel
                                    color: root.mutedColor
                                    font.family: Notes.AppBackend.displayFontFamily
                                }
                            }

                            Repeater {
                                model: Notes.AppBackend.noteEditor.tagListModel

                                delegate: Rectangle {
                                    id: tagDelegate
                                    required property var model
                                    radius: 12
                                    color: tagDelegate.model.tagColor ? tagDelegate.model.tagColor : root.accentColor
                                    opacity: 0.18
                                    implicitHeight: 24
                                    implicitWidth: label.implicitWidth + 18

                                    Text {
                                        id: label
                                        anchors.centerIn: parent
                                        text: tagDelegate.model.tagName ? tagDelegate.model.tagName : ""
                                        color: root.titleColor
                                        font.family: Notes.AppBackend.displayFontFamily
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
                                implicitHeight: editorArea.implicitHeight + 40

                                TextArea {
                                    id: editorArea
                                    width: Notes.AppBackend.textFullWidth ? editorCanvas.width - 40
                                                                          : Math.min(editorCanvas.width - 40, Notes.AppBackend.textColumnWidth)
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    anchors.top: parent.top
                                    anchors.topMargin: 20
                                    wrapMode: TextArea.Wrap
                                    selectByMouse: true
                                    readOnly: Notes.AppBackend.noteEditor.readOnly
                                    persistentSelection: true
                                    placeholderText: qsTr("Start writing…")
                                    color: root.titleColor
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
}
