#include <QtCore/QObject>



struct HasDtor // Warning
{
    ~HasDtor() { int a; }
};

struct HasDtorAndCopyCtor // Warning
{
    HasDtorAndCopyCtor(const HasDtorAndCopyCtor &) {}
    ~HasDtorAndCopyCtor() {}
};

struct HasDtorAndCopyCtorAndAssign // OK
{
    HasDtorAndCopyCtorAndAssign(const HasDtorAndCopyCtorAndAssign &) {}
    ~HasDtorAndCopyCtorAndAssign() {}
    HasDtorAndCopyCtorAndAssign& operator=(const HasDtorAndCopyCtorAndAssign &) { return *this; }
};

struct HasNothing // OK
{
};

struct HasCopyCtor // Warning
{
    HasCopyCtor() {}
    HasCopyCtor(const HasCopyCtor &) {}
};

struct HasDefaults // OK
{
    HasDefaults(const HasDefaults &) = default;
    ~HasDefaults() = default;
    HasDefaults& operator=(const HasDefaults &) = default;
};

struct HasOnlyDTOROnPurpose // OK
{
    ~HasOnlyDTOROnPurpose();
private:
    HasOnlyDTOROnPurpose(const HasOnlyDTOROnPurpose &) = delete;
    HasOnlyDTOROnPurpose& operator=(const HasOnlyDTOROnPurpose &) = delete;
};

struct HasOnlyDTOROnPurpose2;  // OK

struct HasOnlyDTOROnPurpose2  // OK
{
    ~HasOnlyDTOROnPurpose2();
private:
    Q_DISABLE_COPY(HasOnlyDTOROnPurpose2)
};

struct HasNothingButNested // OK
{
    HasNothingButNested() {}
    HasCopyCtor c;
};

struct Has3ButDtorDefault
{
    ~Has3ButDtorDefault() = default;
    Has3ButDtorDefault(const Has3ButDtorDefault &);
    Has3ButDtorDefault& operator=(const Has3ButDtorDefault &);
};

struct HasEmptyDtor
{
    ~HasEmptyDtor() {} // OK
};
