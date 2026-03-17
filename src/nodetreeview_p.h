#pragma once
#include <QtWidgets/private/qabstractitemview_p.h>

#include "nodetreeview.h"

class NodeTreeViewPrivate : public QAbstractItemViewPrivate {
    Q_DECLARE_PUBLIC(NodeTreeView)

public:
    NodeTreeViewPrivate() : QAbstractItemViewPrivate() {};
    ~NodeTreeViewPrivate() override = default;
};
