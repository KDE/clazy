#include "main.h"
#include <QtGui/QColor>
#include <QtCore/QSharedDataPointer>
#include <QtCore/QSet>
struct Private
{
    ~Private() { int a; };
};

struct AnotherPrivate
{
    AnotherPrivate(const AnotherPrivate &) {};
};


Q_GLOBAL_STATIC(QObject, s_a)

class MyPrivate;
struct WithQSharedDataPointer
{
    ~WithQSharedDataPointer();
    WithQSharedDataPointer& operator=(const WithQSharedDataPointer &);

private:
    QSharedDataPointer<MyPrivate> d;
};
