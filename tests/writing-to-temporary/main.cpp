struct Foo
{
    int m;
    void setM(int v) { m = v; }
    int setX(int v) { return 0; }
    void setY() const {}
    static void setM_static(int) {}
};


Foo getFoo() { return Foo(); }
Foo& getFooRef() { static Foo f; return f; }
Foo* getFooPtr() { return new Foo(); }

void test()
{
    getFoo().setM(1); // Warning
    getFoo().setM_static(1);
    getFoo().setX(1); // OK
    getFoo().setY(); // OK
    getFooRef().setM(1); // OK
    getFooPtr()->setM(1); // OK

    Foo f;
    f.setM(1); // OK
}

#include <QtCore/QSize>

QSize getSize() { return QSize(); }

void testKnownTypes()
{
    getSize().transpose(); // Warning
}

#include <QtXml/QDomNode>

QDomNode getNode() { return {}; }

void testDisallowedType() // bug #354363
{
    getNode().firstChild(); // OK
    QDomNode node;
    node.firstChild().setPrefix(""); // OK
}
