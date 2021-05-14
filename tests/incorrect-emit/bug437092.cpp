#include <QtCore/QObject>

class Obj : public QObject
{
Q_OBJECT
public:
    template<class T>
    void createSomeObject();
Q_SIGNALS:
};

template<class T>
void Obj::createSomeObject()
{
}

void test() {
    auto obj = new Obj();
    obj->createSomeObject<QObject>();
}
