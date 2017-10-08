#include <QtCore/QObject>
#include "ui_widget.h"

class MyObject : public QObject, private Ui::Widget
{
    Q_OBJECT
public:
    bool on_mButton_something(); // No warn

public Q_SLOTS:
    void slot1() const {} // No warn
    void slot2_something() {} // No warn
    void slot3_something_(); // No warn
    void on_mButton_clicked(); // Warn
    void on_button_clicked(); // Warn
    void on_mButton_enabled(); // Warn
    void on_mButton_released() {} // Warn
    void on_mButton(); // No warn
    void on_notButton_clicked(); // No warn
    void on_mDouble_clicked(); // No warn - not a widget

  private:

    double mDouble;
    QPushButton* button;

};

void MyObject::on_mButton_enabled() // OK, already warned
{

}

void test()
{
    MyObject o;
}
