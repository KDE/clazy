# lambda-in-connect

Warns when a lambda inside a connect captures local variables by reference.
This usually results in a crash since the lambda might get called after the captured variable went out of scope.

#### Example:
````
    int a;
    connect(obj, &MyObj::mySignal, [&a]{ ... });
````
Although it's dangerous to capture by reference in other situations too, this check only warns for
connects, otherwise it would generate false-positives in legitimate situations where you only
use the lambda before going out of scope.
