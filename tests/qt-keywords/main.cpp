#include <QtCore/QObject>
#include <QtCore/QString>

class MyObj : public QObject
{
public slots:
    void slot1();

public Q_SLOTS:
    void slot2();
signals:
    void signal1();
Q_SIGNALS:
    void signal2();
public:
    Q_SLOT void slot3();
    Q_SIGNAL void signal3();
    void test()
    {
        emit signal1();
        QList<int> l;
        foreach(int i, l) {}
    }
};

