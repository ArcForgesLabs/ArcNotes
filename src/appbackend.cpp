#include "appbackend.h"

#include <QColor>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFont>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QJsonArray>
#include <QMetaObject>
#include <QRandomGenerator>
#include <QScreen>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <algorithm>

#include "nodepath.h"
#include "nodetreemodel.h"
#include "noteeditorbackend.h"
#include "notelistmodel.h"
#include "tagpool.h"

namespace {
constexpr auto SETTINGS_ORG = "Awesomeness";
constexpr auto SETTINGS_APP = "Settings";
constexpr auto WINDOW_WIDTH_DEFAULT = 1240;
constexpr auto WINDOW_HEIGHT_DEFAULT = 820;

QString defaultDisplayFont() {
#ifdef __APPLE__
    return QFont(QStringLiteral("SF Pro Text")).exactMatch() ? QStringLiteral("SF Pro Text") : QStringLiteral("Roboto");
#elif _WIN32
    return QFont(QStringLiteral("Segoe UI")).exactMatch() ? QStringLiteral("Segoe UI") : QStringLiteral("Roboto");
#else
    return QStringLiteral("Roboto");
#endif
}

QString currentPlatformName() {
#if defined(Q_OS_WINDOWS)
    return QStringLiteral("Windows");
#elif defined(Q_OS_MACOS)
    return QStringLiteral("Apple");
#else
    return QStringLiteral("Unix");
#endif
}

QColor editorTextColorForTheme(Theme::Value theme) {
    switch (theme) {
        case Theme::Dark:
            return QColor(QStringLiteral("#ececec"));
        case Theme::Sepia:
            return QColor(QStringLiteral("#4b3a24"));
        case Theme::Light:
        default:
            return QColor(QStringLiteral("#2f2c26"));
    }
}
}  // namespace

AppBackend::AppBackend(QObject* parent)
    : QObject(parent),
      m_settings(nullptr),
      m_dbManager(nullptr),
      m_dbThread(nullptr),
      m_tagPool(nullptr),
      m_noteModel(nullptr),
      m_treeModel(nullptr),
      m_noteEditor(nullptr),
      m_listOfSerifFonts({QStringLiteral("Trykker"), QStringLiteral("PT Serif"), QStringLiteral("Mate")}),
      m_listOfSansSerifFonts({QStringLiteral("Source Sans Pro"), QStringLiteral("Roboto")}),
      m_listOfMonoFonts({QStringLiteral("iA Writer Mono S"), QStringLiteral("iA Writer Duo S"),
                         QStringLiteral("iA Writer Quattro S")}),
      m_platformName(currentPlatformName()),
      m_displayFontFamily(defaultDisplayFont()),
      m_chosenSerifFontIndex(0),
      m_chosenSansSerifFontIndex(0),
      m_chosenMonoFontIndex(0),
      m_editorMediumFontSize(
#ifdef __APPLE__
          17
#else
          13
#endif
          ),
      m_textColumnWidth(720),
      m_selectedNoteId(INVALID_NODE_ID),
      m_initialWindowWidth(WINDOW_WIDTH_DEFAULT),
      m_initialWindowHeight(WINDOW_HEIGHT_DEFAULT),
      m_initialWindowX(0),
      m_initialWindowY(0),
      m_parentWindowWidth(WINDOW_WIDTH_DEFAULT),
      m_parentWindowHeight(WINDOW_HEIGHT_DEFAULT),
      m_parentWindowX(0),
      m_parentWindowY(0),
      m_currentFontTypeface(FontTypeface::SansSerif),
      m_currentTheme(Theme::Light),
      m_subscriptionStatus(SubscriptionStatus::Active),
      m_textFullWidth(false),
      m_folderTreeCollapsed(false),
      m_noteListCollapsed(false),
      m_kanbanVisible(false),
      m_alwaysOnTop(false),
      m_editorSettingsVisible(false),
      m_isProVersionActivated(false),
      m_doCreate(false),
      m_needMigrateFromV1_5_0(false),
      m_hasLoadedInitialSelection(false),
      m_restoreIsSelectingFolder(true),
      m_hasAppliedSavedNoteSelection(false),
      m_editorSettingsScrollBarPosition(0.0),
      m_restoreSelectedNoteId(INVALID_NODE_ID) {
    setupSettings();
    setupDatabases();
    setupModels();
    setupConnections();
    updateCurrentFont();
    startBackend();
    connect(&m_searchDebounceTimer, &QTimer::timeout, this, &AppBackend::triggerSearch);
    m_searchDebounceTimer.setSingleShot(true);
    m_searchDebounceTimer.setInterval(120);
}

AppBackend::~AppBackend() {
    if (m_dbThread != nullptr) {
        m_dbThread->quit();
        m_dbThread->wait();
    }
}

QAbstractItemModel* AppBackend::noteModel() const {
    return m_noteModel;
}

QAbstractItemModel* AppBackend::treeModel() const {
    return m_treeModel;
}

NoteEditorBackend* AppBackend::noteEditor() const {
    return m_noteEditor;
}

QString AppBackend::platformName() const {
    return m_platformName;
}

QString AppBackend::displayFontFamily() const {
    return m_displayFontFamily;
}

QString AppBackend::editorFontFamily() const {
    return m_currentEditorFontFamily;
}

int AppBackend::editorFontPointSize() const {
    return m_editorMediumFontSize;
}

int AppBackend::textColumnWidth() const {
    return m_textColumnWidth;
}

bool AppBackend::textFullWidth() const {
    return m_textFullWidth;
}

QVariant AppBackend::themeData() const {
    return QVariant(buildThemeData());
}

QString AppBackend::listLabel1() const {
    return m_listLabel1;
}

QString AppBackend::listLabel2() const {
    return m_listLabel2;
}

QString AppBackend::searchText() const {
    return m_searchText;
}

void AppBackend::setSearchText(const QString& text) {
    if (m_searchText == text) {
        return;
    }
    m_searchText = text;
    emit searchTextChanged();
    if (m_searchText.isEmpty()) {
        clearSearch();
        return;
    }
    m_searchDebounceTimer.start();
}

bool AppBackend::folderTreeCollapsed() const {
    return m_folderTreeCollapsed;
}

bool AppBackend::noteListCollapsed() const {
    return m_noteListCollapsed;
}

bool AppBackend::kanbanVisible() const {
    return m_kanbanVisible;
}

bool AppBackend::alwaysOnTop() const {
    return m_alwaysOnTop;
}

bool AppBackend::canCreateNotes() const {
    return !((!m_listViewInfo.isInTag) && (m_listViewInfo.parentFolderId == TRASH_FOLDER_ID));
}

int AppBackend::initialWindowWidth() const {
    return m_initialWindowWidth;
}

int AppBackend::initialWindowHeight() const {
    return m_initialWindowHeight;
}

int AppBackend::initialWindowX() const {
    return m_initialWindowX;
}

int AppBackend::initialWindowY() const {
    return m_initialWindowY;
}

bool AppBackend::currentContextIsTrash() const {
    return !m_listViewInfo.isInTag && m_listViewInfo.parentFolderId == TRASH_FOLDER_ID;
}

bool AppBackend::currentContextIsTag() const {
    return m_listViewInfo.isInTag;
}

bool AppBackend::currentContextAllowsPinning() const {
    return !m_listViewInfo.isInTag && m_listViewInfo.parentFolderId != TRASH_FOLDER_ID;
}

bool AppBackend::currentNotePinned() const {
    if (m_noteModel == nullptr || m_noteEditor == nullptr) {
        return false;
    }

    const QModelIndex noteIndex = m_noteModel->getNoteIndex(m_noteEditor->currentEditingNoteId());
    if (!noteIndex.isValid()) {
        return false;
    }

    return noteIndex.data(NoteListModel::NoteIsPinned).toBool();
}

void AppBackend::publishState() {
    emitDisplayFontState();
    emitFontState();
    emitThemeState();
    emitSettingsState();
    emitWindowState();
    emit editorSettingsScrollBarPositionChanged(QVariant(m_editorSettingsScrollBarPosition));
    checkProVersion();
}

