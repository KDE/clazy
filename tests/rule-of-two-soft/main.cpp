#include <QtCore/QModelIndex>
#include <QtCore/QPair>













class Empty
{
};

class HasCopyCtor
{
public:
    HasCopyCtor();
    HasCopyCtor(const HasCopyCtor &);
};

class HasAssign
{
public:
    HasAssign() {}
    HasAssign& operator=(const HasAssign &) { return *this; }
};

class HasAssignAndCopy
{
public:
    HasAssignAndCopy(const HasAssignAndCopy &);
    HasAssignAndCopy& operator=(const HasAssignAndCopy &) { return *this; }
};

class DeletedCopyCtor
{
public:
    DeletedCopyCtor(const DeletedCopyCtor &) = delete;
};

void test()
{
    HasCopyCtor hasCopyCtor;
    HasCopyCtor hasCopyCtor2;
    hasCopyCtor = hasCopyCtor2;

    HasAssign hasAssign;
    HasAssign hasAssign2 = hasAssign;

}
