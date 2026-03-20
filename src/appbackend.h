#ifndef APPBACKEND_H
#define APPBACKEND_H

#include <QtQml/qqmlregistration.h>

#include <QAbstractItemModel>
#include <QJsonObject>
#include <QObject>
#include <QSet>
#include <QSettings>
#include <QThread>
#include <QTimer>
#include <QUrl>

#include "dbmanager.h"
#include "editorsettingsoptions.h"
#include "noteeditorbackend.h"
#include "subscriptionstatus.h"

class DBManager;
class NodeTreeModel;
class NoteListModel;
class TagPool;

class AppBackend : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QAbstractItemModel* noteModel READ noteModel CONSTANT)
    Q_PROPERTY(QAbstractItemModel* treeModel READ treeModel CONSTANT)
    Q_PROPERTY(NoteEditorBackend* noteEditor READ noteEditor CONSTANT)
    Q_PROPERTY(QString platformName READ platformName CONSTANT)
    Q_PROPERTY(QString displayFontFamily READ displayFontFamily NOTIFY displayFontFamilyChanged)
    Q_PROPERTY(QString editorFontFamily READ editorFontFamily NOTIFY editorFontChanged)
    Q_PROPERTY(int editorFontPointSize READ editorFontPointSize NOTIFY editorFontChanged)
    Q_PROPERTY(int textColumnWidth READ textColumnWidth NOTIFY editorLayoutChanged)
    Q_PROPERTY(bool textFullWidth READ textFullWidth NOTIFY editorLayoutChanged)
    Q_PROPERTY(QVariant themeData READ themeData NOTIFY themeDataObjectChanged)
    Q_PROPERTY(QString listLabel1 READ listLabel1 NOTIFY listLabelsChanged)
    Q_PROPERTY(QString listLabel2 READ listLabel2 NOTIFY listLabelsChanged)
    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY searchTextChanged)
    Q_PROPERTY(bool folderTreeCollapsed READ folderTreeCollapsed NOTIFY folderTreeCollapsedChanged)
    Q_PROPERTY(bool noteListCollapsed READ noteListCollapsed NOTIFY noteListCollapsedChanged)
    Q_PROPERTY(bool kanbanVisible READ kanbanVisible NOTIFY kanbanVisibleChanged)
    Q_PROPERTY(bool alwaysOnTop READ alwaysOnTop NOTIFY alwaysOnTopChanged)
    Q_PROPERTY(bool canCreateNotes READ canCreateNotes NOTIFY canCreateNotesChanged)
    Q_PROPERTY(bool currentContextIsTrash READ currentContextIsTrash NOTIFY contextStateChanged)
    Q_PROPERTY(bool currentContextIsTag READ currentContextIsTag NOTIFY contextStateChanged)
    Q_PROPERTY(bool currentContextAllowsPinning READ currentContextAllowsPinning NOTIFY contextStateChanged)
    Q_PROPERTY(bool currentNotePinned READ currentNotePinned NOTIFY currentNotePinnedChanged)
    Q_PROPERTY(int initialWindowWidth READ initialWindowWidth CONSTANT)
    Q_PROPERTY(int initialWindowHeight READ initialWindowHeight CONSTANT)
    Q_PROPERTY(int initialWindowX READ initialWindowX CONSTANT)
    Q_PROPERTY(int initialWindowY READ initialWindowY CONSTANT)