void AppBackend::activateTreeItem(int itemType, int nodeId) {
    const auto type = static_cast<NodeItem::Type>(itemType);
    switch (type) {
        case NodeItem::AllNoteButton:
            saveTreeSelection(true, NodePath::getAllNoteFolderPath(), {});
            openFolderContext(ROOT_FOLDER_ID, true);
            break;
        case NodeItem::TrashButton:
            saveTreeSelection(true, NodePath::getTrashFolderPath(), {});
            openFolderContext(TRASH_FOLDER_ID, true);
            break;
        case NodeItem::FolderItem: {
            NodeData folder;
            QMetaObject::invokeMethod(m_dbManager, "getNode", Qt::BlockingQueuedConnection,
                                      Q_RETURN_ARG(NodeData, folder), Q_ARG(int, nodeId));
            saveTreeSelection(true, folder.absolutePath(), {});
            openFolderContext(nodeId, false);
            break;
        }
        case NodeItem::TagItem:
            saveTreeSelection(false, QString(), {nodeId});
            openTagContext({nodeId});
            break;
        default:
            break;
    }
}

void AppBackend::selectNoteRow(int row) {
    if (m_noteModel == nullptr) {
        return;
    }
    selectNoteIndex(m_noteModel->index(row, 0));
}

bool AppBackend::noteListHasPinnedNotes() const {
    return m_noteModel != nullptr && m_noteModel->hasPinnedNote();
}

bool AppBackend::noteRowStartsPinnedSection(int row) const {
    if (m_noteModel == nullptr || row < 0) {
        return false;
    }
    return m_noteModel->isFirstPinnedNote(m_noteModel->index(row, 0));
}

bool AppBackend::noteRowStartsNotesSection(int row) const {
    if (m_noteModel == nullptr || row < 0) {
        return false;
    }
    return m_noteModel->isFirstUnpinnedNote(m_noteModel->index(row, 0));
}

void AppBackend::createNewNote() {
    if (!canCreateNotes() || m_noteModel == nullptr) {
        return;
    }

    if (!m_noteEditor->isTempNote()) {
        NodeData tmpNote;
        tmpNote.setNodeType(NodeData::Type::Note);
        const auto noteDate = QDateTime::currentDateTime();
        tmpNote.setCreationDateTime(noteDate);
        tmpNote.setLastModificationDateTime(noteDate);
        tmpNote.setFullTitle(QStringLiteral("New Note"));

        const int folderId = currentFolderIdForNewNote();
        NodeData parent;
        QMetaObject::invokeMethod(m_dbManager, "getNode", Qt::BlockingQueuedConnection, Q_RETURN_ARG(NodeData, parent),
                                  Q_ARG(int, folderId));
        tmpNote.setParentId(parent.nodeType() == NodeData::Type::Folder ? parent.id() : DEFAULT_NOTES_FOLDER_ID);
        tmpNote.setParentName(parent.nodeType() == NodeData::Type::Folder ? parent.fullTitle()
                                                                          : QStringLiteral("Notes"));

        int noteId = INVALID_NODE_ID;
        QMetaObject::invokeMethod(m_dbManager, "nextAvailableNodeId", Qt::BlockingQueuedConnection,
                                  Q_RETURN_ARG(int, noteId));
        tmpNote.setId(noteId);
        tmpNote.setIsTempNote(true);
        if (m_listViewInfo.isInTag) {
            tmpNote.setTagIds(m_listViewInfo.currentTagList);
        }

        const QModelIndex newNoteIndex = m_noteModel->insertNote(tmpNote, 0);
        m_noteEditor->setForcedReadOnly(false);
        m_noteEditor->showNotesInEditor({tmpNote});
        selectNoteIndex(newNoteIndex);
        return;
    }

    selectNoteIndex(m_noteModel->getNoteIndex(m_noteEditor->currentEditingNoteId()));
}

void AppBackend::addNewFolder() {
    if (m_treeModel == nullptr || m_dbManager == nullptr) {
        return;
    }

    const QModelIndex rootIdx = m_treeModel->rootIndex();
    const QString folderName = m_treeModel->getNewFolderPlaceholderName(rootIdx);

    NodeData newFolder;
    newFolder.setNodeType(NodeData::Type::Folder);
    const auto currentDate = QDateTime::currentDateTime();
    newFolder.setCreationDateTime(currentDate);
    newFolder.setLastModificationDateTime(currentDate);
    newFolder.setFullTitle(folderName);
    newFolder.setParentId(ROOT_FOLDER_ID);

    int newNodeId = INVALID_NODE_ID;
    QMetaObject::invokeMethod(m_dbManager, "addNode", Qt::BlockingQueuedConnection, Q_RETURN_ARG(int, newNodeId),
                              Q_ARG(NodeData, newFolder));
    if (newNodeId <= 0) {
        return;
    }

    NodeData createdFolder;
    QMetaObject::invokeMethod(m_dbManager, "getNode", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(NodeData, createdFolder), Q_ARG(int, newNodeId));

    QHash<NodeItem::Roles, QVariant> data;
    data[NodeItem::Roles::ItemType] = NodeItem::Type::FolderItem;
    data[NodeItem::Roles::DisplayText] = createdFolder.fullTitle();
    data[NodeItem::Roles::NodeId] = createdFolder.id();
    data[NodeItem::Roles::AbsPath] = createdFolder.absolutePath();
    data[NodeItem::Roles::RelPos] = createdFolder.relativePosition();
    data[NodeItem::Roles::ChildCount] = 0;

    m_treeModel->appendChildNodeToParent(rootIdx, data);
}

void AppBackend::addNewTag() {
    if (m_treeModel == nullptr || m_dbManager == nullptr) {
        return;
    }

    static const QStringList tagColorPalette = {
        QStringLiteral("#448ac9"), QStringLiteral("#e74c3c"), QStringLiteral("#2ecc71"), QStringLiteral("#f39c12"),
        QStringLiteral("#9b59b6"), QStringLiteral("#1abc9c"), QStringLiteral("#e67e22"), QStringLiteral("#3498db"),
        QStringLiteral("#e91e63"), QStringLiteral("#00bcd4"),
    };

    const QString tagName = m_treeModel->getNewTagPlaceholderName();
    const QString& tagColor = tagColorPalette.at(QRandomGenerator::global()->bounded(tagColorPalette.size()));

    TagData newTag;
    newTag.setName(tagName);
    newTag.setColor(tagColor);

    int newTagId = INVALID_NODE_ID;
    QMetaObject::invokeMethod(m_dbManager, "addTag", Qt::BlockingQueuedConnection, Q_RETURN_ARG(int, newTagId),
                              Q_ARG(TagData, newTag));
    if (newTagId <= 0) {
        return;
    }

    const QModelIndex rootIdx = m_treeModel->rootIndex();
    QHash<NodeItem::Roles, QVariant> data;
    data[NodeItem::Roles::ItemType] = NodeItem::Type::TagItem;
    data[NodeItem::Roles::DisplayText] = tagName;
    data[NodeItem::Roles::NodeId] = newTagId;
    data[NodeItem::Roles::TagColor] = tagColor;
    data[NodeItem::Roles::RelPos] = 0;
    data[NodeItem::Roles::ChildCount] = 0;

    m_treeModel->appendChildNodeToParent(rootIdx, data);
}

void AppBackend::renameTreeItem(int itemType, int nodeId, const QString& newName) {
    if (m_treeModel == nullptr || m_dbManager == nullptr || newName.trimmed().isEmpty()) {
        return;
    }

    const auto type = static_cast<NodeItem::Type>(itemType);

    if (type == NodeItem::FolderItem) {
        QMetaObject::invokeMethod(m_dbManager, "renameNode", Qt::BlockingQueuedConnection, Q_ARG(int, nodeId),
                                  Q_ARG(QString, newName.trimmed()));
        NodeData folderNode;
        QMetaObject::invokeMethod(m_dbManager, "getNode", Qt::BlockingQueuedConnection,
                                  Q_RETURN_ARG(NodeData, folderNode), Q_ARG(int, nodeId));
        const auto folderIndex = m_treeModel->folderIndexFromIdPath(folderNode.absolutePath());
        if (folderIndex.isValid()) {
            m_treeModel->setData(folderIndex, newName.trimmed(), NodeItem::Roles::DisplayText);
        }
    } else if (type == NodeItem::TagItem) {
        QMetaObject::invokeMethod(m_dbManager, "renameTag", Qt::BlockingQueuedConnection, Q_ARG(int, nodeId),
                                  Q_ARG(QString, newName.trimmed()));
        const auto tagIndex = m_treeModel->tagIndexFromId(nodeId);
        if (tagIndex.isValid()) {
            m_treeModel->setData(tagIndex, newName.trimmed(), NodeItem::Roles::DisplayText);
        }
    }
}

