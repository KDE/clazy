#include <QtWidgets/QSpinBox>

void test()
{
    QSpinBox* spinbox = new QSpinBox();
    spinbox->value();
    delete spinbox;
}
