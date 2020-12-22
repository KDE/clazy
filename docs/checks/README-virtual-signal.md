# virtual-signal

Warns when a signal is virtual.


Virtual pointers don't compare properly on Windows/MSVC when crossing DLL boundaries.
The pointer points to a DLL-local stub which does the actual call.

Also, virtual signals make it very hard to read connect statements since people don't
know they are virtual, and don't expect them to be.

moc also discourages the use of virtual signals, by printing a non-fatal warning:
`Warning: Signals cannot be declared virtual`

If you really need virtual signals see https://github.com/KDAB/KDDockWidgets/pull/124/commits/7a2ffa030b1b2089ee4cc000a5cc6cf7d1653785
for a workaround