void AppBackend::deleteFolderFromTree(int nodeId) {
    if (m_treeModel == nullptr || m_dbManager == nullptr || nodeId <= DEFAULT_NOTES_FOLDER_ID) {
        return;
    }

    NodeData folderNode;
    QMetaObject::invokeMethod(m_dbManager, "getNode", Qt::BlockingQueuedConnection, Q_RETURN_ARG(NodeData, folderNode),
                              Q_ARG(int, nodeId));
    if (folderNode.id() == INVALID_NODE_ID) {
        return;
    }

    const auto folderIndex = m_treeModel->folderIndexFromIdPath(folderNode.absolutePath());
    if (!folderIndex.isValid()) {
        return;
    }

    QMetaObject::invokeMethod(m_dbManager, "moveFolderToTrash", Qt::BlockingQueuedConnection,
                              Q_ARG(NodeData, folderNode));

    const auto parentIndex = m_treeModel->parent(folderIndex);
    m_treeModel->deleteRow(folderIndex, parentIndex.isValid() ? parentIndex : m_treeModel->rootIndex());

    // Reload current context and refresh tree counts
    emit requestNodesTree();
    reloadCurrentContext();
}

void AppBackend::deleteTagFromTree(int tagId) {
    if (m_treeModel == nullptr || m_dbManager == nullptr) {
        return;
    }

    const auto tagIndex = m_treeModel->tagIndexFromId(tagId);
    if (!tagIndex.isValid()) {
        return;
    }

    QMetaObject::invokeMethod(m_dbManager, "removeTag", Qt::BlockingQueuedConnection, Q_ARG(int, tagId));
    m_treeModel->deleteRow(tagIndex, m_treeModel->rootIndex());
}

void AppBackend::moveCurrentNoteToTrash() {
    m_noteEditor->deleteCurrentNote();
}

void AppBackend::restoreCurrentNote() {
    if (m_noteModel == nullptr || m_noteEditor == nullptr) {
        return;
    }

    const int noteId = m_noteEditor->currentEditingNoteId();
    if (noteId == INVALID_NODE_ID) {
        return;
    }

    NodeData note;
    QMetaObject::invokeMethod(m_dbManager, "getNode", Qt::BlockingQueuedConnection, Q_RETURN_ARG(NodeData, note),
                              Q_ARG(int, noteId));
    if (note.id() == INVALID_NODE_ID || note.parentId() != TRASH_FOLDER_ID) {
        return;
    }

    const QModelIndex noteIndex = m_noteModel->getNoteIndex(note.id());
    const int removedRow = noteIndex.isValid() ? noteIndex.row() : -1;
    if (noteIndex.isValid()) {
        m_noteModel->removeNotes({noteIndex});
    }

    NodeData defaultNotesFolder;
    QMetaObject::invokeMethod(m_dbManager, "getNode", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(NodeData, defaultNotesFolder), Q_ARG(int, DEFAULT_NOTES_FOLDER_ID));
    QMetaObject::invokeMethod(m_dbManager, "moveNode", Qt::BlockingQueuedConnection, Q_ARG(int, note.id()),
                              Q_ARG(NodeData, defaultNotesFolder));

    if (m_noteModel->rowCount() == 0) {
        m_selectedNoteId = INVALID_NODE_ID;
        saveSelectedNote(INVALID_NODE_ID);
        m_noteEditor->closeEditor();
        emit currentNotePinnedChanged();
        reloadCurrentContext(false, INVALID_NODE_ID);
        return;
    }

    if (removedRow >= 0) {
        const int nextRow = std::min(removedRow, m_noteModel->rowCount() - 1);
        selectNoteIndex(m_noteModel->index(nextRow, 0));
    }
    reloadCurrentContext(false, m_selectedNoteId);
}

void AppBackend::setCurrentNotePinned(bool isPinned) {
    if (m_noteModel == nullptr || m_noteEditor == nullptr) {
        return;
    }

    const QModelIndex noteIndex = m_noteModel->getNoteIndex(m_noteEditor->currentEditingNoteId());
    if (!noteIndex.isValid()) {
        return;
    }

    m_noteModel->setNotesIsPinned({noteIndex}, isPinned);
    emit currentNotePinnedChanged();
}

void AppBackend::changeEditorFontTypeFromStyleButtons(FontTypeface::Value fontTypeface, int chosenFontIndex) {
    if (chosenFontIndex < 0) {
        return;
    }

    switch (fontTypeface) {
        case FontTypeface::Mono:
            if (chosenFontIndex >= m_listOfMonoFonts.size()) return;
            m_chosenMonoFontIndex = chosenFontIndex;
            break;
        case FontTypeface::Serif:
            if (chosenFontIndex >= m_listOfSerifFonts.size()) return;
            m_chosenSerifFontIndex = chosenFontIndex;
            break;
        case FontTypeface::SansSerif:
            if (chosenFontIndex >= m_listOfSansSerifFonts.size()) return;
            m_chosenSansSerifFontIndex = chosenFontIndex;
            break;
    }

    m_currentFontTypeface = fontTypeface;
    updateCurrentFont();
    saveEditorSettings();
    emitFontState();
    emitSettingsState();
}

void AppBackend::changeEditorFontSizeFromStyleButtons(FontSizeAction::Value fontSizeAction) {
    switch (fontSizeAction) {
        case FontSizeAction::FontSizeIncrease:
            ++m_editorMediumFontSize;
            break;
        case FontSizeAction::FontSizeDecrease:
            m_editorMediumFontSize = std::max(9, m_editorMediumFontSize - 1);
            break;
    }

    updateCurrentFont();
    saveEditorSettings();
    emitFontState();
    emitSettingsState();
}

void AppBackend::changeEditorTextWidthFromStyleButtons(EditorTextWidth::Value editorTextWidth) {
    switch (editorTextWidth) {
        case EditorTextWidth::TextWidthFullWidth:
            m_textFullWidth = !m_textFullWidth;
            break;
        case EditorTextWidth::TextWidthIncrease:
            m_textFullWidth = false;
            switch (m_currentFontTypeface) {
                case FontTypeface::Mono:
                    ++m_charsLimitPerFont.mono;
                    break;
                case FontTypeface::Serif:
                    ++m_charsLimitPerFont.serif;
                    break;
                case FontTypeface::SansSerif:
                    ++m_charsLimitPerFont.sansSerif;
                    break;
            }
            break;
        case EditorTextWidth::TextWidthDecrease:
            m_textFullWidth = false;
            switch (m_currentFontTypeface) {
                case FontTypeface::Mono:
                    m_charsLimitPerFont.mono = std::max(30, m_charsLimitPerFont.mono - 1);
                    break;
                case FontTypeface::Serif:
                    m_charsLimitPerFont.serif = std::max(40, m_charsLimitPerFont.serif - 1);
                    break;
                case FontTypeface::SansSerif:
                    m_charsLimitPerFont.sansSerif = std::max(40, m_charsLimitPerFont.sansSerif - 1);
                    break;
            }
            break;
    }

    updateCurrentFont();
    saveEditorSettings();
    emit editorLayoutChanged();
    emitSettingsState();
}

void AppBackend::resetEditorSettings() {
    m_currentFontTypeface = FontTypeface::SansSerif;
    m_chosenMonoFontIndex = 0;
    m_chosenSerifFontIndex = 0;
    m_chosenSansSerifFontIndex = 0;
#ifdef __APPLE__
    m_editorMediumFontSize = 17;
#else
    m_editorMediumFontSize = 13;
#endif
    m_charsLimitPerFont = {};
    m_textFullWidth = false;
    m_currentTheme = Theme::Light;
    m_folderTreeCollapsed = false;
    m_noteListCollapsed = false;
    m_alwaysOnTop = false;
    m_kanbanVisible = false;
    m_noteEditor->setMarkdownEnabled(true);

    updateCurrentFont();
    saveEditorSettings();
    emitFontState();
    emitThemeState();
    emitSettingsState();
    emit editorLayoutChanged();
    emit folderTreeCollapsedChanged();
    emit noteListCollapsedChanged();
    emit alwaysOnTopChanged();
    emit kanbanVisibleChanged();
}

void AppBackend::setTheme(Theme::Value theme) {
    if (m_currentTheme == theme) {
        return;
    }
    m_currentTheme = theme;
    m_noteEditor->updateHighlightingTheme(m_currentTheme, editorTextColorForTheme(m_currentTheme),
                                          m_editorMediumFontSize);
    saveEditorSettings();
    emitThemeState();
    emitSettingsState();
}

