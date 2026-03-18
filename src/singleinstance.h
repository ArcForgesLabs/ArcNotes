#ifndef SINGLEINSTANCE_H
#define SINGLEINSTANCE_H

#include <QLocalServer>
#include <QLocalSocket>
#include <QObject>

class SingleInstance : public QObject {
    Q_OBJECT

public:
    explicit SingleInstance(QObject* parent = nullptr);

    void listen(const QString& name);
    static bool hasPrevious(const QString& name);

signals:
    void newInstance();

private:
    QLocalSocket* m_socket;
    QLocalServer m_server;
};

#endif  // SINGLEINSTANCE_H
