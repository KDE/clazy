# const-signal-or-slot

Warns when a signal or non-void slot is const.

This aims to prevent unintentionally marking a getter as slot, or connecting to
the wrong method. For signals, it's just pointless to mark them as const.

Warns for the following cases:

- non-void const method marked as slot
- const method marked as signal
- connecting to a method which isn't marked as slot, is const and returns non-void

For exposing methods to QML prefer either Q_PROPERTY or Q_INVOKABLE.
