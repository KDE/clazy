unset CLAZY_CHECKS
unset CLAZY_FIXIT
CLAZY_COMMAND="clazy -c -o /dev/null -xc++ -Xclang -plugin-arg-clang-lazy -Xclang print-requested-checks "
CLAZY_COMMAND_STDIN=$CLAZY_COMMAND"-"

# Test without checks:
echo | $CLAZY_COMMAND_STDIN

# Test with invalid check:
export CLAZY_CHECKS="foo"
echo | $CLAZY_COMMAND_STDIN

# Test with 1 check specified through env variable
export CLAZY_CHECKS="foreach"
echo | $CLAZY_COMMAND_STDIN

# Test with 2 checks specified through env variable
export CLAZY_CHECKS="foreach,writing-to-temporary"
echo | $CLAZY_COMMAND_STDIN

# Test with 2 checks specified through env variable plus one error
export CLAZY_CHECKS="foreach,writing-to-temporary,foo"
echo | $CLAZY_COMMAND_STDIN

# Test that fixit enables the check
unset CLAZY_CHECKS
export CLAZY_FIXIT="fix-old-style-connect"
echo | $CLAZY_COMMAND_STDIN

# Test both check and fixit
export CLAZY_CHECKS="old-style-connect"
export CLAZY_FIXIT="fix-old-style-connect"
echo | $CLAZY_COMMAND_STDIN

# Test fixit+check + unrelated check
export CLAZY_CHECKS="old-style-connect,foreach"
export CLAZY_FIXIT="fix-old-style-connect"
echo | $CLAZY_COMMAND_STDIN

# test all_checks
unset CLAZY_FIXIT
export CLAZY_CHECKS="all_checks"
echo | $CLAZY_COMMAND_STDIN

unset CLAZY_FIXIT
unset CLAZY_CHECKS

# Test specifying check in command line
echo | $($CLAZY_COMMAND" -Xclang -plugin-arg-clang-lazy -Xclang implicit-casts -")

# Pass two checks in command line
echo | $($CLAZY_COMMAND" -Xclang -plugin-arg-clang-lazy -Xclang implicit-casts,foreach -")

# Pass fixits through the command-line
echo | $($CLAZY_COMMAND" -Xclang -plugin-arg-clang-lazy -Xclang fix-old-style-connect -")

# Pass level0
echo | $($CLAZY_COMMAND" -Xclang -plugin-arg-clang-lazy -Xclang level0 -")

# Pass level1
echo | $($CLAZY_COMMAND" -Xclang -plugin-arg-clang-lazy -Xclang level1 -")

# Pass level0 + another check
echo | $($CLAZY_COMMAND" -Xclang -plugin-arg-clang-lazy -Xclang reserve-candidates -Xclang -plugin-arg-clang-lazy -Xclang level0 -")

# Pass level0 + another check that's already in level0
echo | $($CLAZY_COMMAND" -Xclang -plugin-arg-clang-lazy -Xclang qdatetime-utc -Xclang -plugin-arg-clang-lazy -Xclang level0 -")

# Use a level argument in the checks list
echo | $($CLAZY_COMMAND" -Xclang -plugin-arg-clang-lazy -Xclang implicit-casts,foreach,level0 -")

# Use a level in env-variable
export CLAZY_CHECKS="level1"
echo | $CLAZY_COMMAND_STDIN

# Use a level in env-variable + another check
export CLAZY_CHECKS="level0,reserve-candidates"
echo | $CLAZY_COMMAND_STDIN

# Use both env variable and compiler argument
export CLAZY_CHECKS="level0,reserve-candidates"
echo | $($CLAZY_COMMAND" -Xclang -plugin-arg-clang-lazy -Xclang implicit-casts,level0 -")
