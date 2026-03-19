#ifndef NODETREEMODEL_H
#define NODETREEMODEL_H

#include <QAbstractItemModel>
#include <QHash>
#include <QModelIndex>
#include <QVariant>
#include <QVector>

#include "dbmanager.h"
#include "nodedata.h"
#include "nodepath.h"

namespace NodeItem {
enum Roles {
    ItemType = Qt::UserRole,
    DisplayText = Qt::DisplayRole,
    Icon = Qt::DecorationRole,
    TagColor = Qt::UserRole + 1,
    IsExpandable,
    AbsPath,
    RelPos,
    ChildCount,
    NodeId
};
enum Type {
    AllNoteButton = 1,
    // We store this enum inside QVariant,
    // and an invalid QVariant conversion return 0
    TrashButton,
    FolderSeparator,
    TagSeparator,
    FolderItem,
    NoteItem,
    TagItem,
    RootItem
};
}  // namespace NodeItem

class NodeTreeItem {
public:
    explicit NodeTreeItem(const QHash<NodeItem::Roles, QVariant>& data, NodeTreeItem* parentItem = nullptr);
    ~NodeTreeItem();

    void appendChild(NodeTreeItem* child);
    void insertChild(int row, NodeTreeItem* child);
    [[nodiscard]] NodeTreeItem* getChild(int row) const;
    void removeChild(int row);
    NodeTreeItem* takeChildAt(int row);
    [[nodiscard]] int getChildCount() const;
    static int getColumnCount();  // columnCount is always 1 => static
    [[nodiscard]] int recursiveNodeCount() const;
    void recursiveUpdateFolderPath(const QString& oldP, const QString& newP);
    [[nodiscard]] QVariant getData(NodeItem::Roles role) const;
    void setData(NodeItem::Roles role, const QVariant& d);
    [[nodiscard]] int getRow() const;
    [[nodiscard]] NodeTreeItem* getParentItem() const;
    void setParentItem(NodeTreeItem* parentItem);
    void moveChild(int from, int to);
    void recursiveSort();

private:
    QVector<NodeTreeItem*> m_childItems;
    QHash<NodeItem::Roles, QVariant> m_itemData;
    NodeTreeItem* m_parentItem;
};

class NodeTreeModel : public QAbstractItemModel {
    Q_OBJECT

public:
    explicit NodeTreeModel(QObject* parent = nullptr);
    ~NodeTreeModel() override;

    void appendChildNodeToParent(const QModelIndex& parentIndex, const QHash<NodeItem::Roles, QVariant>& data);
    [[nodiscard]] QModelIndex rootIndex() const;
    QModelIndex folderIndexFromIdPath(const NodePath& idPath);
    QModelIndex tagIndexFromId(int id);
    QString getNewFolderPlaceholderName(const QModelIndex& parentIndex);
    QString getNewTagPlaceholderName();
    QVector<QModelIndex> getSeparatorIndex();
    QModelIndex getDefaultNotesIndex();
    QModelIndex getAllNotesButtonIndex();
    QModelIndex getTrashButtonIndex();
    void deleteRow(const QModelIndex& rowIndex, const QModelIndex& parentIndex);

public slots:
    void setTreeData(const NodeTagTreeData& treeData);

    // QAbstractItemModel interface

public:
    [[nodiscard]] QModelIndex index(int row, int column, const QModelIndex& parent) const override;
    [[nodiscard]] QModelIndex parent(const QModelIndex& index) const override;
    [[nodiscard]] int rowCount(const QModelIndex& parent) const override;
    [[nodiscard]] int columnCount(const QModelIndex& parent) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;
    [[nodiscard]] Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;
    [[nodiscard]] Qt::DropActions supportedDropActions() const override;
    [[nodiscard]] Qt::DropActions supportedDragActions() const override;
    [[nodiscard]] QStringList mimeTypes() const override;
    [[nodiscard]] QMimeData* mimeData(const QModelIndexList& indexes) const override;
    bool dropMimeData(const QMimeData* mime, Qt::DropAction action, int row, int column,
                      const QModelIndex& parent) override;

signals:
    void topLevelItemLayoutChanged();
    void requestExpand(const QString& indexPath);
    void requestMoveNode(int nodeId, int targetId);
    void requestUpdateNodeRelativePosition(int nodeId, int relativePosition);
    void requestUpdateTagRelativePosition(int nodeId, int relativePosition);
    void requestUpdateAbsPath(const QString& oldPath, const QString& newPath);
    void dropFolderSuccessful(const QString& paths);
    void dropTagsSuccessful(const QSet<int>& ids);
    void requestMoveFolderToTrash(const QModelIndex& index);

private:
    NodeTreeItem* m_rootItem;
    void loadNodeTree(const QVector<NodeData>& nodeData, NodeTreeItem* rootNode);
    void appendAllNotesAndTrashButton(NodeTreeItem* rootNode);
    void appendFolderSeparator(NodeTreeItem* rootNode);
    void appendTagsSeparator(NodeTreeItem* rootNode);
    void loadTagList(const QVector<TagData>& tagData, NodeTreeItem* rootNode);
    void updateChildRelativePosition(NodeTreeItem* parent, NodeItem::Type type);
};

#endif  // NODETREEMODEL_H
