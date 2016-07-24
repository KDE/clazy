#include <QtCore/QHash>
#include <QtCore/QMap>
#include <QtCore/QPointer>
#include <QtCore/QString>
#include <memory>
struct A {};
typedef QPointer<A> A_QPointer;
using namespace std;
void test()
{
    QPointer<A> pa;
    A a;

    QHash<A, QString> hash1;
    QHash<A*, QString> hash2;
    QHash<QPointer<A>, QString> hash3; // Warning
    QHash<A_QPointer, QString> hash4; // Warning

    QMap<A, QString> map1;
    QMap<A*, QString> map2;
    QMap<QPointer<A>, QString> map3; // Warning
    QMap<A_QPointer, QString> map4; // Warning
    QMap<weak_ptr<A>, QString> map5; // Warning

    QList<QPointer<A>> list;
}
