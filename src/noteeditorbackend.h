#ifndef NOTEEDITORBACKEND_H
#define NOTEEDITORBACKEND_H

#include <QAbstractItemModel>
#include <QColor>
#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <QObject>
#include <QPointer>
#include <QTextDocument>
#include <QTimer>
#include <QVector>
#include <QtQml/qqmlregistration.h>

#include "editorsettingsoptions.h"
#include "nodedata.h"

class CustomMarkdownHighlighter;
class DBManager;
class TagListModel;
class TagPool;

class NoteEditorBackend : public QObject {
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(QString currentText READ currentText NOTIFY currentTextChanged)
    Q_PROPERTY(QString editorDateLabel READ editorDateLabel NOTIFY editorDateLabelChanged)
    Q_PROPERTY(int scrollBarPosition READ scrollBarPosition NOTIFY scrollBarPositionChanged)
    Q_PROPERTY(int currentNoteId READ currentEditingNoteId NOTIFY currentNoteIdChanged)
    Q_PROPERTY(bool markdownEnabled READ markdownEnabled WRITE setMarkdownEnabled NOTIFY markdownEnabledChanged)
    Q_PROPERTY(bool readOnly READ readOnly NOTIFY readOnlyChanged)
    Q_PROPERTY(QAbstractItemModel* tagListModel READ tagListModel CONSTANT)

public:
    explicit NoteEditorBackend(TagPool* tagPool, DBManager* dbManager, QObject* parent = nullptr);

    [[nodiscard]] QString currentText() const;
    [[nodiscard]] QString editorDateLabel() const;
    [[nodiscard]] int scrollBarPosition() const;
    [[nodiscard]] bool markdownEnabled() const;
    void setMarkdownEnabled(bool enabled);
    [[nodiscard]] bool readOnly() const;
    [[nodiscard]] QAbstractItemModel* tagListModel() const;
    [[nodiscard]] bool isTempNote() const;
    [[nodiscard]] int currentEditingNoteId() const;

    void setForcedReadOnly(bool forcedReadOnly);
    void updateHighlightingTheme(Theme::Value theme, const QColor& textColor, qreal fontSize);

    Q_INVOKABLE void setCurrentText(const QString& text);
    Q_INVOKABLE void setScrollBarPosition(int value);
    Q_INVOKABLE void saveNoteToDB();
    Q_INVOKABLE void closeEditor();
    Q_INVOKABLE void deleteCurrentNote();
    Q_INVOKABLE bool checkForTasksInEditor();
    Q_INVOKABLE void rearrangeTasksInTextEditor(int startLinePosition, int endLinePosition, int newLinePosition);
    Q_INVOKABLE void rearrangeColumnsInTextEditor(int startLinePosition, int endLinePosition, int newLinePosition);
    Q_INVOKABLE void checkTaskInLine(int lineNumber);
    Q_INVOKABLE void uncheckTaskInLine(int lineNumber);
    Q_INVOKABLE void updateTaskText(int startLinePosition, int endLinePosition, const QString& newText);
    Q_INVOKABLE void addNewTask(int startLinePosition, const QString& newTaskText);
    Q_INVOKABLE void removeTask(int startLinePosition, int endLinePosition);
    Q_INVOKABLE void addNewColumn(int startLinePosition, const QString& columnTitle);
    Q_INVOKABLE void removeColumn(int startLinePosition, int endLinePosition);
    Q_INVOKABLE void updateColumnTitle(int lineNumber, const QString& newText);
    Q_INVOKABLE void attachTextDocument(QObject* textDocumentObject);

    void showTextView();
    void showKanbanView();

    static QString getNthLine(const QString& str, int targetLineNumber);
    static QString getFirstLine(const QString& str);
    static QString getSecondLine(const QString& str);
    static QString getNoteDateEditor(const QString& dateEdited);

public slots:
    void showNotesInEditor(const QVector<NodeData>& notes);
    void onNoteTagListChanged(int noteId, const QSet<int>& tagIds);

signals:
    void currentTextChanged();
    void editorDateLabelChanged();
    void scrollBarPositionChanged();
    void currentNoteIdChanged();
    void markdownEnabledChanged();
    void readOnlyChanged();

    void requestCreateUpdateNote(const NodeData& note);
    void noteEditClosed(const NodeData& note, bool selectNext);
    void moveNoteToListViewTop(const NodeData& note);
    void updateNoteDataInList(const NodeData& note);
    void deleteNoteRequested(const NodeData& note);
    void textShown();
    void kanbanShown();
    void tasksFoundInEditor(QVariant data);
    void clearKanbanModel();
    void resetKanbanSettings();
    void checkMultipleNotesSelected(QVariant isMultipleNotesSelected);

private:
    [[nodiscard]] bool isInEditMode() const;
    void showTagListForCurrentNote();
    void setEditorText(const QString& text, bool updateCurrentNote);
    void setEditorDateLabel(const QString& text);
    void setReadOnlyState(bool readOnlyState);
    void clearEditorState();
    void syncTextFromDocument(bool updateCurrentNote);
    void refreshMarkdownHighlighter();
    void clearDocumentHighlighting() const;

    static QDateTime getQDateTime(const QString& date);
    QMap<QString, int> getTaskDataInLine(const QString& line) const;
    void replaceTextBetweenLines(int startLinePosition, int endLinePosition, QString& newText);
    void removeTextBetweenLines(int startLinePosition, int endLinePosition);
    void appendNewColumn(QJsonArray& data, QJsonObject& currentColumn, QString& currentTitle, QJsonArray& tasks);
    void addUntitledColumnToTextEditor(int startLinePosition);

    TagListModel* m_tagListModel;
    DBManager* m_dbManager;
    QTextDocument m_document;
    QPointer<QTextDocument> m_attachedTextDocument;
    CustomMarkdownHighlighter* m_highlighter;
    QVector<NodeData> m_currentNotes;
    QTimer m_autoSaveTimer;
    QString m_currentText;
    QString m_editorDateLabel;
    int m_scrollBarPosition;
    bool m_markdownEnabled;
    bool m_forcedReadOnly;
    bool m_isContentModified;
    Theme::Value m_highlightingTheme;
    QColor m_highlightingTextColor;
    qreal m_highlightingFontSize;
};

#endif  // NOTEEDITORBACKEND_H
