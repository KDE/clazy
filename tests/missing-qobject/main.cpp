#include <QtCore/QObject>
#include <QtCore/QFile>








class MissingMacro : public QObject
{
public: // Warning
};

class HasMacro : public QObject
{
    Q_OBJECT
public:
};

class Derived1 : public HasMacro
{
public: // Warning
};

class QFile;
class FwdDecl;
