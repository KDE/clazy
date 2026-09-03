#include <QtCore/QAbstractItemModel>
#include <QtCore/QObject>
#include <QtCore/QString>

enum AnimalRoles { TypeRole = Qt::UserRole + 1, SizeRole };

class MyModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    QHash<int, QByteArray> roleNames() const override
    {
        QHash<int, QByteArray> roles;
        roles[TypeRole] = "type";
        roles[SizeRole] = "size";
        return roles;
    }
};

class MyModelDirectReturn : public QAbstractItemModel
{
    Q_OBJECT
public:
    QHash<int, QByteArray> roleNames() const override
    {
        return {
            {TypeRole, "type"},
            {SizeRole, "size"},
        };
    }
};

class MyModelCached : public QAbstractItemModel
{
    Q_OBJECT
public:
    QHash<int, QByteArray> roleNames() const override
    {
        static const QHash<int, QByteArray> roles{
            {TypeRole, "type"},
            {SizeRole, "size"},
        };

        return roles;
    }
};
