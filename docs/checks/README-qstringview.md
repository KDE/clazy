# qstring-ref

Finds places where `QString::fooRef()` should be used instead of `QString::foo()`, to avoid temporary heap allocations.

#### Example

    str.mid(5).toInt(ok) // BAD

    str.midRef(5).toInt(ok) // GOOD

Where `mid` can be any of: `mid`, `left`, `right`.
And `toInt()` can be any of: `compare`, `contains`, `count`, `startsWith`, `endsWith`, `indexOf`, `isEmpty`, `isNull`, `lastIndexOf`, `length`, `size`, `to*`, `trimmed`

#### FixIts

This check supports a fixit to rewrite your code. See the README.md on how to enable it.