public:
    explicit AppBackend(QObject* parent = nullptr);
    ~AppBackend() override;

    [[nodiscard]] QAbstractItemModel* noteModel() const;
    [[nodiscard]] QAbstractItemModel* treeModel() const;
    [[nodiscard]] NoteEditorBackend* noteEditor() const;
    [[nodiscard]] QString platformName() const;
    [[nodiscard]] QString displayFontFamily() const;
    [[nodiscard]] QString editorFontFamily() const;
    [[nodiscard]] int editorFontPointSize() const;
    [[nodiscard]] int textColumnWidth() const;
    [[nodiscard]] bool textFullWidth() const;
    [[nodiscard]] QVariant themeData() const;
    [[nodiscard]] QString listLabel1() const;
    [[nodiscard]] QString listLabel2() const;
    [[nodiscard]] QString searchText() const;
    void setSearchText(const QString& text);
    [[nodiscard]] bool folderTreeCollapsed() const;
    [[nodiscard]] bool noteListCollapsed() const;
    [[nodiscard]] bool kanbanVisible() const;
    [[nodiscard]] bool alwaysOnTop() const;
    [[nodiscard]] bool canCreateNotes() const;
    [[nodiscard]] int initialWindowWidth() const;
    [[nodiscard]] int initialWindowHeight() const;
    [[nodiscard]] int initialWindowX() const;
    [[nodiscard]] int initialWindowY() const;
    [[nodiscard]] bool currentContextIsTrash() const;
    [[nodiscard]] bool currentContextIsTag() const;
    [[nodiscard]] bool currentContextAllowsPinning() const;
    [[nodiscard]] bool currentNotePinned() const;

    Q_INVOKABLE void publishState();
    Q_INVOKABLE void activateTreeItem(int itemType, int nodeId);
    Q_INVOKABLE void selectNoteRow(int row);
    Q_INVOKABLE bool noteListHasPinnedNotes() const;
    Q_INVOKABLE bool noteRowStartsPinnedSection(int row) const;
    Q_INVOKABLE bool noteRowStartsNotesSection(int row) const;
    Q_INVOKABLE void createNewNote();
    Q_INVOKABLE void addNewFolder();
    Q_INVOKABLE void addNewTag();
    Q_INVOKABLE void renameTreeItem(int itemType, int nodeId, const QString& newName);
    Q_INVOKABLE void deleteFolderFromTree(int nodeId);
    Q_INVOKABLE void deleteTagFromTree(int tagId);
    Q_INVOKABLE void moveCurrentNoteToTrash();
    Q_INVOKABLE void restoreCurrentNote();
    Q_INVOKABLE void setCurrentNotePinned(bool isPinned);
    Q_INVOKABLE void changeEditorFontTypeFromStyleButtons(FontTypeface::Value fontTypeface, int chosenFontIndex);
    Q_INVOKABLE void changeEditorFontSizeFromStyleButtons(FontSizeAction::Value fontSizeAction);
    Q_INVOKABLE void changeEditorTextWidthFromStyleButtons(EditorTextWidth::Value editorTextWidth);
    Q_INVOKABLE void resetEditorSettings();
    Q_INVOKABLE void setTheme(Theme::Value theme);
    Q_INVOKABLE void setKanbanVisibility(bool isVisible);
    Q_INVOKABLE void collapseNoteList();
    Q_INVOKABLE void expandNoteList();
    Q_INVOKABLE void collapseFolderTree();
    Q_INVOKABLE void expandFolderTree();
    Q_INVOKABLE void setMarkdownEnabled(bool isMarkdownEnabled);
    Q_INVOKABLE void stayOnTop(bool checked);
    Q_INVOKABLE void showEditorSettings();
    Q_INVOKABLE void setEditorSettingsFromQuickViewVisibility(bool isVisible);
    Q_INVOKABLE void setEditorSettingsScrollBarPosition(double position);
    Q_INVOKABLE void setActivationSuccessful(const QString& licenseKey, bool removeGracePeriodStartedDate = true);
    Q_INVOKABLE void checkProVersion();
    Q_INVOKABLE QVariant getUserLicenseKey() const;
    Q_INVOKABLE void updateWindowMetrics(double width, double height, double x, double y);
    Q_INVOKABLE void fireToggleEditorSettingsShortcut();
    Q_INVOKABLE void importNotes(const QUrl& fileUrl);
    Q_INVOKABLE void restoreNotes(const QUrl& fileUrl);
    Q_INVOKABLE void exportNotesBackup(const QUrl& fileUrl);
    Q_INVOKABLE void exportNotesAsText(const QUrl& dirUrl, const QString& extension);
    Q_INVOKABLE void changeDatabasePath(const QUrl& fileUrl);
    Q_INVOKABLE void clearSearch();

signals:
    void displayFontFamilyChanged();
    void editorFontChanged();
    void editorLayoutChanged();
    void themeDataObjectChanged();
    void listLabelsChanged();
    void searchTextChanged();
    void folderTreeCollapsedChanged();
    void noteListCollapsedChanged();
    void kanbanVisibleChanged();
    void alwaysOnTopChanged();
    void canCreateNotesChanged();
    void contextStateChanged();
    void currentNotePinnedChanged();

    void themeChanged(QVariant theme);
    void platformSet(QVariant platform);
    void qtVersionSet(QVariant qtVersion);
    void editorSettingsShowed(QVariant data);
    void mainWindowResized(QVariant data);
    void mainWindowMoved(QVariant data);
    void displayFontSet(QVariant data);
    void settingsChanged(QVariant data);
    void fontsChanged(QVariant data);
    void toggleEditorSettingsKeyboardShorcutFired();
    void editorSettingsScrollBarPositionChanged(QVariant data);
    void proVersionCheck(QVariant data);
    void subscriptionStatusChanged(QVariant subscriptionStatus);
    void errorOccurred(QVariant data);

    void requestOpenDBManager(const QString& path, bool doCreate);
    void requestMigrateNotesFromV1_5_0(const QString& path);
    void requestNodesTree();
    void requestNotesListInFolder(int parentID, bool isRecursive, bool newNote, int scrollToId);
    void requestNotesListInTags(const QSet<int>& tagIds, bool newNote, int scrollToId);
    void requestSearchInDb(const QString& keyword, const ListViewInfo& inf);
    void requestClearSearchDb(const ListViewInfo& inf);
    void requestRemoveNoteDb(const NodeData& noteData);
    void requestImportNotes(const QString& filePath);
    void requestRestoreNotes(const QString& filePath);
    void requestExportNotes(QString fileName);
    void requestChangeDatabasePath(const QString& newPath);

