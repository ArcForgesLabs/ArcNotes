#include "noteeditorbackend.h"

#include <QColor>
#include <QDateTime>
#include <QLocale>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextStream>
#include <QtQuick/qquicktextdocument.h>

#include "custommarkdownhighlighter.h"
#include "dbmanager.h"
#include "taglistmodel.h"
#include "tagpool.h"

namespace {
auto constexpr FIRST_LINE_MAX = 80;
}

NoteEditorBackend::NoteEditorBackend(TagPool* tagPool, DBManager* dbManager, QObject* parent)
    : QObject(parent),
      m_tagListModel(new TagListModel(this)),
      m_dbManager(dbManager),
      m_highlighter(nullptr),
      m_scrollBarPosition(0),
      m_markdownEnabled(true),
      m_forcedReadOnly(false),
      m_isContentModified(false),
      m_highlightingTheme(Theme::Light),
      m_highlightingTextColor(QStringLiteral("#2f2c26")),
      m_highlightingFontSize(13.0) {
    m_tagListModel->setTagPool(tagPool);
    m_autoSaveTimer.setSingleShot(true);
    m_autoSaveTimer.setInterval(50);
    connect(&m_autoSaveTimer, &QTimer::timeout, this, &NoteEditorBackend::saveNoteToDB);
    connect(tagPool, &TagPool::dataUpdated, this, [this](int) { showTagListForCurrentNote(); });
}

QString NoteEditorBackend::currentText() const {
    return m_currentText;
}

QString NoteEditorBackend::editorDateLabel() const {
    return m_editorDateLabel;
}

int NoteEditorBackend::scrollBarPosition() const {
    return m_scrollBarPosition;
}

bool NoteEditorBackend::markdownEnabled() const {
    return m_markdownEnabled;
}

void NoteEditorBackend::setMarkdownEnabled(bool enabled) {
    if (m_markdownEnabled == enabled) {
        return;
    }
    m_markdownEnabled = enabled;
    refreshMarkdownHighlighter();
    emit markdownEnabledChanged();
}

bool NoteEditorBackend::readOnly() const {
    return m_forcedReadOnly || m_currentNotes.size() != 1;
}

QAbstractItemModel* NoteEditorBackend::tagListModel() const {
    return m_tagListModel;
}

bool NoteEditorBackend::isTempNote() const {
    return currentEditingNoteId() != INVALID_NODE_ID && m_currentNotes[0].isTempNote();
}

int NoteEditorBackend::currentEditingNoteId() const {
    if (isInEditMode()) {
        return m_currentNotes[0].id();
    }
    return INVALID_NODE_ID;
}

void NoteEditorBackend::setForcedReadOnly(bool forcedReadOnly) {
    if (m_forcedReadOnly == forcedReadOnly) {
        return;
    }
    m_forcedReadOnly = forcedReadOnly;
    emit readOnlyChanged();
}

void NoteEditorBackend::updateHighlightingTheme(Theme::Value theme, const QColor& textColor, qreal fontSize) {
    m_highlightingTheme = theme;
    m_highlightingTextColor = textColor;
    m_highlightingFontSize = fontSize;
    refreshMarkdownHighlighter();
}

void NoteEditorBackend::setCurrentText(const QString& text) {
    if (readOnly() || currentEditingNoteId() == INVALID_NODE_ID) {
        return;
    }
    setEditorText(text, true);
}

void NoteEditorBackend::setScrollBarPosition(int value) {
    if (m_scrollBarPosition == value) {
        return;
    }
    m_scrollBarPosition = value;
    emit scrollBarPositionChanged();
    if (isInEditMode() && currentEditingNoteId() != INVALID_NODE_ID) {
        m_currentNotes[0].setScrollBarPosition(value);
        emit updateNoteDataInList(m_currentNotes[0]);
        m_isContentModified = true;
        m_autoSaveTimer.start();
    }
}

