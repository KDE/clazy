# wrong-qevent-cast

Warns when a QEvent is possibly cast to the wrong derived class via static_cast.

Example:
switch (ev->type()) {
    case QEvent::MouseMove:
        auto e = static_cast<QKeyEvent*>(ev);
}

Currently only casts inside switches are verified.
