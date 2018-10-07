# skipped-base-method

Warns when calling a method from the "grand-base class" instead of the base-class one.

Example:
```
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
```

If you really need jump over the direct base-method then at least add a comment in the code, to provide intention. Or even better, a `// clazy:exclude=skipped-base-method` comment, which also silences this warning.

This check might get removed in the future, as clang-tidy recently got a similar [feature](https://clang.llvm.org/extra/clang-tidy/checks/bugprone-parent-virtual-call.html).