void NoteEditorBackend::saveNoteToDB() {
    if (currentEditingNoteId() != INVALID_NODE_ID && m_isContentModified && !m_currentNotes[0].isTempNote()) {
        emit requestCreateUpdateNote(m_currentNotes[0]);
        m_isContentModified = false;
    }
}

void NoteEditorBackend::closeEditor() {
    if (currentEditingNoteId() != INVALID_NODE_ID) {
        saveNoteToDB();
        emit noteEditClosed(m_currentNotes[0], false);
    }
    clearEditorState();
}

void NoteEditorBackend::deleteCurrentNote() {
    if (isTempNote()) {
        const auto noteNeedDeleted = m_currentNotes[0];
        clearEditorState();
        emit noteEditClosed(noteNeedDeleted, true);
        return;
    }

    if (currentEditingNoteId() != INVALID_NODE_ID) {
        const auto noteNeedDeleted = m_currentNotes[0];
        saveNoteToDB();
        clearEditorState();
        emit noteEditClosed(noteNeedDeleted, false);
        emit deleteNoteRequested(noteNeedDeleted);
    }
}

void NoteEditorBackend::showNotesInEditor(const QVector<NodeData>& notes) {
    if (notes.size() == 1 && notes[0].id() != INVALID_NODE_ID) {
        const auto currentId = currentEditingNoteId();
        if (currentId != INVALID_NODE_ID && notes[0].id() != currentId) {
            emit noteEditClosed(m_currentNotes[0], false);
        }

        emit resetKanbanSettings();
        emit checkMultipleNotesSelected(QVariant(false));

        m_currentNotes = notes;
        emit currentNoteIdChanged();
        showTagListForCurrentNote();
        setEditorText(notes[0].content(), false);
        setEditorDateLabel(getNoteDateEditor(notes[0].lastModificationdateTime().toString(Qt::ISODate)));
        if (m_scrollBarPosition != notes[0].scrollBarPosition()) {
            m_scrollBarPosition = notes[0].scrollBarPosition();
            emit scrollBarPositionChanged();
        }
        emit readOnlyChanged();
        emit textShown();
        return;
    }

    if (notes.size() > 1) {
        m_currentNotes = notes;
        emit currentNoteIdChanged();
        m_tagListModel->setModelData({});
        QStringList contents;
        contents.reserve(notes.size());
        for (const auto& note : notes) {
            contents.append(note.content());
        }
        setEditorText(contents.join(QStringLiteral("\n\n-----\n\n")), false);
        setEditorDateLabel(QString());
        emit checkMultipleNotesSelected(QVariant(true));
        emit readOnlyChanged();
        emit textShown();
        return;
    }

    clearEditorState();
}

void NoteEditorBackend::onNoteTagListChanged(int noteId, const QSet<int>& tagIds) {
    if (currentEditingNoteId() == noteId) {
        m_currentNotes[0].setTagIds(tagIds);
        showTagListForCurrentNote();
    }
}

void NoteEditorBackend::showTextView() {
    emit clearKanbanModel();
    emit textShown();
}

void NoteEditorBackend::showKanbanView() {
    bool shouldRecheck = checkForTasksInEditor();
    if (shouldRecheck) {
        checkForTasksInEditor();
    }
    emit kanbanShown();
}

QString NoteEditorBackend::getNthLine(const QString& str, int targetLineNumber) {
    if (targetLineNumber < 1) {
        return tr("Invalid line number");
    }

    int previousLineBreakIndex = -1;
    int lineCount = 0;
    for (int i = 0; i <= str.length(); i++) {
        if (i == str.length() || str[i] == '\n') {
            lineCount++;
            if (lineCount >= targetLineNumber &&
                (i - previousLineBreakIndex > 1 || (i > 0 && i == str.length() && str[i - 1] != '\n'))) {
                QString line = str.mid(previousLineBreakIndex + 1, i - previousLineBreakIndex - 1);
                line = line.trimmed();
                if (!line.isEmpty() && !line.startsWith("---") && !line.startsWith("```")) {
                    QTextDocument doc;
                    doc.setMarkdown(line);
                    QString text = doc.toPlainText();
                    if (text.length() > 1 && text.at(0) == '^') {
                        text = text.mid(1);
                    }
                    if (text.isEmpty()) {
                        return tr("No additional text");
                    }
                    QTextStream ts(&text);
                    return ts.readLine(FIRST_LINE_MAX);
                }
            }
            previousLineBreakIndex = i;
        }
    }

    return tr("No additional text");
}

