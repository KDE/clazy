#include <QtCore/QObject>

class MyObject;

MyObject* somefunc()
{
    return nullptr;
}

static MyObject * s_obj;

class MyObject : public QObject
{
public:
    MyObject();
    void pub();
    MyObject* memberFunc() const;
    MyObject *another;
private:
    void priv();

public slots:
    void pubSlot();

signals:
    void sig();

private Q_SLOTS:
    void privSlot();

protected:
    void prot();
    Q_SIGNAL void singularSig();
    Q_SLOT void singularSlot();
};

void MyObject::pub()
{
    emit  prot(); // Warning: emit on non slot.
    sig(); // Warning: missing emit
    prot(); // OK
    pub(); // OK
    priv(); // OK
    privSlot(); // OK
    Q_EMIT privSlot(); // Warning
    Q_EMIT somefunc()->sig(); // OK
    somefunc()->sig(); // Warning
    Q_EMIT memberFunc()->sig(); // OK
    memberFunc()->sig(); // Warning
    emit another->sig(); // OK
    emit s_obj->sig(); // OK
}


MyObject::MyObject()
{
    emit sig(); // Warning
    emit another->sig(); // OK;
    emit memberFunc()->sig(); // OK;
    [this]{ emit sig(); }; // OK
    emit singularSig(); // Warning
    singularSlot(); // OK
}

void MyObject::singularSlot()
{
    singularSig(); // Warning
}

struct NotQObject
{
    QObject *o;
    void test1() {}
    void test()
    {
        test1(); // OK
        emit test1(); // Warning
        emit o->destroyed(); // OK
    }
};

class TestBug373947 : public QObject
{
    int method()
    {
        return otherMethod(); // OK
    }

Q_SIGNALS:
    void someSignal();

public:
    int otherMethod();
};
