#include <QtCore/QObject>

class A {
public:
};

namespace NS {
class A {
public:
};
}

typedef int FooInt;

enum Enum1 {};


class MyObj : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int r_good READ r_good CONSTANT)
    Q_PROPERTY(int r_bad READ r_bad CONSTANT)

    Q_PROPERTY(int m_good MEMBER)
    Q_PROPERTY(int m_bad MEMBER)

    Q_PROPERTY(int rw_good READ rw_good WRITE set_rw_good NOTIFY rw_good_changed)
    Q_PROPERTY(int rw_bad READ rw_bad WRITE set_rw_bad NOTIFY rw_bad_changed)

    Q_PROPERTY(int rw_good_cref READ rw_good_cref WRITE set_rw_good_cref NOTIFY rw_good_cref_changed)
    Q_PROPERTY(int rw_bad_cref READ rw_bad_cref WRITE set_rw_bad_cref NOTIFY rw_bad_cref_changed)

    Q_PROPERTY(int* rw_good_ptr READ rw_good_ptr WRITE set_rw_good_ptr NOTIFY rw_good_ptr_changed)
    Q_PROPERTY(int* rw_bad_ptr READ rw_bad_ptr WRITE set_rw_bad_ptr NOTIFY rw_bad_ptr_changed)

    Q_PROPERTY(bool boolTest READ boolTest CONSTANT)
    Q_PROPERTY(A* classTest1 READ classTest1 CONSTANT)
    Q_PROPERTY(A classTest2 READ classTest2 CONSTANT)
    Q_PROPERTY(NS::A* classTest3 READ classTest3 CONSTANT)
    Q_PROPERTY(NS::A classTest4 READ classTest4 CONSTANT)
    Q_PROPERTY(A *classTest5 READ classTest3 CONSTANT)
    Q_PROPERTY(Enum1 enumTest READ enumTest CONSTANT)
    Q_PROPERTY(Enum1 enumTest READ enumTest NOTIFY zeroArgsSignal)
    Q_PROPERTY(InnerEnum innerEnum READ innerEnum CONSTANT)
    Q_PROPERTY(MyObj::InnerEnum innerEnum1 READ innerEnum1 CONSTANT)
    Q_PROPERTY(FooInt intTypedef1 READ intTypedef1 CONSTANT)
    Q_PROPERTY(int intTypedef2 READ intTypedef2 CONSTANT)
    Q_PROPERTY(FooInt intTypedef3 READ intTypedef3 CONSTANT)
    Q_PROPERTY(Qt::Alignment intTypedef4 READ intTypedef4 CONSTANT)
    Q_PROPERTY(InnerEnums intTypedef5 READ intTypedef5 CONSTANT)

    enum InnerEnum {};
    Q_DECLARE_FLAGS(InnerEnums, InnerEnum)

    int r_good(); // OK    
    float r_bad(); // Warn

    int m_good; // OK
    float m_bad; // Warn
 
    int rw_good(); // OK
    void set_rw_good(int); // OK

    float rw_bad(); // Warn
    void set_rw_bad(float); // Warn
    
    const int& rw_good_cref(); // OK
    void set_rw_good_cref(const int&); // OK

    const float& rw_bad_cref(); // Warn
    void set_rw_bad_cref(const float&); // Warn

    int* rw_good_ptr(); // OK
    void set_rw_good_ptr(int*); // OK

    float* rw_bad_ptr(); // Warn
    void set_rw_bad_ptr(float*); // Warn

    bool boolTest() const;
    A* classTest1() const;
    A classTest2() const;
    NS::A* classTest3() const;
    NS::A classTest4() const;
    A* classTest5() const;
    Enum1 enumTest() const;
    InnerEnum innerEnum () const;
    InnerEnum innerEnum1 () const;

    int intTypedef1() const;
    FooInt intTypedef2() const;
    FooInt intTypedef3() const;
    Qt::Alignment intTypedef4() const;
    InnerEnums intTypedef5() const;

signals:
    void rw_good_changed(int); // OK
    void rw_bad_changed(float); // Warn
    void rw_good_cref_changed(const int&); // OK
    void rw_bad_cref_changed(const float&); // Warn
    void rw_good_ptr_changed(int*); // OK
    void rw_bad_ptr_changed(float*); // Warn
    void zeroArgsSignal();
};

class MyGadget
{
    Q_GADGET
    Q_PROPERTY(int good MEMBER)
    Q_PROPERTY(int bad MEMBER)

    int good; // Ok
    float bad; // Warn
};