QString NoteEditorBackend::getFirstLine(const QString& str) {
    return getNthLine(str, 1);
}

QString NoteEditorBackend::getSecondLine(const QString& str) {
    return getNthLine(str, 2);
}

QString NoteEditorBackend::getNoteDateEditor(const QString& dateEdited) {
    QDateTime dateTimeEdited(getQDateTime(dateEdited));
    QLocale usLocale(QLocale(QStringLiteral("en_US")));
    return usLocale.toString(dateTimeEdited, QStringLiteral("MMMM d, yyyy, h:mm A"));
}

bool NoteEditorBackend::checkForTasksInEditor() {
    const QStringList lines = m_currentText.split('\n');
    QJsonArray data;
    QJsonObject currentColumn;
    QJsonArray tasks;
    QString currentTitle;
    bool isPreviousLineATask = false;

    for (int i = 0; i < lines.size(); i++) {
        const QString& line = lines[i];
        const QString lineTrimmed = line.trimmed();

        if (lineTrimmed.startsWith('#')) {
            if (!tasks.isEmpty() && currentTitle.isEmpty()) {
                addUntitledColumnToTextEditor(tasks.first().toObject()["taskStartLine"].toInt());
                syncTextFromDocument(true);
                return true;
            }
            appendNewColumn(data, currentColumn, currentTitle, tasks);
            currentColumn["columnStartLine"] = i;
            const int countOfHashTags = lineTrimmed.count('#');
            currentTitle = lineTrimmed.mid(countOfHashTags);
            isPreviousLineATask = false;
        } else if (lineTrimmed.endsWith("::") && getTaskDataInLine(line)["taskMatchIndex"] == -1) {
            if (!tasks.isEmpty() && currentTitle.isEmpty()) {
                addUntitledColumnToTextEditor(tasks.first().toObject()["taskStartLine"].toInt());
                syncTextFromDocument(true);
                return true;
            }
            appendNewColumn(data, currentColumn, currentTitle, tasks);
            currentColumn["columnStartLine"] = i;
            currentTitle = line.split("::").first().trimmed();
            isPreviousLineATask = false;
        } else {
            const auto taskDataInLine = getTaskDataInLine(line);
            const int indexOfTaskInLine = taskDataInLine["taskMatchIndex"];
            if (indexOfTaskInLine != -1) {
                QJsonObject taskObject;
                const QString taskText =
                    line.mid(indexOfTaskInLine + taskDataInLine["taskExpressionSize"]).trimmed();
                taskObject["text"] = taskText;
                taskObject["checked"] = taskDataInLine["taskChecked"] == 1;
                taskObject["taskStartLine"] = i;
                taskObject["taskEndLine"] = i;
                tasks.append(taskObject);
                isPreviousLineATask = true;
            } else if (!line.isEmpty() && isPreviousLineATask) {
                if (!tasks.empty()) {
                    auto newTask = tasks[tasks.size() - 1].toObject();
                    const QString newTaskText = QStringLiteral("%1  \n%2").arg(newTask["text"].toString(), lineTrimmed);
                    newTask["text"] = newTaskText;
                    newTask["taskEndLine"] = i;
                    tasks[tasks.size() - 1] = newTask;
                }
            } else {
                isPreviousLineATask = false;
            }
        }
    }

    if (!tasks.isEmpty() && currentTitle.isEmpty()) {
        addUntitledColumnToTextEditor(tasks.first().toObject()["taskStartLine"].toInt());
        syncTextFromDocument(true);
        return true;
    }

    appendNewColumn(data, currentColumn, currentTitle, tasks);
    emit tasksFoundInEditor(QVariant(data));
    return false;
}

