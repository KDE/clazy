#include <QtCore/QObject>
#include <QtCore/QVariant>

void test()
{
    QObject o;

    QVariant v, returnedValue;
    QMetaObject::invokeMethod(&o, "mySlot",
                              Q_RETURN_ARG(QVariant, returnedValue), // OK
                              Q_ARG(const QVariant, v)); // Warn
    QMetaObject::invokeMethod(&o, "mySlot",
                              Q_RETURN_ARG(QVariant, returnedValue), // OK
                              Q_ARG(const QVariant, // Warn
                                    v)); // multi-line

    Q_ARG(QVariant , v); // OK
    Q_ARG(QVariant   &, v); // Warn
    Q_ARG(QVariant&, v); // OK
    Q_ARG(const QVariant &, v); // Warn
}

void testConnect()
{
    QObject o;
    o.connect(&o, SIGNAL(destroyed(int, int)), // Warn
              &o, SLOT(void foo(const int))); // Warn
    o.connect(&o, SIGNAL(destroyed(int,int)), // OK
              &o, SLOT(void foo(int))); // OK

    o.disconnect(&o, SLOT(void foo(const int))); // OK
}


class MyObj :public QObject
{
public:
    MyObj()
    {
        connect(ui->host, SIGNAL(textChanged(QString)), // OK
                SLOT(validateHostAddress(const QString&))); // WARN
    }

    MyObj *ui;
    MyObj *host;
};
