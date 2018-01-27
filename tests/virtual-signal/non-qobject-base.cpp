#include <QtCore/QObject>

struct IFoo {
    ~IFoo() {};

    virtual void fooChanged(int); // Will be a signal in derived classes
    int getFoo() const;
    void setFoo(int) {}

private:
    int m_foo;
};

struct MyObject : QObject, IFoo {
    Q_OBJECT

signals:
    void fooChanged(int) override;
};
