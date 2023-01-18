# qhash-namespace

Warns when a `qHash()` function is not declared inside the namespace of its argument.
`qHash()` needs to be inside the namespace for ADL lookup to happen.
