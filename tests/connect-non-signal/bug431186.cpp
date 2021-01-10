#include <QtWidgets/QTreeWidget>

class MyTree : public QTreeView
{
public:
    MyTree() {
        connect(this, &QTreeWidget::entered, this, []{});
    }
};
