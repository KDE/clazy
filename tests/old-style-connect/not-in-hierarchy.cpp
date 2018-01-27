#include <QtCore/QObject>

class A : public QObject {
public slots:
    void OnEvent() {}
};

class B : public QObject {
public slots:
    void OnEvent() {}
};

class C : public QObject
{
    C(QObject *client)
    {
        connect(this, SIGNAL(mysig()), client, SLOT(OnEvent())); // TODO: Would be nice to not warn in this case
    }

signals:
    void mysig();
};