void AppBackend::setKanbanVisibility(bool isVisible) {
    const bool nextVisible = isVisible && canCreateNotes();
    if (m_kanbanVisible == nextVisible) {
        return;
    }
    m_kanbanVisible = nextVisible;
    saveEditorSettings();
    if (m_kanbanVisible) {
        m_noteEditor->showKanbanView();
    } else {
        m_noteEditor->showTextView();
    }
    emit kanbanVisibleChanged();
    emitSettingsState();
}

void AppBackend::collapseNoteList() {
    if (m_noteListCollapsed) {
        return;
    }
    m_noteListCollapsed = true;
    saveEditorSettings();
    emit noteListCollapsedChanged();
    emitSettingsState();
}

void AppBackend::expandNoteList() {
    if (!m_noteListCollapsed) {
        return;
    }
    m_noteListCollapsed = false;
    saveEditorSettings();
    emit noteListCollapsedChanged();
    emitSettingsState();
}

void AppBackend::collapseFolderTree() {
    if (m_folderTreeCollapsed) {
        return;
    }
    m_folderTreeCollapsed = true;
    saveEditorSettings();
    emit folderTreeCollapsedChanged();
    emitSettingsState();
}

void AppBackend::expandFolderTree() {
    if (!m_folderTreeCollapsed) {
        return;
    }
    m_folderTreeCollapsed = false;
    saveEditorSettings();
    emit folderTreeCollapsedChanged();
    emitSettingsState();
}

void AppBackend::setMarkdownEnabled(bool isMarkdownEnabled) {
    m_noteEditor->setMarkdownEnabled(isMarkdownEnabled);
    saveEditorSettings();
    emitSettingsState();
}

void AppBackend::stayOnTop(bool checked) {
    if (m_alwaysOnTop == checked) {
        return;
    }
    m_alwaysOnTop = checked;
    saveEditorSettings();
    emit alwaysOnTopChanged();
    emitSettingsState();
}

void AppBackend::showEditorSettings() {
    m_editorSettingsVisible = true;
    QJsonObject dataToSendToView{{"parentWindowHeight", m_parentWindowHeight},
                                 {"parentWindowWidth", m_parentWindowWidth},
                                 {"parentWindowX", m_parentWindowX},
                                 {"parentWindowY", m_parentWindowY}};
    emit editorSettingsShowed(QVariant(dataToSendToView));
}

void AppBackend::setEditorSettingsFromQuickViewVisibility(bool isVisible) {
    m_editorSettingsVisible = isVisible;
}

void AppBackend::setEditorSettingsScrollBarPosition(double position) {
    m_editorSettingsScrollBarPosition = position;
    saveEditorSettings();
    emit editorSettingsScrollBarPositionChanged(QVariant(position));
}

void AppBackend::setActivationSuccessful(const QString& licenseKey, bool removeGracePeriodStartedDate) {
    Q_UNUSED(removeGracePeriodStartedDate)
    m_userLicenseKey = licenseKey;
    checkProVersion();
}

void AppBackend::checkProVersion() {
    m_isProVersionActivated = true;
    m_subscriptionStatus = SubscriptionStatus::Active;
    emit proVersionCheck(QVariant(m_isProVersionActivated));
    emit subscriptionStatusChanged(QVariant(m_subscriptionStatus));
}

QVariant AppBackend::getUserLicenseKey() const {
    return QVariant(m_userLicenseKey);
}

void AppBackend::updateWindowMetrics(double width, double height, double x, double y) {
    const int newWidth = qRound(width);
    const int newHeight = qRound(height);
    const int newX = qRound(x);
    const int newY = qRound(y);

    if (m_parentWindowWidth != newWidth || m_parentWindowHeight != newHeight) {
        m_parentWindowWidth = newWidth;
        m_parentWindowHeight = newHeight;
        m_settings->setValue(QStringLiteral("windowWidth"), newWidth);
        m_settings->setValue(QStringLiteral("windowHeight"), newHeight);
        QJsonObject dataToSendToView{{"parentWindowHeight", m_parentWindowHeight},
                                     {"parentWindowWidth", m_parentWindowWidth}};
        emit mainWindowResized(QVariant(dataToSendToView));
    }

    if (m_parentWindowX != newX || m_parentWindowY != newY) {
        m_parentWindowX = newX;
        m_parentWindowY = newY;
        m_settings->setValue(QStringLiteral("windowX"), newX);
        m_settings->setValue(QStringLiteral("windowY"), newY);
        QJsonObject dataToSendToView{{"parentWindowX", m_parentWindowX}, {"parentWindowY", m_parentWindowY}};
        emit mainWindowMoved(QVariant(dataToSendToView));
    }
}

void AppBackend::fireToggleEditorSettingsShortcut() {
    emit toggleEditorSettingsKeyboardShorcutFired();
}

void AppBackend::importNotes(const QUrl& fileUrl) {
    const QString filePath = fileUrl.toLocalFile();
    if (!filePath.isEmpty()) {
        emit requestImportNotes(filePath);
    }
}

void AppBackend::restoreNotes(const QUrl& fileUrl) {
    const QString filePath = fileUrl.toLocalFile();
    if (!filePath.isEmpty()) {
        emit requestRestoreNotes(filePath);
    }
}

void AppBackend::exportNotesBackup(const QUrl& fileUrl) {
    const QString filePath = fileUrl.toLocalFile();
    if (!filePath.isEmpty()) {
        emit requestExportNotes(filePath);
    }
}

void AppBackend::exportNotesAsText(const QUrl& dirUrl, const QString& extension) {
    const QString dirPath = dirUrl.toLocalFile();
    if (!dirPath.isEmpty()) {
        QMetaObject::invokeMethod(
            m_dbManager, [this, dirPath, extension]() { m_dbManager->exportNotes(dirPath, extension); },
            Qt::QueuedConnection);
    }
}

void AppBackend::changeDatabasePath(const QUrl& fileUrl) {
    const QString filePath = fileUrl.toLocalFile();
    if (filePath.isEmpty()) {
        return;
    }
    m_noteDbPath = filePath;
    m_settings->setValue(QStringLiteral("noteDBFilePath"), filePath);
    emit requestChangeDatabasePath(filePath);
    emit requestNodesTree();
    reloadCurrentContext();
}

void AppBackend::clearSearch() {
    if (m_searchText.isEmpty() && !m_listViewInfo.isInSearch) {
        return;
    }
    if (!m_searchText.isEmpty()) {
        m_searchText.clear();
        emit searchTextChanged();
    }
    m_listViewInfo.needCreateNewNote = false;
    m_listViewInfo.scrollToId = INVALID_NODE_ID;
    emit requestClearSearchDb(m_listViewInfo);
}

void AppBackend::loadTreeModel(const NodeTagTreeData& treeData) {
    m_treeModel->setTreeData(treeData);

    NodeData allNotesCountNode;
    QMetaObject::invokeMethod(m_dbManager, "getChildNotesCountFolder", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(NodeData, allNotesCountNode), Q_ARG(int, ROOT_FOLDER_ID));
    const auto allNotesIndex = m_treeModel->getAllNotesButtonIndex();
    if (allNotesIndex.isValid()) {
        m_treeModel->setData(allNotesIndex, allNotesCountNode.childNotesCount(), NodeItem::Roles::ChildCount);
    }

    NodeData trashCountNode;
    QMetaObject::invokeMethod(m_dbManager, "getChildNotesCountFolder", Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(NodeData, trashCountNode), Q_ARG(int, TRASH_FOLDER_ID));
    const auto trashIndex = m_treeModel->getTrashButtonIndex();
    if (trashIndex.isValid()) {
        m_treeModel->setData(trashIndex, trashCountNode.childNotesCount(), NodeItem::Roles::ChildCount);
    }

    restoreSelectionFromSettings();
}

