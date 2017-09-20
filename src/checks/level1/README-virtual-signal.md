# virtual-signal

Warns when a signal is virtual.
Virtual signals make it very hard to read connect statements since people don't
know they are virtual, and don't expect them to be.

moc also discourages the use of virtual signals, by printing a non-fatal warning:
`Warning: Signals cannot be declared virtual`