private slots:
    void loadTreeModel(const NodeTagTreeData& treeData);
    void loadNoteListModel(const QVector<NodeData>& noteList, const ListViewInfo& inf);
    void onChildNotesCountChangedTag(int tagId, int notesCount);
    void onChildNotesCountChangedFolder(int folderId, const QString& absPath, int notesCount);
    void onNoteEditClosed(const NodeData& note, bool selectNext);
    void onDeleteNoteRequested(const NodeData& note);
    void moveNoteToTop(const NodeData& note);
    void setNoteDataInList(const NodeData& note);
    void triggerSearch();

private:
    struct CharsLimitPerFont {
        int mono = 64;
        int serif = 80;
        int sansSerif = 80;
    };

    void setupSettings();
    void setupDatabases();
    void setupModels();
    void setupConnections();
    void startBackend();

    void restoreSelectionFromSettings();
    void openFolderContext(int folderId, bool isRecursive, bool newNote = false, int scrollToId = INVALID_NODE_ID);
    void openTagContext(const QSet<int>& tagIds, bool newNote = false, int scrollToId = INVALID_NODE_ID);
    void reloadCurrentContext(bool newNote = false, int scrollToId = INVALID_NODE_ID);
    void selectNoteIndex(const QModelIndex& index, bool saveSelection = true);
    void updateListViewLabel();
    void updateCurrentFont();
    void saveEditorSettings() const;
    void saveTreeSelection(bool isFolder, const QString& folderPath, const QSet<int>& tagIds) const;
    void saveSelectedNote(int noteId) const;
    void emitDisplayFontState();
    void emitFontState();
    void emitThemeState();
    void emitSettingsState();
    void emitWindowState();
    [[nodiscard]] QJsonObject buildThemeData() const;
    [[nodiscard]] QJsonObject buildFontData() const;
    [[nodiscard]] QJsonObject buildSettingsData() const;
    [[nodiscard]] QString currentThemeName() const;
    [[nodiscard]] int currentCharsLimit() const;
    [[nodiscard]] int currentFolderIdForNewNote() const;

    QSettings* m_settings;
    DBManager* m_dbManager;
    QThread* m_dbThread;
    TagPool* m_tagPool;
    NoteListModel* m_noteModel;
    NodeTreeModel* m_treeModel;
    NoteEditorBackend* m_noteEditor;

    QString m_noteDbPath;
    QString m_oldDbPath;
    QStringList m_listOfSerifFonts;
    QStringList m_listOfSansSerifFonts;
    QStringList m_listOfMonoFonts;
    QString m_platformName;
    QString m_displayFontFamily;
    QString m_currentEditorFontFamily;
    QString m_listLabel1;
    QString m_listLabel2;
    QString m_searchText;
    QString m_userLicenseKey;
    ListViewInfo m_listViewInfo;
    CharsLimitPerFont m_charsLimitPerFont;

    int m_chosenSerifFontIndex;
    int m_chosenSansSerifFontIndex;
    int m_chosenMonoFontIndex;
    int m_editorMediumFontSize;
    int m_textColumnWidth;
    int m_selectedNoteId;
    int m_initialWindowWidth;
    int m_initialWindowHeight;
    int m_initialWindowX;
    int m_initialWindowY;
    int m_parentWindowWidth;
    int m_parentWindowHeight;
    int m_parentWindowX;
    int m_parentWindowY;

    FontTypeface::Value m_currentFontTypeface;
    Theme::Value m_currentTheme;
    SubscriptionStatus::Value m_subscriptionStatus;

    bool m_textFullWidth;
    bool m_folderTreeCollapsed;
    bool m_noteListCollapsed;
    bool m_kanbanVisible;
    bool m_alwaysOnTop;
    bool m_editorSettingsVisible;
    bool m_isProVersionActivated;
    bool m_doCreate;
    bool m_needMigrateFromV1_5_0;
    bool m_hasLoadedInitialSelection;
    bool m_restoreIsSelectingFolder;
    bool m_hasAppliedSavedNoteSelection;

    double m_editorSettingsScrollBarPosition;
    QSet<int> m_restoreTagIds;
    QString m_restoreFolderPath;
    int m_restoreSelectedNoteId;
    QTimer m_searchDebounceTimer;
};

#endif  // APPBACKEND_H
