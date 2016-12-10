#include <QtCore/QObject>

class MyObj : public QObject
{
public:

    bool event(QEvent *ev) override;
};

bool MyObj::event(QEvent *ev)
{
    if (false) {}

    return false; // Warning
}


class MyObj2 : public QObject
{
public:

    bool event(QEvent *ev) override
    {
        return false; // Warning
    }
};



class MyObj3 : public QObject
{
public:

    bool event2()
    {
        return false; // OK
    }

    bool event(QEvent *ev) override
    {
        return true; // OK
    }

    bool eventFilter(QObject *, QEvent *) override
    {
        return false; // OK
    }
};

class MyObj4 : public MyObj3
{
public:

    bool event2()
    {
        return false; // OK
    }

    bool event(QEvent *ev) override
    {
        return false; // Warning
    }

    bool eventFilter(QObject *, QEvent *) override
    {
        return false; // Warning
    }
};
