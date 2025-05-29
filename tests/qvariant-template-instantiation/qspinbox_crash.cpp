#include <QtWidgets/QSpinBox>

void test()
{
    QSpinBox* spinbox = new QSpinBox();
    spinbox->value(); // OK, no crash
    delete spinbox;
}
