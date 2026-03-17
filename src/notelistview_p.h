#pragma once
#include <QtWidgets/private/qabstractitemview_p.h>

#include "notelistview.h"

class NoteListViewPrivate : public QAbstractItemViewPrivate {
    Q_DECLARE_PUBLIC(NoteListView)

public:
    NoteListViewPrivate() : QAbstractItemViewPrivate() {};
    ~NoteListViewPrivate() override = default;
    QPixmap renderToPixmap(const QModelIndexList& indexes, QRect* r) const;
    QStyleOptionViewItem viewOptionsV1() const;
};
