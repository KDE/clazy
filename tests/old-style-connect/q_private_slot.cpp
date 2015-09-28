#include <QtCore/QObject>

class MyObj : public QObject
{
    Q_OBJECT
    MyObj()
    {
        connect(this, SIGNAL(destroyed()), SLOT(privSlot()));
        connect(this, SIGNAL(destroyed()), this, SLOT(privSlot()));
    }
public:
    class Private;
    Private * d;
    Q_PRIVATE_SLOT(d, void privSlot())
};

class MyObj::Private
{
public:
    Private()
    {
        q->connect(q, SIGNAL(destroyed()), SLOT(privSlot()));
        q->connect(q, SIGNAL(destroyed()), q, SLOT(privSlot()));
        QObject *other;
        q->connect(other, SIGNAL(destroyed()), SLOT(privSlot()));
    }





    void somePrivFunction();

public Q_SLOTS:
    void privSlot();
private:
    MyObj *q;
};

void MyObj::Private::somePrivFunction()
{
    QObject *other;
    q->connect(other, SIGNAL(destroyed()), SLOT(privSlot()));
    q->connect(other, SIGNAL(doesnt_exist()), SLOT(privSlot()));
}