void NoteEditorBackend::rearrangeTasksInTextEditor(int startLinePosition, int endLinePosition, int newLinePosition) {
    QTextCursor cursor(&m_document);
    cursor.setPosition(m_document.findBlockByNumber(startLinePosition).position());
    cursor.setPosition(m_document.findBlockByNumber(endLinePosition).position(), QTextCursor::KeepAnchor);
    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    if (m_document.findBlockByNumber(endLinePosition + 1).isValid()) {
        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
    }
    QString selectedText = cursor.selectedText();
    cursor.removeSelectedText();

    if (newLinePosition <= startLinePosition) {
        cursor.setPosition(m_document.findBlockByLineNumber(newLinePosition).position());
    } else {
        const int newPositionBecauseOfRemoval = newLinePosition - (endLinePosition - startLinePosition + 1);
        if (newPositionBecauseOfRemoval == m_document.lineCount()) {
            cursor.setPosition(m_document.findBlockByLineNumber(newPositionBecauseOfRemoval - 1).position());
            cursor.movePosition(QTextCursor::EndOfBlock);
            selectedText = "\n" + selectedText;
        } else {
            cursor.setPosition(m_document.findBlockByLineNumber(newPositionBecauseOfRemoval).position());
        }
    }
    cursor.insertText(selectedText);
    syncTextFromDocument(true);
    checkForTasksInEditor();
}

void NoteEditorBackend::rearrangeColumnsInTextEditor(int startLinePosition, int endLinePosition, int newLinePosition) {
    QTextCursor cursor(&m_document);
    cursor.setPosition(m_document.findBlockByNumber(startLinePosition).position());
    cursor.setPosition(m_document.findBlockByNumber(endLinePosition).position(), QTextCursor::KeepAnchor);
    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    if (m_document.findBlockByNumber(endLinePosition + 1).isValid()) {
        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
    }
    const QString selectedText = cursor.selectedText();
    cursor.removeSelectedText();
    cursor.setPosition(m_document.findBlockByNumber(startLinePosition).position());
    if (m_document.findBlockByNumber(startLinePosition + 1).isValid()) {
        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
    }

    if (startLinePosition < newLinePosition) {
        const int newPositionBecauseOfRemoval = newLinePosition - (endLinePosition - startLinePosition + 1);
        cursor.setPosition(m_document.findBlockByLineNumber(newPositionBecauseOfRemoval).position());
        cursor.movePosition(QTextCursor::EndOfBlock);
        cursor.insertText("\n" + selectedText);
    } else {
        cursor.setPosition(m_document.findBlockByLineNumber(newLinePosition).position());
        cursor.insertText(selectedText + "\n");
    }

    syncTextFromDocument(true);
    checkForTasksInEditor();
}

void NoteEditorBackend::checkTaskInLine(int lineNumber) {
    const QTextBlock block = m_document.findBlockByLineNumber(lineNumber);
    if (!block.isValid()) {
        return;
    }
    const int indexOfTaskInLine = getTaskDataInLine(block.text())["taskMatchIndex"];
    if (indexOfTaskInLine == -1) {
        return;
    }
    QTextCursor cursor(block);
    cursor.setPosition(block.position() + indexOfTaskInLine + 3, QTextCursor::MoveAnchor);
    cursor.deleteChar();
    cursor.insertText("x");
    syncTextFromDocument(true);
}

void NoteEditorBackend::uncheckTaskInLine(int lineNumber) {
    const QTextBlock block = m_document.findBlockByLineNumber(lineNumber);
    if (!block.isValid()) {
        return;
    }
    const int indexOfTaskInLine = getTaskDataInLine(block.text())["taskMatchIndex"];
    if (indexOfTaskInLine == -1) {
        return;
    }
    QTextCursor cursor(block);
    cursor.setPosition(block.position() + indexOfTaskInLine + 3, QTextCursor::MoveAnchor);
    cursor.deleteChar();
    cursor.insertText(" ");
    syncTextFromDocument(true);
}

