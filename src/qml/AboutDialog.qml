pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

Dialog {
    id: root
    modal: true
    focus: true
    padding: 0
    width: 390
    height: 368
    title: qsTr("About %1").arg(root.applicationName)

    property string applicationName: Qt.application.name ? Qt.application.name : "ArcNotes"
    property string applicationVersion: Qt.application.version ? Qt.application.version : "0.0.0"
    property string displayFontFamily: "Roboto"
    property string themeName: "Light"
    property bool proVersion: false
    property string aboutHtml: root.buildAboutHtml()

    readonly property color backgroundColor: themeName === "Dark" ? "#191919"
                                                                  : themeName === "Sepia" ? "#fbf0d9" : "#f7f7f7"
    readonly property color foregroundColor: themeName === "Dark" ? "#d4d4d4" : "#1a1a1a"
    readonly property color linkColor: themeName === "Dark" ? "#7ab4ff" : "#0a84ff"

    function buildAboutHtml() {
        const proVersionText = root.proVersion ? " (Pro Version)" : ""
        return "<style>" +
                "a { color: " + root.linkColor + "; text-decoration: none; }" +
                "p, li { white-space: pre-wrap; }" +
                "</style>" +
                "<strong>Version:</strong> " + root.applicationVersion + proVersionText +
                "<p>Notes was founded by <a href='https://rubymamistvalove.com'>Ruby Mamistvalove</a>, " +
                "to create an elegant yet powerful cross-platform and open-source note-taking app.</p>" +
                "<a href='https://www.notes-foss.com'>Notes Website</a><br/>" +
                "<a href='https://github.com/nuttyartist/notes'>Source code on Github</a><br/>" +
                "<a href='https://www.notes-foss.com/notes-app-terms-privacy-policy'>Terms and Privacy Policy</a><br/><br/>" +
                "<strong>Acknowledgments</strong><br/>" +
                "This project couldn't come this far without the help of these amazing people:<br/><br/>" +
                "<strong>Programmers:</strong><br/>" +
                "Alex Spataru<br/>Ali Diouri<br/>David Planella<br/>Diep Ngoc<br/>" +
                "Guilherme Silva<br/>Thorbjorn Lindeijer<br/>Tuur Vanhoutte<br/>Waqar Ahmed<br/><br/>" +
                "<strong>Designers:</strong><br/>Kevin Doyle<br/><br/>" +
                "And to the many of our beloved users who keep sending us feedback, you are an essential force " +
                "in helping us improve, thank you!<br/><br/>" +
                "<strong>Notes makes use of the following third-party libraries:</strong><br/><br/>" +
                "QMarkdownTextEdit<br/>QAutostart<br/>QXT<br/><br/>" +
                "<strong>Notes makes use of the following open source fonts:</strong><br/><br/>" +
                "Roboto<br/>Source Sans Pro<br/>Trykker<br/>Mate<br/>iA Writer Mono<br/>" +
                "iA Writer Duo<br/>iA Writer Quattro<br/>Font Awesome<br/>Material Symbols<br/><br/>" +
                "<strong>Qt version:</strong> " + Qt.version
    }

    background: Rectangle {
        radius: 12
        color: root.backgroundColor
    }

    contentItem: ScrollView {
        id: scrollView
        clip: true
        contentWidth: availableWidth

        Item {
            width: scrollView.availableWidth
            implicitHeight: aboutText.implicitHeight + 40

            TextEdit {
                id: aboutText
                anchors.top: parent.top
                anchors.topMargin: 20
                anchors.horizontalCenter: parent.horizontalCenter
                width: Math.max(0, scrollView.availableWidth - 40)
                readOnly: true
                selectByMouse: true
                wrapMode: TextEdit.Wrap
                textFormat: TextEdit.RichText
                text: root.aboutHtml
                color: root.foregroundColor
                selectedTextColor: root.foregroundColor
                selectionColor: root.linkColor
                font.family: root.displayFontFamily
                font.pointSize: 11
                onLinkActivated: function(link) {
                    Qt.openUrlExternally(link)
                }
            }
        }
    }
}
