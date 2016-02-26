#include <QtCore/QVariant>
#include <QtCore/QUrl>













void test()
{
    QVariant v;
    v.toUrl();
    v.toBool();
    v.value<QUrl>();
    v.value<bool>();
}