void NoteEditorBackend::updateTaskText(int startLinePosition, int endLinePosition, const QString& newText) {
    const QTextBlock block = m_document.findBlockByLineNumber(startLinePosition);
    if (!block.isValid()) {
        return;
    }

    const auto taskData = getTaskDataInLine(block.text());
    const int indexOfTaskInLine = taskData["taskMatchIndex"];
    if (indexOfTaskInLine == -1) {
        return;
    }

    const QString taskExpressionText = block.text().mid(0, taskData["taskMatchIndex"] + taskData["taskExpressionSize"]);
    QString newTextModified = newText;
    newTextModified.replace("\n\n", "\n");
    newTextModified.replace("~~", "");

    const QStringList taskExpressions = {"- [ ]", "- [x]", "* [ ]", "* [x]", "- [X]", "* [X]"};
    for (const auto& taskExpression : taskExpressions) {
        newTextModified.replace(taskExpression, "");
    }

    if (newTextModified.count('\n') > 1) {
        QStringList newTextModifiedSplitted = newTextModified.split('\n');
        if (newTextModifiedSplitted.size() > 1) {
            for (int i = 1; i < newTextModifiedSplitted.size(); i++) {
                newTextModifiedSplitted[i].replace("# ", "");
                newTextModifiedSplitted[i].replace("#", "");
            }
            newTextModified = newTextModifiedSplitted.join('\n');
        }
    }

    QString newTaskText = QStringLiteral("%1 %2").arg(taskExpressionText, newTextModified);
    if (!newTaskText.isEmpty() && newTaskText.back() == '\n') {
        newTaskText.chop(1);
    }
    replaceTextBetweenLines(startLinePosition, endLinePosition, newTaskText);
    syncTextFromDocument(true);
    checkForTasksInEditor();
}

void NoteEditorBackend::addNewTask(int startLinePosition, const QString& newTaskText) {
    if (startLinePosition < 0) {
        return;
    }
    const QString newText = QStringLiteral("\n- [ ] %1").arg(newTaskText);
    QTextBlock startBlock = m_document.findBlockByLineNumber(startLinePosition);
    if (!startBlock.isValid()) {
        return;
    }
    QTextCursor cursor(startBlock);
    cursor.movePosition(QTextCursor::EndOfBlock);
    cursor.insertText(newText);
    syncTextFromDocument(true);
    checkForTasksInEditor();
}

void NoteEditorBackend::removeTask(int startLinePosition, int endLinePosition) {
    removeTextBetweenLines(startLinePosition, endLinePosition);
    syncTextFromDocument(true);
    checkForTasksInEditor();
}

void NoteEditorBackend::addNewColumn(int startLinePosition, const QString& columnTitle) {
    if (startLinePosition < 0) {
        return;
    }

    QTextBlock block = m_document.findBlockByNumber(startLinePosition);
    if (block.isValid()) {
        QTextCursor cursor(block);
        if (startLinePosition == 0) {
            cursor.movePosition(QTextCursor::StartOfBlock);
        } else {
            cursor.movePosition(QTextCursor::EndOfBlock);
        }
        cursor.insertText(columnTitle);
    } else {
        QTextCursor cursor(&m_document);
        cursor.movePosition(QTextCursor::End);
        cursor.insertText(columnTitle);
    }

    syncTextFromDocument(true);
    checkForTasksInEditor();
}

void NoteEditorBackend::removeColumn(int startLinePosition, int endLinePosition) {
    removeTextBetweenLines(startLinePosition, endLinePosition);

    if (startLinePosition < 0 || endLinePosition < startLinePosition) {
        return;
    }

    QTextCursor cursor(&m_document);
    cursor.setPosition(m_document.findBlockByNumber(startLinePosition).position());
    if (cursor.block().isValid() && cursor.block().text().isEmpty()) {
        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
    }

    syncTextFromDocument(true);
    checkForTasksInEditor();
}

