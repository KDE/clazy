#include <QtCore/QObject>
#include <QtCore/QString>

class MyObj : public QObject
{
public:
Q_SIGNALS:
    void signal1();

    void test()
    {
        emit signal1();
    }
};