void AppBackend::loadNoteListModel(const QVector<NodeData>& noteList, const ListViewInfo& inf) {
    const int previousSelectedNoteId = m_selectedNoteId;
    m_listViewInfo = inf;
    m_noteModel->setListNote(noteList, m_listViewInfo);
    updateListViewLabel();
    emit canCreateNotesChanged();
    emit contextStateChanged();

    if (m_listViewInfo.needCreateNewNote) {
        m_listViewInfo.needCreateNewNote = false;
        createNewNote();
        return;
    }

    QModelIndex targetIndex;
    if (!m_listViewInfo.isInSearch && m_listViewInfo.scrollToId != INVALID_NODE_ID) {
        targetIndex = m_noteModel->getNoteIndex(m_listViewInfo.scrollToId);
    }
    if (!targetIndex.isValid() && previousSelectedNoteId != INVALID_NODE_ID) {
        targetIndex = m_noteModel->getNoteIndex(previousSelectedNoteId);
    }
    if (!targetIndex.isValid() && !m_hasAppliedSavedNoteSelection && m_restoreSelectedNoteId != INVALID_NODE_ID) {
        targetIndex = m_noteModel->getNoteIndex(m_restoreSelectedNoteId);
        m_hasAppliedSavedNoteSelection = true;
    }
    if (!targetIndex.isValid() && m_noteModel->rowCount() > 0) {
        targetIndex = m_noteModel->index(0, 0);
    }

    selectNoteIndex(targetIndex);
    emit currentNotePinnedChanged();
}

void AppBackend::onChildNotesCountChangedTag(int tagId, int notesCount) {
    const auto index = m_treeModel->tagIndexFromId(tagId);
    if (index.isValid()) {
        m_treeModel->setData(index, notesCount, NodeItem::Roles::ChildCount);
    }
}

void AppBackend::onChildNotesCountChangedFolder(int folderId, const QString& absPath, int notesCount) {
    if (folderId == ROOT_FOLDER_ID) {
        const auto index = m_treeModel->getAllNotesButtonIndex();
        if (index.isValid()) {
            m_treeModel->setData(index, notesCount, NodeItem::Roles::ChildCount);
        }
        return;
    }

    if (folderId == TRASH_FOLDER_ID) {
        const auto index = m_treeModel->getTrashButtonIndex();
        if (index.isValid()) {
            m_treeModel->setData(index, notesCount, NodeItem::Roles::ChildCount);
        }
        return;
    }

    const auto index = m_treeModel->folderIndexFromIdPath(absPath);
    if (index.isValid()) {
        m_treeModel->setData(index, notesCount, NodeItem::Roles::ChildCount);
    }
}

void AppBackend::onNoteEditClosed(const NodeData& note, bool selectNext) {
    if (!note.isTempNote()) {
        return;
    }
    const QModelIndex noteIndex = m_noteModel->getNoteIndex(note.id());
    if (!noteIndex.isValid()) {
        return;
    }

    const int removedRow = noteIndex.row();
    m_noteModel->removeNotes({noteIndex});
    if (!selectNext) {
        return;
    }

    if (m_noteModel->rowCount() == 0) {
        m_selectedNoteId = INVALID_NODE_ID;
        saveSelectedNote(INVALID_NODE_ID);
        emit currentNotePinnedChanged();
        return;
    }

    const int nextRow = std::min(removedRow, m_noteModel->rowCount() - 1);
    selectNoteIndex(m_noteModel->index(nextRow, 0));
}

void AppBackend::onDeleteNoteRequested(const NodeData& note) {
    const QModelIndex noteIndex = m_noteModel->getNoteIndex(note.id());
    if (!noteIndex.isValid()) {
        QMetaObject::invokeMethod(m_dbManager, "removeNote", Qt::BlockingQueuedConnection, Q_ARG(NodeData, note));
        reloadCurrentContext(false, INVALID_NODE_ID);
        return;
    }

    const int removedRow = noteIndex.row();
    m_noteModel->removeNotes({noteIndex});
    QMetaObject::invokeMethod(m_dbManager, "removeNote", Qt::BlockingQueuedConnection, Q_ARG(NodeData, note));

    if (m_noteModel->rowCount() == 0) {
        m_selectedNoteId = INVALID_NODE_ID;
        saveSelectedNote(INVALID_NODE_ID);
        m_noteEditor->closeEditor();
        emit currentNotePinnedChanged();
        reloadCurrentContext(false, INVALID_NODE_ID);
        return;
    }

    const int nextRow = std::min(removedRow, m_noteModel->rowCount() - 1);
    selectNoteIndex(m_noteModel->index(nextRow, 0));
    reloadCurrentContext(false, m_selectedNoteId);
}

void AppBackend::moveNoteToTop(const NodeData& note) {
    const QModelIndex noteIndex = m_noteModel->getNoteIndex(note.id());
    if (!noteIndex.isValid()) {
        return;
    }

    QModelIndex destinationIndex;
    if (note.isPinnedNote()) {
        destinationIndex = m_noteModel->index(0, 0);
    } else {
        destinationIndex = m_noteModel->index(m_noteModel->getFirstUnpinnedNote().row(), 0);
    }

    if (!destinationIndex.isValid() || destinationIndex == noteIndex) {
        return;
    }

    m_noteModel->moveRow(noteIndex, noteIndex.row(), destinationIndex, destinationIndex.row());
    m_selectedNoteId = note.id();
    saveSelectedNote(m_selectedNoteId);
}

void AppBackend::setNoteDataInList(const NodeData& note) {
    const QModelIndex noteIndex = m_noteModel->getNoteIndex(note.id());
    if (!noteIndex.isValid()) {
        return;
    }

    const bool wasTemp = noteIndex.data(NoteListModel::NoteIsTemp).toBool();
    QMap<int, QVariant> dataValue;
    dataValue[NoteListModel::NoteContent] = QVariant::fromValue(note.content());
    dataValue[NoteListModel::NoteFullTitle] = QVariant::fromValue(note.fullTitle());
    dataValue[NoteListModel::NoteLastModificationDateTime] = QVariant::fromValue(note.lastModificationdateTime());
    dataValue[NoteListModel::NoteIsTemp] = QVariant::fromValue(note.isTempNote());
    dataValue[NoteListModel::NoteScrollbarPos] = QVariant::fromValue(note.scrollBarPosition());
    m_noteModel->setItemData(noteIndex, dataValue);

    if (wasTemp) {
        saveSelectedNote(note.id());
    }
}

void AppBackend::triggerSearch() {
    if (m_searchText.isEmpty()) {
        clearSearch();
        return;
    }

    if (!m_listViewInfo.isInSearch) {
        m_listViewInfo.currentNotesId = {m_selectedNoteId == INVALID_NODE_ID ? INVALID_NODE_ID : m_selectedNoteId};
    }

    emit requestSearchInDb(m_searchText, m_listViewInfo);
}