void NoteEditorBackend::updateColumnTitle(int lineNumber, const QString& newText) {
    QTextBlock block = m_document.findBlockByLineNumber(lineNumber);
    if (!block.isValid()) {
        return;
    }

    const int lastIndexOfHashTag = block.text().lastIndexOf('#');
    if (lastIndexOfHashTag != -1) {
        QTextCursor cursor(block);
        cursor.setPosition(block.position() + lastIndexOfHashTag + 1, QTextCursor::MoveAnchor);
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
        cursor.setPosition(block.position() + lastIndexOfHashTag + 1);
        cursor.insertText(" " + newText);
        syncTextFromDocument(true);
        return;
    }

    const int lastIndexOfColon = block.text().lastIndexOf("::");
    if (lastIndexOfColon != -1) {
        QTextCursor cursor(block);
        cursor.setPosition(block.position(), QTextCursor::MoveAnchor);
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
        cursor.setPosition(block.position());
        cursor.insertText(newText + "::");
        syncTextFromDocument(true);
    }
}

void NoteEditorBackend::attachTextDocument(QObject* textDocumentObject) {
    auto* quickTextDocument = qobject_cast<QQuickTextDocument*>(textDocumentObject);
    if (quickTextDocument == nullptr) {
        return;
    }

    m_attachedTextDocument = quickTextDocument->textDocument();
    if (m_attachedTextDocument == nullptr) {
        return;
    }

    if (m_highlighter == nullptr) {
        m_highlighter = new CustomMarkdownHighlighter(m_attachedTextDocument, MarkdownHighlighter::HighlightingOption::None);
    } else {
        m_highlighter->setDocument(m_attachedTextDocument);
    }

    refreshMarkdownHighlighter();
}

bool NoteEditorBackend::isInEditMode() const {
    return m_currentNotes.size() == 1;
}

void NoteEditorBackend::showTagListForCurrentNote() {
    if (currentEditingNoteId() != INVALID_NODE_ID) {
        m_tagListModel->setModelData(m_currentNotes[0].tagIds());
        return;
    }
    m_tagListModel->setModelData({});
}

void NoteEditorBackend::setEditorText(const QString& text, bool updateCurrentNote) {
    const bool textChanged = m_currentText != text;
    if (textChanged) {
        m_currentText = text;
        emit currentTextChanged();
    }
    if (m_document.toPlainText() != text) {
        m_document.setPlainText(text);
    }

    if (!updateCurrentNote || currentEditingNoteId() == INVALID_NODE_ID) {
        return;
    }

    emit moveNoteToListViewTop(m_currentNotes[0]);
    const QDateTime dateTime = QDateTime::currentDateTime();
    m_currentNotes[0].setContent(m_currentText);
    m_currentNotes[0].setFullTitle(getFirstLine(m_currentText));
    m_currentNotes[0].setLastModificationDateTime(dateTime);
    m_currentNotes[0].setIsTempNote(false);
    m_currentNotes[0].setScrollBarPosition(m_scrollBarPosition);
    setEditorDateLabel(getNoteDateEditor(dateTime.toString(Qt::ISODate)));
    emit updateNoteDataInList(m_currentNotes[0]);
    m_isContentModified = true;
    m_autoSaveTimer.start();
}

void NoteEditorBackend::setEditorDateLabel(const QString& text) {
    if (m_editorDateLabel == text) {
        return;
    }
    m_editorDateLabel = text;
    emit editorDateLabelChanged();
}

void NoteEditorBackend::setReadOnlyState(bool readOnlyState) {
    Q_UNUSED(readOnlyState)
    emit readOnlyChanged();
}

void NoteEditorBackend::clearEditorState() {
    m_currentNotes.clear();
    emit currentNoteIdChanged();
    m_isContentModified = false;
    m_tagListModel->setModelData({});
    setEditorText(QString(), false);
    setEditorDateLabel(QString());
    if (m_scrollBarPosition != 0) {
        m_scrollBarPosition = 0;
        emit scrollBarPositionChanged();
    }
    emit clearKanbanModel();
    emit readOnlyChanged();
}

void NoteEditorBackend::syncTextFromDocument(bool updateCurrentNote) {
    setEditorText(m_document.toPlainText(), updateCurrentNote);
}

