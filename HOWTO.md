
This is dumping ground with tips for developers interested in writing their own checks.


# Create a new check or fixit
Files to create or modify:

```
checks/levelX/my-check.cpp
checks/levelX/my-check.h
checks/README-my-check.md
tests/my-check/config.json
tests/my-check/main.cpp
ClazySources.cmake
checks.json
ChangeLog
README.md
```

Just add your check to `checks.json` and run `dev-scripts/generate.py --generate`
which will generate the files you need to write, and edit others for you.

## Tips

- Write the unit-test before the check

- Dump the abstract syntax tree (AST) of your unit-test:

  `./run_tests.py my-test --dump-ast`

  This creates a `main.cpp.ast` file, if you include Qt headers, the AST will be
  very big, your stuff will be at the end of the file. Use the AST to check which
  nodes you have to visit.

- You can also dump the AST of a statement in C++:
  `stmt->dump()`

  This dumps the sub-AST to stdout, having stmt as its root

- `llvm::errs()` is useful to print to stderr
  `llvm::errs() << record->getNameAsString() << "\n";`
  `llvm::errs() << stmt->getLocStart().printToString(m_ci.getSourceManager()) << "\n";`

- Look in `Utils.h`, `StringUtils.h`, `QtUtils.h`, `FixitUtils.h`, etc for helper functions you might need.

## Using ASTMatchers

- See the `qcolor-from-literal` check for how to use ASTMatchers with clazy

## Tips for fixits

- Usually you'll have to poke around and debug print `getLocStart` and `getLocEnd` of statements until
  you find the locations you want, otherwise you'll have to manipulate the locations with `Lexer`,
  see `FixItUtils.cpp`.

- Learn from existing fixits:
```
    qgetenv.cpp
    functionargsbyref.cpp
    autounexpectedqstringbuilder.cpp
    qstringref.cpp
```

# Running tests
    ./run_tests.py # This runs all tests
    ./run_tests.py my-check # This runs one tests
    ./run_tests.py my-check --verbose # Prints the compiler invocation command
    ./run_tests.py my-check --dump-ast # dumps the AST into a file with .ast extension

# Creating a release

- Clone `git@invent.kde.org:sysadmin/repo-metadata` if you haven't yet, open `dependencies/logical-module-structure.json` and update the stable
branch (search for clazy in that file).

- cd into `clazy`, from `master` create a new stable branch, called for example `1.12` (use the version you want here)

- Edit `CLAZY_VERSION_MINOR` in `CMakeLists.txt`

- Edit ChangeLog

- Merge to master, and bump `CLAZY_VERSION_MINOR` to the next version.