void AppBackend::setupSettings() {
    m_settings = new QSettings(QSettings::IniFormat, QSettings::UserScope, QString::fromLatin1(SETTINGS_ORG),
                               QString::fromLatin1(SETTINGS_APP), this);
    m_settings->setFallbacksEnabled(false);

    if (m_settings->value(QStringLiteral("version"), QStringLiteral("NULL")).toString() == QStringLiteral("NULL")) {
        m_settings->setValue(QStringLiteral("version"), qApp->applicationVersion());
    }

    const auto screen = QGuiApplication::primaryScreen();
    const QRect screenGeometry = screen != nullptr ? screen->availableGeometry() : QRect(0, 0, 1600, 900);
    m_initialWindowWidth = m_settings->value(QStringLiteral("windowWidth"), WINDOW_WIDTH_DEFAULT).toInt();
    m_initialWindowHeight = m_settings->value(QStringLiteral("windowHeight"), WINDOW_HEIGHT_DEFAULT).toInt();
    m_initialWindowX =
        m_settings->value(QStringLiteral("windowX"), screenGeometry.center().x() - (m_initialWindowWidth / 2)).toInt();
    m_initialWindowY =
        m_settings->value(QStringLiteral("windowY"), screenGeometry.center().y() - (m_initialWindowHeight / 2)).toInt();
    m_parentWindowWidth = m_initialWindowWidth;
    m_parentWindowHeight = m_initialWindowHeight;
    m_parentWindowX = m_initialWindowX;
    m_parentWindowY = m_initialWindowY;

    const QString selectedFontTypefaceFromDatabase =
        m_settings->value(QStringLiteral("selectedFontTypeface"), QStringLiteral("SansSerif")).toString();
    if (selectedFontTypefaceFromDatabase == QStringLiteral("Mono")) {
        m_currentFontTypeface = FontTypeface::Mono;
    } else if (selectedFontTypefaceFromDatabase == QStringLiteral("Serif")) {
        m_currentFontTypeface = FontTypeface::Serif;
    } else {
        m_currentFontTypeface = FontTypeface::SansSerif;
    }

    m_editorMediumFontSize = m_settings->value(QStringLiteral("editorMediumFontSize"), m_editorMediumFontSize).toInt();
    m_textFullWidth = m_settings->value(QStringLiteral("isTextFullWidth"), false).toBool();
    m_charsLimitPerFont.mono =
        m_settings->value(QStringLiteral("charsLimitPerFontMono"), m_charsLimitPerFont.mono).toInt();
    m_charsLimitPerFont.serif =
        m_settings->value(QStringLiteral("charsLimitPerFontSerif"), m_charsLimitPerFont.serif).toInt();
    m_charsLimitPerFont.sansSerif =
        m_settings->value(QStringLiteral("charsLimitPerFontSansSerif"), m_charsLimitPerFont.sansSerif).toInt();

    const QString chosenMonoFont = m_settings->value(QStringLiteral("chosenMonoFont"), QString()).toString();
    if (!chosenMonoFont.isEmpty()) {
        const auto fontIndex = m_listOfMonoFonts.indexOf(chosenMonoFont);
        if (fontIndex >= 0) {
            m_chosenMonoFontIndex = fontIndex;
        }
    }
    const QString chosenSerifFont = m_settings->value(QStringLiteral("chosenSerifFont"), QString()).toString();
    if (!chosenSerifFont.isEmpty()) {
        const auto fontIndex = m_listOfSerifFonts.indexOf(chosenSerifFont);
        if (fontIndex >= 0) {
            m_chosenSerifFontIndex = fontIndex;
        }
    }
    const QString chosenSansFont = m_settings->value(QStringLiteral("chosenSansSerifFont"), QString()).toString();
    if (!chosenSansFont.isEmpty()) {
        const auto fontIndex = m_listOfSansSerifFonts.indexOf(chosenSansFont);
        if (fontIndex >= 0) {
            m_chosenSansSerifFontIndex = fontIndex;
        }
    }

    const QString chosenTheme = m_settings->value(QStringLiteral("theme"), QStringLiteral("Light")).toString();
    if (chosenTheme == QStringLiteral("Dark")) {
        m_currentTheme = Theme::Dark;
    } else if (chosenTheme == QStringLiteral("Sepia")) {
        m_currentTheme = Theme::Sepia;
    } else {
        m_currentTheme = Theme::Light;
    }

    m_folderTreeCollapsed = m_settings->value(QStringLiteral("isTreeCollapsed"), false).toBool();
    m_noteListCollapsed = m_settings->value(QStringLiteral("isNoteListCollapsed"), false).toBool();
    m_alwaysOnTop = m_settings->value(QStringLiteral("alwaysStayOnTop"), false).toBool();
    m_editorSettingsScrollBarPosition =
        m_settings->value(QStringLiteral("editorSettingsScrollBarPosition"), 0.0).toDouble();

    m_restoreIsSelectingFolder = m_settings->value(QStringLiteral("isSelectingFolder"), true).toBool();
    m_restoreFolderPath =
        m_settings->value(QStringLiteral("currentSelectFolder"), NodePath::getAllNoteFolderPath()).toString();

    const QStringList currentSelectTagsId =
        m_settings->value(QStringLiteral("currentSelectTagsId"), QStringList{}).toStringList();
    for (const auto& tagId : currentSelectTagsId) {
        m_restoreTagIds.insert(tagId.toInt());
    }

    const QStringList currentSelectNotes =
        m_settings->value(QStringLiteral("currentSelectNotesId"), QStringList{}).toStringList();
    if (!currentSelectNotes.isEmpty()) {
        m_restoreSelectedNoteId = currentSelectNotes.first().toInt();
    }
}

void AppBackend::setupDatabases() {
    QFileInfo fi(m_settings->fileName());
    QDir dir(fi.absolutePath());
    if (!dir.mkpath(QStringLiteral("."))) {
        qFatal("ERROR: Can't create settings folder : %s", dir.absolutePath().toStdString().c_str());
    }

    const QString defaultDBPath = QStringLiteral("%1%2notes.db").arg(dir.path(), QDir::separator());
    QString noteDBFilePath = m_settings->value(QStringLiteral("noteDBFilePath"), QString()).toString();
    if (noteDBFilePath.isEmpty()) {
        noteDBFilePath = defaultDBPath;
    }

    QFileInfo noteDBFilePathInf(noteDBFilePath);
    QFileInfo defaultDBPathInf(defaultDBPath);
    if ((!noteDBFilePathInf.exists()) && defaultDBPathInf.exists()) {
        QDir().mkpath(noteDBFilePathInf.absolutePath());
        QFile defaultDBFile(defaultDBPath);
        defaultDBFile.rename(noteDBFilePath);
    }

    if (m_settings->value(QStringLiteral("version"), QStringLiteral("NULL")).toString() == QStringLiteral("NULL")) {
        m_needMigrateFromV1_5_0 = true;
    }
    const auto versionString = m_settings->value(QStringLiteral("version"), QStringLiteral("0.0.0")).toString();
    if (versionString.split('.').first().toInt() < 2) {
        m_needMigrateFromV1_5_0 = true;
    }

    if (QFile::exists(noteDBFilePath) && m_needMigrateFromV1_5_0) {
        {
            auto db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("default_database"));
            db.setDatabaseName(noteDBFilePath);
            if (db.open()) {
                QSqlQuery query(db);
                if (query.exec(
                        QStringLiteral("SELECT name FROM sqlite_master WHERE type='table' AND name='tag_table';")) &&
                    query.next() && query.value(0).toString() == QStringLiteral("tag_table")) {
                    m_needMigrateFromV1_5_0 = false;
                }
                db.close();
            }
        }
        QSqlDatabase::removeDatabase(QStringLiteral("default_database"));
    }

    if (!QFile::exists(noteDBFilePath)) {
        QFile noteDBFile(noteDBFilePath);
        if (!noteDBFile.open(QIODevice::WriteOnly)) {
            qFatal("ERROR : Can't create database file");
        }
        noteDBFile.close();
        m_doCreate = true;
        m_needMigrateFromV1_5_0 = false;
    } else if (m_needMigrateFromV1_5_0) {
        QFile noteDBFile(noteDBFilePath);
        m_oldDbPath = dir.path() + QDir::separator() + QStringLiteral("oldNotes.db");
        noteDBFile.rename(m_oldDbPath);
        noteDBFile.setFileName(noteDBFilePath);
        if (!noteDBFile.open(QIODevice::WriteOnly)) {
            qFatal("ERROR : Can't create database file");
        }
        noteDBFile.close();
        m_doCreate = true;
    }

    if (m_needMigrateFromV1_5_0) {
        m_settings->setValue(QStringLiteral("version"), qApp->applicationVersion());
    }

    m_noteDbPath = noteDBFilePath;
    m_dbManager = new DBManager;
    m_dbThread = new QThread(this);
    m_dbThread->setObjectName(QStringLiteral("dbThread"));
    m_dbManager->moveToThread(m_dbThread);
    connect(m_dbThread, &QThread::finished, m_dbManager, &QObject::deleteLater);
}

void AppBackend::setupModels() {
    m_tagPool = new TagPool(m_dbManager, this);
    m_noteModel = new NoteListModel(this);
    m_treeModel = new NodeTreeModel(this);
    m_noteEditor = new NoteEditorBackend(m_tagPool, m_dbManager, this);
    m_noteEditor->setMarkdownEnabled(!m_settings->value(QStringLiteral("isMarkdownDisabled"), false).toBool());
}

