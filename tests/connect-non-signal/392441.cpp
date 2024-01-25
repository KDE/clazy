#include <QtWidgets/QComboBox>

void test_bug392441()
{
    QComboBox *combo;
    QObject::connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), [] (int) {}); // OK
}
