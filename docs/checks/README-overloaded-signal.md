# overloaded-signal

Warns when signals are overloaded (two signals with different signature in the same class).
Overloaded signals requires annoying casts during connects, which are not very elegant.

Example of ugly connect:
```
connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
      [=](int exitCode, QProcess::ExitStatus exitStatus){ /* ... */ });
```
