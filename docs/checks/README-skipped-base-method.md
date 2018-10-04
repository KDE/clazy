# skipped-base-method

Warns when calling a method from the "grand-base class" instead of the base-class method.

Example:
class MyFrame : public QFrame
{
    Q_OBJECT
public:
    bool event(QEvent *ev) override
    {
        (...)
        return QWidget::event(ev); // warning: Maybe you meant to call QFrame::event() instead [-Wclazy-skipped-base-method]
    }
};

Try to avoid jumping over the direct base method. If you really need to then at least
add a comment in the code, so people know it was intentional. Or even better, an clazy:exclude=skipped-base-method comment, which also silences this warning.
