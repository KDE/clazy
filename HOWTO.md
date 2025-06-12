
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

- If you try to run the tests from the build dir directly without installation, you may get error
  messages about not finding `ClazyPlugin.so` and/or `clazy-standalone` when you call
  `run_tests.py`. This can be resolved by exporting the following environment variables:

  `export CLAZYPLUGIN_CXX=<path-to-builddir>/lib/ClazyPlugin.so && export CLAZYSTANDALONE_CXX=<path-to-builddir>/bin/clazy-standalone`

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

- Edit ChangeLog with release date/additional info
- Make sure releases in `org.kde.clazy.metainfo.xml` are up to date. Changelog may be converted to XML format using following command: `appstreamcli news-to-metainfo --format text Changelog -`
- Create git tag with the new version
- Edit `CLAZY_VERSION_MINOR` in `CMakeLists.txt`

- Sign the files, upload them and create sysadmin ticket for putting them on https://download.kde.org/stable/clazy/.


```bash
basename="clazy-1.15"
tarfile="$basename.tar"
xzfile="$tarfile.xz"
sigfile="$xzfile.sig"
ftp_url="ftp://upload.kde.org/incoming/"

xz -zk "$tarfile"  # -k to keep original tar

gpg --output "$sigfile" --detach-sig "$xzfile"
gpg --verify "$sigfile" "$xzfile"

# For sysadmin ticket
for file in "$xzfile" "$sigfile"; do
    echo "File: $file"
    echo "SHA-1:   $(sha1sum "$file" | awk '{print $1}')"
    echo "SHA-256: $(sha256sum "$file" | awk '{print $1}')"
    echo
done

curl -T "$xzfile" "${ftp_url}${xzfile}"
curl -T "$sigfile" "${ftp_url}${sigfile}"
```


<!--- Clone `git@invent.kde.org:sysadmin/repo-metadata` if you haven't yet, open `dependencies/logical-module-structure.json` and update the stable branch (search for clazy in that file). -->
