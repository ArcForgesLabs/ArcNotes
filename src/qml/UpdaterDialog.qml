pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Dialog {
    id: root
    modal: true
    focus: true
    width: 420
    padding: 20
    title: ""

    property string displayFontFamily: "Roboto"
    property string themeName: "Light"
    property string titleText: qsTr("Checking for updates....")
    property string installedVersion: ""
    property string availableVersion: ""
    property string changelogHtml: "<p>No changelog found...</p>"
    property bool showProgressControls: false
    property bool progressIndeterminate: false
    property real progressValue: 0
    property string downloadLabelText: qsTr("Downloading updates...")
    property string timeRemainingText: qsTr("Time remaining: unknown")
    property bool dontNotifyAutomatically: false
    property bool updateButtonEnabled: false

    signal updateRequested()
    signal closeRequested()
    signal dontNotifyAutomaticallyToggled(bool checked)

    readonly property color windowBackground: themeName === "Dark" ? "#1f1f1f" : "#ffffff"
    readonly property color panelBackground: themeName === "Dark" ? "#262626" : "#fffdf9"
    readonly property color textColor: themeName === "Dark" ? "#f1f1f1" : "#1a1a1a"
    readonly property color mutedTextColor: themeName === "Dark" ? "#b6b6b6" : "#757575"
    readonly property color borderColor: themeName === "Dark" ? "#454545" : "#cdcdcd"
    readonly property color accentColor: "#419adf"
    readonly property color linkColor: themeName === "Dark" ? "#7ab4ff" : "#448ac9"

    background: Rectangle {
        radius: 12
        color: root.windowBackground
    }

    contentItem: ColumnLayout {
        spacing: 18

        ColumnLayout {
            spacing: 8

            Text {
                Layout.fillWidth: true
                text: root.titleText
                color: root.textColor
                font.family: root.displayFontFamily
                font.pixelSize: 25
                font.weight: Font.Medium
                wrapMode: Text.WordWrap
            }

            GridLayout {
                columns: 2
                columnSpacing: 8
                rowSpacing: 2

                Text {
                    text: qsTr("Installed Version:")
                    color: root.textColor
                    font.family: root.displayFontFamily
                }

                Text {
                    text: root.installedVersion
                    color: root.textColor
                    font.family: root.displayFontFamily
                }

                Text {
                    text: qsTr("Latest Version:")
                    color: root.textColor
                    font.family: root.displayFontFamily
                }

                Text {
                    text: root.availableVersion
                    color: root.textColor
                    font.family: root.displayFontFamily
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 4

            Text {
                text: qsTr("What's New:")
                color: root.textColor
                font.family: root.displayFontFamily
                font.pixelSize: 14
                font.weight: Font.Medium
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumHeight: 128
                radius: 8
                color: root.panelBackground
                border.color: root.borderColor
                border.width: 1

                ScrollView {
                    id: changelogScrollView
                    anchors.fill: parent
                    anchors.margins: 1
                    clip: true
                    contentWidth: availableWidth

                    Item {
                        width: changelogScrollView.availableWidth
                        implicitHeight: changelogText.implicitHeight + 24

                        TextEdit {
                            id: changelogText
                            anchors.top: parent.top
                            anchors.topMargin: 12
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: Math.max(0, changelogScrollView.availableWidth - 24)
                            readOnly: true
                            selectByMouse: true
                            wrapMode: TextEdit.Wrap
                            textFormat: TextEdit.RichText
                            text: root.changelogHtml
                            color: root.mutedTextColor
                            selectedTextColor: root.textColor
                            selectionColor: root.accentColor
                            font.family: root.displayFontFamily
                            font.pointSize: 11
                            onLinkActivated: function(link) {
                                Qt.openUrlExternally(link)
                            }
                        }
                    }
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            visible: root.showProgressControls
            spacing: 6

            Text {
                text: root.downloadLabelText
                color: root.textColor
                font.family: root.displayFontFamily
                font.pixelSize: 14
                font.weight: Font.Medium
            }

            ProgressBar {
                Layout.fillWidth: true
                from: 0
                to: 100
                value: root.progressValue
                indeterminate: root.progressIndeterminate

                background: Rectangle {
                    radius: 5
                    color: root.themeName === "Dark" ? "#313131" : "#fbfbfb"
                    border.color: root.borderColor
                    border.width: 1
                }

                contentItem: Item {
                    Rectangle {
                        width: root.progressIndeterminate ? parent.width * 0.35 : parent.width * (root.progressValue / 100)
                        height: parent.height
                        radius: 5
                        color: root.accentColor
                    }
                }
            }

            Text {
                text: root.timeRemainingText
                color: root.mutedTextColor
                font.family: root.displayFontFamily
                font.pixelSize: 13
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            CheckBox {
                id: dontNotifyCheckBox
                checked: root.dontNotifyAutomatically
                onToggled: root.dontNotifyAutomaticallyToggled(checked)
            }

            Text {
                Layout.fillWidth: true
                text: qsTr("Don't notify me automatically every time there's a new update.")
                wrapMode: Text.WordWrap
                color: root.textColor
                font.family: root.displayFontFamily
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 25

            Item {
                Layout.fillWidth: true
            }

            Button {
                text: qsTr("Close")
                flat: true
                onClicked: {
                    root.closeRequested()
                    root.close()
                }
            }

            Button {
                text: qsTr("Update")
                flat: true
                enabled: root.updateButtonEnabled
                onClicked: root.updateRequested()
            }
        }
    }
}
