#include <QtWidgets/QComboBox>

void test_bug392441()
{
    QComboBox *combo;
    QObject::connect(combo, QOverload<const QString &>::of(&QComboBox::currentIndexChanged), [] (QString) {}); // OK
}