void AppBackend::setupConnections() {
    connect(this, &AppBackend::requestOpenDBManager, m_dbManager, &DBManager::onOpenDBManagerRequested,
            Qt::QueuedConnection);
    connect(this, &AppBackend::requestMigrateNotesFromV1_5_0, m_dbManager, &DBManager::onMigrateNotesFrom1_5_0Requested,
            Qt::QueuedConnection);
    connect(this, &AppBackend::requestNodesTree, m_dbManager, &DBManager::onNodeTagTreeRequested, Qt::QueuedConnection);
    connect(this, &AppBackend::requestNotesListInFolder, m_dbManager, &DBManager::onNotesListInFolderRequested,
            Qt::QueuedConnection);
    connect(this, &AppBackend::requestNotesListInTags, m_dbManager, &DBManager::onNotesListInTagsRequested,
            Qt::QueuedConnection);
    connect(this, &AppBackend::requestSearchInDb, m_dbManager, &DBManager::searchForNotes, Qt::QueuedConnection);
    connect(this, &AppBackend::requestClearSearchDb, m_dbManager, &DBManager::clearSearch, Qt::QueuedConnection);
    connect(this, &AppBackend::requestRemoveNoteDb, m_dbManager, &DBManager::removeNote, Qt::QueuedConnection);
    connect(this, &AppBackend::requestImportNotes, m_dbManager, &DBManager::onImportNotesRequested,
            Qt::QueuedConnection);
    connect(this, &AppBackend::requestRestoreNotes, m_dbManager, &DBManager::onRestoreNotesRequested,
            Qt::QueuedConnection);
    connect(this, &AppBackend::requestExportNotes, m_dbManager, &DBManager::onExportNotesRequested,
            Qt::QueuedConnection);
    connect(this, &AppBackend::requestChangeDatabasePath, m_dbManager, &DBManager::onChangeDatabasePathRequested,
            Qt::QueuedConnection);

    connect(m_dbManager, &DBManager::nodesTagTreeReceived, this, &AppBackend::loadTreeModel, Qt::QueuedConnection);
    connect(m_dbManager, &DBManager::notesListReceived, this, &AppBackend::loadNoteListModel, Qt::QueuedConnection);
    connect(m_dbManager, &DBManager::childNotesCountUpdatedTag, this, &AppBackend::onChildNotesCountChangedTag,
            Qt::QueuedConnection);
    connect(m_dbManager, &DBManager::childNotesCountUpdatedFolder, this, &AppBackend::onChildNotesCountChangedFolder,
            Qt::QueuedConnection);
    connect(m_dbManager, &DBManager::showErrorMessage, this, [this](const QString& title, const QString& content) {
        QJsonObject data{{"title", title}, {"content", content}};
        emit errorOccurred(QVariant(data));
    });

    connect(m_noteEditor, &NoteEditorBackend::requestCreateUpdateNote, m_dbManager,
            &DBManager::onCreateUpdateRequestedNoteContent, Qt::QueuedConnection);
    connect(m_noteEditor, &NoteEditorBackend::moveNoteToListViewTop, this, &AppBackend::moveNoteToTop);
    connect(m_noteEditor, &NoteEditorBackend::updateNoteDataInList, this, &AppBackend::setNoteDataInList);
    connect(m_noteEditor, &NoteEditorBackend::noteEditClosed, this, &AppBackend::onNoteEditClosed);
    connect(m_noteEditor, &NoteEditorBackend::deleteNoteRequested, this, &AppBackend::onDeleteNoteRequested);

    connect(m_noteModel, &NoteListModel::rowCountChanged, this, &AppBackend::updateListViewLabel);
    connect(m_noteModel, &NoteListModel::requestUpdatePinnedRelPos, m_dbManager, &DBManager::updateRelPosPinnedNote,
            Qt::QueuedConnection);
    connect(m_noteModel, &NoteListModel::requestUpdatePinnedRelPosAN, m_dbManager, &DBManager::updateRelPosPinnedNoteAN,
            Qt::QueuedConnection);
    connect(m_noteModel, &NoteListModel::requestUpdatePinned, m_dbManager, &DBManager::setNoteIsPinned,
            Qt::QueuedConnection);
}

void AppBackend::startBackend() {
    connect(m_dbThread, &QThread::started, this, [this]() {
        emit requestOpenDBManager(m_noteDbPath, m_doCreate);
        if (m_needMigrateFromV1_5_0 && !m_oldDbPath.isEmpty()) {
            emit requestMigrateNotesFromV1_5_0(m_oldDbPath);
        }
        emit requestNodesTree();
    });
    m_dbThread->start();
}

void AppBackend::restoreSelectionFromSettings() {
    if (m_hasLoadedInitialSelection) {
        return;
    }
    m_hasLoadedInitialSelection = true;

    if (m_restoreIsSelectingFolder) {
        if (m_restoreFolderPath == NodePath::getTrashFolderPath()) {
            openFolderContext(TRASH_FOLDER_ID, true);
            return;
        }
        if (m_restoreFolderPath == NodePath::getAllNoteFolderPath() || m_restoreFolderPath.isEmpty()) {
            openFolderContext(ROOT_FOLDER_ID, true);
            return;
        }
        const auto folderIndex = m_treeModel->folderIndexFromIdPath(m_restoreFolderPath);
        if (folderIndex.isValid()) {
            openFolderContext(folderIndex.data(NodeItem::Roles::NodeId).toInt(), false);
            return;
        }
    }

    if (!m_restoreTagIds.isEmpty()) {
        openTagContext({*m_restoreTagIds.begin()});
        return;
    }

    openFolderContext(ROOT_FOLDER_ID, true);
}

void AppBackend::openFolderContext(int folderId, bool isRecursive, bool newNote, int scrollToId) {
    if (m_listViewInfo.isInSearch && !m_searchText.isEmpty()) {
        m_listViewInfo.parentFolderId = folderId;
        m_listViewInfo.currentNotesId = {INVALID_NODE_ID};
        m_listViewInfo.isInTag = false;
        m_listViewInfo.needCreateNewNote = false;
        m_listViewInfo.currentTagList = {};
        m_listViewInfo.scrollToId = INVALID_NODE_ID;
        emit requestSearchInDb(m_searchText, m_listViewInfo);
        return;
    }
    emit requestNotesListInFolder(folderId, isRecursive, newNote, scrollToId);
}

void AppBackend::openTagContext(const QSet<int>& tagIds, bool newNote, int scrollToId) {
    if (m_listViewInfo.isInSearch && !m_searchText.isEmpty()) {
        m_listViewInfo.parentFolderId = INVALID_NODE_ID;
        m_listViewInfo.currentNotesId = {INVALID_NODE_ID};
        m_listViewInfo.isInTag = true;
        m_listViewInfo.needCreateNewNote = false;
        m_listViewInfo.currentTagList = tagIds;
        m_listViewInfo.scrollToId = INVALID_NODE_ID;
        emit requestSearchInDb(m_searchText, m_listViewInfo);
        return;
    }
    emit requestNotesListInTags(tagIds, newNote, scrollToId);
}

void AppBackend::reloadCurrentContext(bool newNote, int scrollToId) {
    if (m_listViewInfo.isInTag) {
        openTagContext(m_listViewInfo.currentTagList, newNote, scrollToId);
    } else {
        const int folderId =
            m_listViewInfo.parentFolderId == INVALID_NODE_ID ? ROOT_FOLDER_ID : m_listViewInfo.parentFolderId;
        const bool recursive = folderId == ROOT_FOLDER_ID || folderId == TRASH_FOLDER_ID;
        openFolderContext(folderId, recursive, newNote, scrollToId);
    }
}

void AppBackend::selectNoteIndex(const QModelIndex& index, bool saveSelection) {
    if (!index.isValid()) {
        m_selectedNoteId = INVALID_NODE_ID;
        if (saveSelection) {
            saveSelectedNote(INVALID_NODE_ID);
        }
        m_noteEditor->closeEditor();
        emit currentNotePinnedChanged();
        return;
    }

    const int targetNoteId = index.data(NoteListModel::NoteID).toInt();
    if (m_noteEditor->isTempNote() && m_noteEditor->currentText().trimmed().isEmpty() &&
        m_noteEditor->currentEditingNoteId() != targetNoteId) {
        m_noteEditor->closeEditor();
    }

    const NodeData note = m_noteModel->getNote(index);
    m_selectedNoteId = note.id();
    if (saveSelection) {
        saveSelectedNote(m_selectedNoteId);
    }
    m_noteEditor->setForcedReadOnly((!m_listViewInfo.isInTag) && m_listViewInfo.parentFolderId == TRASH_FOLDER_ID);
    m_noteEditor->showNotesInEditor({note});
    emit currentNotePinnedChanged();
}

void AppBackend::updateListViewLabel() {
    QString label1;
    if ((!m_listViewInfo.isInTag) && m_listViewInfo.parentFolderId == ROOT_FOLDER_ID) {
        label1 = QStringLiteral("All Notes");
    } else if ((!m_listViewInfo.isInTag) && m_listViewInfo.parentFolderId == TRASH_FOLDER_ID) {
        label1 = QStringLiteral("Trash");
    } else if (!m_listViewInfo.isInTag) {
        NodeData parentFolder;
        QMetaObject::invokeMethod(m_dbManager, "getNode", Qt::BlockingQueuedConnection,
                                  Q_RETURN_ARG(NodeData, parentFolder), Q_ARG(int, m_listViewInfo.parentFolderId));
        label1 = parentFolder.fullTitle();
    } else if (m_listViewInfo.currentTagList.isEmpty()) {
        label1 = QStringLiteral("Tags ...");
    } else if (m_listViewInfo.currentTagList.size() > 1) {
        label1 = QStringLiteral("Multiple tags ...");
    } else {
        const int tagId = *m_listViewInfo.currentTagList.begin();
        label1 = m_tagPool->contains(tagId) ? m_tagPool->getTag(tagId).name() : QStringLiteral("Tags ...");
    }

    const QString label2 = QString::number(m_noteModel->rowCount());
    if (m_listLabel1 != label1 || m_listLabel2 != label2) {
        m_listLabel1 = label1;
        m_listLabel2 = label2;
        emit listLabelsChanged();
    }
}

