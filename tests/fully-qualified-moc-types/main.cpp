#include <QtCore/QObject>

struct A {};

namespace NS {
    struct MyType {};
    class MyObject : public QObject
    {
        Q_OBJECT
    Q_SIGNALS:
        void mysig(NS::MyType);
        void mysig2(MyType); // Warn
        void mysig3(NS::MyType);
        void mysig4(const NS::MyType &);
        void mysig5(A);
        void mysig6(const A);
        void mysig7(const A *);
        void mysig8(A *);
    public Q_SLOTS:
        void myslot1(NS::MyType);
        void myslot2(MyType); // Warn
        void myslot3(NS::MyType);
        void myslot4(const NS::MyType &);
        void myslot5(A);
        void myslot6(const A);
        void myslot7(const A *);
        void myslot8(A *);
    };
}

#include "main.moc_"