void NoteEditorBackend::refreshMarkdownHighlighter() {
    if (m_highlighter == nullptr) {
        return;
    }

    m_highlighter->setTheme(m_highlightingTheme, m_highlightingTextColor, m_highlightingFontSize);
    if (m_markdownEnabled && m_attachedTextDocument != nullptr) {
        if (m_highlighter->document() != m_attachedTextDocument) {
            m_highlighter->setDocument(m_attachedTextDocument);
        }
        m_highlighter->rehighlight();
        return;
    }

    m_highlighter->setDocument(nullptr);
    clearDocumentHighlighting();
}

void NoteEditorBackend::clearDocumentHighlighting() const {
    if (m_attachedTextDocument == nullptr) {
        return;
    }

    QTextCursor cursor(m_attachedTextDocument);
    cursor.beginEditBlock();
    cursor.select(QTextCursor::Document);
    cursor.setCharFormat(QTextCharFormat());
    cursor.clearSelection();
    cursor.endEditBlock();
}

QDateTime NoteEditorBackend::getQDateTime(const QString& date) {
    return QDateTime::fromString(date, Qt::ISODate);
}

QMap<QString, int> NoteEditorBackend::getTaskDataInLine(const QString& line) const {
    const QStringList taskExpressions = {"- [ ]", "- [x]", "* [ ]", "* [x]", "- [X]", "* [X]"};
    QMap<QString, int> taskMatchLineData;
    taskMatchLineData["taskMatchIndex"] = -1;

    for (const auto& taskExpression : taskExpressions) {
        const int taskMatchIndex = line.indexOf(taskExpression);
        if (taskMatchIndex != -1) {
            taskMatchLineData["taskMatchIndex"] = taskMatchIndex;
            taskMatchLineData["taskExpressionSize"] = taskExpression.size();
            taskMatchLineData["taskChecked"] = taskExpression[3] == 'x' ? 1 : 0;
            return taskMatchLineData;
        }
    }

    return taskMatchLineData;
}

void NoteEditorBackend::replaceTextBetweenLines(int startLinePosition, int endLinePosition, QString& newText) {
    QTextBlock startBlock = m_document.findBlockByLineNumber(startLinePosition);
    QTextBlock endBlock = m_document.findBlockByLineNumber(endLinePosition);
    QTextCursor cursor(startBlock);
    cursor.setPosition(endBlock.position() + endBlock.length() - 1, QTextCursor::KeepAnchor);
    cursor.removeSelectedText();
    cursor.insertText(newText);
}

void NoteEditorBackend::removeTextBetweenLines(int startLinePosition, int endLinePosition) {
    if (startLinePosition < 0 || endLinePosition < startLinePosition) {
        return;
    }

    QTextCursor cursor(&m_document);
    cursor.setPosition(m_document.findBlockByNumber(startLinePosition).position());
    cursor.setPosition(m_document.findBlockByNumber(endLinePosition).position(), QTextCursor::KeepAnchor);
    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    if (m_document.findBlockByNumber(endLinePosition + 1).isValid()) {
        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
    }
    cursor.removeSelectedText();
}

void NoteEditorBackend::appendNewColumn(QJsonArray& data, QJsonObject& currentColumn, QString& currentTitle,
                                        QJsonArray& tasks) {
    if (tasks.isEmpty()) {
        return;
    }
    currentColumn["title"] = currentTitle;
    currentColumn["tasks"] = tasks;
    currentColumn["columnEndLine"] = tasks.last().toObject()["taskEndLine"];
    data.append(currentColumn);
    currentColumn = QJsonObject();
    tasks = QJsonArray();
}

void NoteEditorBackend::addUntitledColumnToTextEditor(int startLinePosition) {
    QTextBlock block = m_document.findBlockByNumber(startLinePosition);
    if (!block.isValid()) {
        return;
    }
    QTextCursor cursor(block);
    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.insertText(QStringLiteral("# Untitled\n\n"));
}