void AppBackend::updateCurrentFont() {
    switch (m_currentFontTypeface) {
        case FontTypeface::Mono:
            m_currentEditorFontFamily = m_listOfMonoFonts.value(m_chosenMonoFontIndex, m_listOfMonoFonts.first());
            break;
        case FontTypeface::Serif:
            m_currentEditorFontFamily = m_listOfSerifFonts.value(m_chosenSerifFontIndex, m_listOfSerifFonts.first());
            break;
        case FontTypeface::SansSerif:
            m_currentEditorFontFamily =
                m_listOfSansSerifFonts.value(m_chosenSansSerifFontIndex, m_listOfSansSerifFonts.first());
            break;
    }

    QFont font(m_currentEditorFontFamily, m_editorMediumFontSize);
    const QFontMetrics metrics(font);
    m_textColumnWidth = std::max(360, metrics.horizontalAdvance(QLatin1Char('n')) * currentCharsLimit());
    m_noteEditor->updateHighlightingTheme(m_currentTheme, editorTextColorForTheme(m_currentTheme),
                                          m_editorMediumFontSize);
    emit editorFontChanged();
    emit editorLayoutChanged();
}

void AppBackend::saveEditorSettings() const {
    m_settings->setValue(QStringLiteral("selectedFontTypeface"),
                         QString::fromStdString(to_string(m_currentFontTypeface)));
    m_settings->setValue(QStringLiteral("editorMediumFontSize"), m_editorMediumFontSize);
    m_settings->setValue(QStringLiteral("isTextFullWidth"), m_textFullWidth);
    m_settings->setValue(QStringLiteral("charsLimitPerFontMono"), m_charsLimitPerFont.mono);
    m_settings->setValue(QStringLiteral("charsLimitPerFontSerif"), m_charsLimitPerFont.serif);
    m_settings->setValue(QStringLiteral("charsLimitPerFontSansSerif"), m_charsLimitPerFont.sansSerif);
    m_settings->setValue(QStringLiteral("chosenMonoFont"), m_listOfMonoFonts.value(m_chosenMonoFontIndex));
    m_settings->setValue(QStringLiteral("chosenSerifFont"), m_listOfSerifFonts.value(m_chosenSerifFontIndex));
    m_settings->setValue(QStringLiteral("chosenSansSerifFont"),
                         m_listOfSansSerifFonts.value(m_chosenSansSerifFontIndex));
    m_settings->setValue(QStringLiteral("theme"), QString::fromStdString(to_string(m_currentTheme)));
    m_settings->setValue(QStringLiteral("isTreeCollapsed"), m_folderTreeCollapsed);
    m_settings->setValue(QStringLiteral("isNoteListCollapsed"), m_noteListCollapsed);
    m_settings->setValue(QStringLiteral("isMarkdownDisabled"), !m_noteEditor->markdownEnabled());
    m_settings->setValue(QStringLiteral("alwaysStayOnTop"), m_alwaysOnTop);
    m_settings->setValue(QStringLiteral("editorSettingsScrollBarPosition"), m_editorSettingsScrollBarPosition);
}

void AppBackend::saveTreeSelection(bool isFolder, const QString& folderPath, const QSet<int>& tagIds) const {
    m_settings->setValue(QStringLiteral("isSelectingFolder"), isFolder);
    m_settings->setValue(QStringLiteral("currentSelectFolder"), folderPath);
    QStringList sl;
    for (const auto& id : tagIds) {
        sl.append(QString::number(id));
    }
    m_settings->setValue(QStringLiteral("currentSelectTagsId"), sl);
}

void AppBackend::saveSelectedNote(int noteId) const {
    QStringList sl;
    if (noteId != INVALID_NODE_ID) {
        sl.append(QString::number(noteId));
    }
    m_settings->setValue(QStringLiteral("currentSelectNotesId"), sl);
}

void AppBackend::emitDisplayFontState() {
    emit displayFontFamilyChanged();
    QJsonObject data{{"displayFont", m_displayFontFamily}};
    emit displayFontSet(QVariant(data));
    emit platformSet(QVariant(m_platformName));
    emit qtVersionSet(QVariant(6));
}

void AppBackend::emitFontState() {
    emit editorFontChanged();
    emit fontsChanged(QVariant(buildFontData()));
}

void AppBackend::emitThemeState() {
    emit themeDataObjectChanged();
    emit themeChanged(themeData());
}

void AppBackend::emitSettingsState() {
    emit settingsChanged(QVariant(buildSettingsData()));
}

void AppBackend::emitWindowState() {
    QJsonObject resizeData{{"parentWindowHeight", m_parentWindowHeight}, {"parentWindowWidth", m_parentWindowWidth}};
    emit mainWindowResized(QVariant(resizeData));
    QJsonObject moveData{{"parentWindowX", m_parentWindowX}, {"parentWindowY", m_parentWindowY}};
    emit mainWindowMoved(QVariant(moveData));
}

QJsonObject AppBackend::buildThemeData() const {
    switch (m_currentTheme) {
        case Theme::Dark:
            return {{"theme", QStringLiteral("Dark")}, {"backgroundColor", QStringLiteral("#191919")}};
        case Theme::Sepia:
            return {{"theme", QStringLiteral("Sepia")}, {"backgroundColor", QStringLiteral("#fbf0d9")}};
        case Theme::Light:
        default:
            return {{"theme", QStringLiteral("Light")}, {"backgroundColor", QStringLiteral("#f7f7f7")}};
    }
}

QJsonObject AppBackend::buildFontData() const {
    QJsonObject data;
    data["listOfSansSerifFonts"] = QJsonArray::fromStringList(m_listOfSansSerifFonts);
    data["listOfSerifFonts"] = QJsonArray::fromStringList(m_listOfSerifFonts);
    data["listOfMonoFonts"] = QJsonArray::fromStringList(m_listOfMonoFonts);
    data["chosenSansSerifFontIndex"] = m_chosenSansSerifFontIndex;
    data["chosenSerifFontIndex"] = m_chosenSerifFontIndex;
    data["chosenMonoFontIndex"] = m_chosenMonoFontIndex;
    data["currentFontTypeface"] = QString::fromStdString(to_string(m_currentFontTypeface));
    return data;
}

QJsonObject AppBackend::buildSettingsData() const {
    QJsonObject data;
    data["currentFontTypeface"] = QString::fromStdString(to_string(m_currentFontTypeface));
    data["currentTheme"] = currentThemeName();
    data["isTextFullWidth"] = m_textFullWidth;
    data["currentView"] = m_kanbanVisible ? QStringLiteral("KanbanView") : QStringLiteral("TextView");
    data["isNoteListCollapsed"] = m_noteListCollapsed;
    data["isFoldersTreeCollapsed"] = m_folderTreeCollapsed;
    data["isMarkdownDisabled"] = !m_noteEditor->markdownEnabled();
    data["isStayOnTop"] = m_alwaysOnTop;
    return data;
}

QString AppBackend::currentThemeName() const {
    return QString::fromStdString(to_string(m_currentTheme));
}

int AppBackend::currentCharsLimit() const {
    switch (m_currentFontTypeface) {
        case FontTypeface::Mono:
            return m_charsLimitPerFont.mono;
        case FontTypeface::Serif:
            return m_charsLimitPerFont.serif;
        case FontTypeface::SansSerif:
        default:
            return m_charsLimitPerFont.sansSerif;
    }
}

int AppBackend::currentFolderIdForNewNote() const {
    if ((!m_listViewInfo.isInTag) && (m_listViewInfo.parentFolderId > ROOT_FOLDER_ID)) {
        NodeData parent;
        QMetaObject::invokeMethod(m_dbManager, "getNode", Qt::BlockingQueuedConnection, Q_RETURN_ARG(NodeData, parent),
                                  Q_ARG(int, m_listViewInfo.parentFolderId));
        if (parent.nodeType() == NodeData::Type::Folder) {
            return parent.id();
        }
    }
    return DEFAULT_NOTES_FOLDER_ID;
}
