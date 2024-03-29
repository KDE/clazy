#include <QtCore/QObject>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QComboBox>
#include <QtTest/QSignalSpy>
#include <QtTest/QTest>

class MyObj : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testMethod()
    {
        QObject *obj = new QObject;
        QSignalSpy destroyedSpy(obj, SIGNAL(destroyed())); // Warn
        QVERIFY(destroyedSpy.isValid());

        QLineEdit lineEdit;
        QSignalSpy textChangedSpy(&lineEdit, SIGNAL(textChanged(QString))); // Warn
        QVERIFY(textChangedSpy.isValid());

        QComboBox combo;
        QSignalSpy activatedIntSpy(&combo, SIGNAL(activated(int))); // Warn and no fixit due to overloads in Qt5
        QVERIFY(activatedIntSpy.isValid());
        QSignalSpy activatedStringSpy(&combo, SIGNAL(activated(QString))); // Warn and no fixit due to overloads in Qt5
        QVERIFY(activatedStringSpy.isValid());
    }
};

int main() { return 0; }
