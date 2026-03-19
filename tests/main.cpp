#include <QCoreApplication>
#include <QTest>

#include "tst_notedata.h"
#include "tst_notemodel.h"

int main(int argc, char* argv[]) {
    QCoreApplication a(argc, argv);
    QTest::qExec(new tst_NoteData, argc, argv);
    QTest::qExec(new tst_NoteModel, argc, argv);
    return 0;
}
