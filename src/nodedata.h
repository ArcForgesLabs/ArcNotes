#ifndef NODEDATA_H
#define NODEDATA_H

#include <QDateTime>
#include <QObject>
#include <QSet>

namespace {
auto constexpr INVALID_NODE_ID = -1;
auto constexpr ROOT_FOLDER_ID = 0;
auto constexpr TRASH_FOLDER_ID = 1;
auto constexpr DEFAULT_NOTES_FOLDER_ID = 2;
}  // namespace

class NodeData {
public:
    explicit NodeData();

    enum class Type : uint8_t { Note = 0, Folder };

    [[nodiscard]] int id() const;
    void setId(int id);

    [[nodiscard]] const QString& fullTitle() const;
    void setFullTitle(const QString& fullTitle);

    [[nodiscard]] const QDateTime& lastModificationdateTime() const;
    void setLastModificationDateTime(const QDateTime& lastModificationdateTime);

    [[nodiscard]] QDateTime creationDateTime() const;
    void setCreationDateTime(const QDateTime& creationDateTime);

    [[nodiscard]] const QString& content() const;
    void setContent(const QString& content);

    [[nodiscard]] bool isModified() const;
    void setModified(bool isModified);

    [[nodiscard]] bool isSelected() const;
    void setSelected(bool isSelected);

    [[nodiscard]] int scrollBarPosition() const;
    void setScrollBarPosition(int scrollBarPosition);

    [[nodiscard]] QDateTime deletionDateTime() const;
    void setDeletionDateTime(const QDateTime& deletionDateTime);

    [[nodiscard]] NodeData::Type nodeType() const;
    void setNodeType(NodeData::Type newNodeType);

    [[nodiscard]] int parentId() const;
    void setParentId(int newParentId);

    [[nodiscard]] int relativePosition() const;
    void setRelativePosition(int newRelativePosition);

    [[nodiscard]] const QString& absolutePath() const;
    void setAbsolutePath(const QString& newAbsolutePath);

    [[nodiscard]] const QSet<int>& tagIds() const;
    void setTagIds(const QSet<int>& newTagIds);

    [[nodiscard]] bool isTempNote() const;
    void setIsTempNote(bool newIsTempNote);

    [[nodiscard]] const QString& parentName() const;
    void setParentName(const QString& newParentName);

    [[nodiscard]] bool isPinnedNote() const;
    void setIsPinnedNote(bool newIsPinnedNote);

    [[nodiscard]] int tagListScrollBarPos() const;
    void setTagListScrollBarPos(int newTagListScrollBarPos);

    [[nodiscard]] int relativePosAN() const;
    void setRelativePosAN(int newRelativePosAN);

    [[nodiscard]] int childNotesCount() const;
    void setChildNotesCount(int newChildCount);

private:
    int m_id;
    QString m_fullTitle;
    QDateTime m_lastModificationDateTime;
    QDateTime m_creationDateTime;
    QDateTime m_deletionDateTime;
    QString m_content;
    bool m_isModified;
    bool m_isSelected;
    int m_scrollBarPosition;
    NodeData::Type m_nodeType;
    int m_parentId;
    int m_relativePosition;
    QString m_absolutePath;
    QSet<int> m_tagIds;
    bool m_isTempNote;
    QString m_parentName;
    bool m_isPinnedNote;
    int m_tagListScrollBarPos;
    int m_relativePosAN;
    int m_childNotesCount;
};

Q_DECLARE_METATYPE(NodeData)

QDataStream& operator>>(QDataStream& stream, NodeData& nodeData);
QDataStream& operator>>(QDataStream& stream, NodeData*& nodeData);

#endif  // NODEDATA_H
