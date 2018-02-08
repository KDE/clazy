# overridden-signal

Warns when overriding a signal, which might make existing connects not work, if done unintentionally.
Doesn't warn when the overridden signal has a different signature.

Warns for:
- Overriding signal with non-signal
- Overriding non-signal with signal
- Overriding signal with signal
