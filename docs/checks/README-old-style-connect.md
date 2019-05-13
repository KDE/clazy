# old-style-connect

Finds usages of old style connects.
Connecting with old style syntax (`SIGNAL`/`SLOT`) is much slower than using pointer to member syntax (PMF).

Here's however a non-exhaustive list of caveats you should be aware of:
- You can't disconnect with new-syntax if the connect was made with old-syntax (and vice-versa)
- You can't disconnect from a static slot with new-syntax (although connecting works)
- Difference in behaviour when calling slots of partially destroyed objects (<https://codereview.qt-project.org/#/c/83800>)

#### Fixits

This check supports a fixit to rewrite your code to the new connect syntax. See the README.md on how to enable it.
Be careful, as PMF is not a 100% drop-in replacement and the fixit might introduce bugs.

#### Pitfalls

Although this check doesn't have false-positives it's a level2 check, that's because some connects are tricky to convert to PMF syntax and might introduce bugs if you don't know what you're doing.
