#include <QtCore/QObject>
#include <bitset>

class SignalWithExpressionInGeneric : public QObject
{
    Q_OBJECT

Q_SIGNALS:
    void valueChanged(std::bitset<int(8)>);
};


