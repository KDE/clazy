unset CLAZY_CHECKS

if [ -z "${CLAZY_CXX}" ]; then
    CLAZY_CXX=clazy
fi

CLAZY_COMMAND="${CLAZY_CXX} -c -o /dev/null -xc++ -Xclang -plugin-arg-clazy -Xclang print-requested-checks "
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
echo | $CLAZY_COMMAND_STDIN

# Test both check and fixit
export CLAZY_CHECKS="old-style-connect"
echo | $CLAZY_COMMAND_STDIN

# Test fixit+check + unrelated check
export CLAZY_CHECKS="old-style-connect,foreach"
echo | $CLAZY_COMMAND_STDIN

# test all_checks
export CLAZY_CHECKS="all_checks"
echo | $CLAZY_COMMAND_STDIN

unset CLAZY_CHECKS

# Test specifying check in command line
echo | $($CLAZY_COMMAND -Xclang -plugin-arg-clazy -Xclang implicit-casts -)

# Pass two checks in command line
echo | $($CLAZY_COMMAND -Xclang -plugin-arg-clazy -Xclang implicit-casts,foreach -)

# Pass fixits through the command-line
echo | $($CLAZY_COMMAND -Xclang -plugin-arg-clazy -Xclang fix-old-style-connect -)

# Pass level0
echo | $($CLAZY_COMMAND -Xclang -plugin-arg-clazy -Xclang level0 -)

# Pass level1
echo | $($CLAZY_COMMAND -Xclang -plugin-arg-clazy -Xclang level1 -)

# Pass level0 + another check
echo | $($CLAZY_COMMAND -Xclang -plugin-arg-clazy -Xclang reserve-candidates -Xclang -plugin-arg-clazy -Xclang level0 -)

# Pass level0 + another check that's already in level0
echo | $($CLAZY_COMMAND -Xclang -plugin-arg-clazy -Xclang qdatetime-utc -Xclang -plugin-arg-clazy -Xclang level0 -)

# Use a level argument in the checks list
echo | $($CLAZY_COMMAND -Xclang -plugin-arg-clazy -Xclang implicit-casts,foreach,level0 -)

# Use a level in env-variable
export CLAZY_CHECKS="level1"
echo | $CLAZY_COMMAND_STDIN

# Should also work with quotes. Users sometimes add quotes in QtCreator.
echo Test9
export CLAZY_CHECKS=\"level1\"
echo | $CLAZY_COMMAND_STDIN

# Use a level in env-variable + another check
export CLAZY_CHECKS="level0,reserve-candidates"
echo | $CLAZY_COMMAND_STDIN

# Use both env variable and compiler argument
export CLAZY_CHECKS="level0,reserve-candidates"
echo | $($CLAZY_COMMAND -Xclang -plugin-arg-clazy -Xclang implicit-casts,level0 -)

unset CLAZY_CHECKS

# Test disabling checks works
echo | $($CLAZY_COMMAND -Xclang -plugin-arg-clazy -Xclang implicit-casts,foreach,no-foreach -)
echo | $($CLAZY_COMMAND -Xclang -plugin-arg-clazy -Xclang implicit-casts,no-foreach -)
echo | $($CLAZY_COMMAND -Xclang -plugin-arg-clazy -Xclang implicit-casts,no-implicit-casts -)
echo | $($CLAZY_COMMAND -Xclang -plugin-arg-clazy -Xclang level0,no-qenums,no-qgetenv -)

# Test disabling checks works, now with env variables
export CLAZY_CHECKS="implicit-casts,foreach,no-foreach"
echo | $CLAZY_COMMAND_STDIN

export CLAZY_CHECKS="implicit-casts,no-foreach"
echo | $CLAZY_COMMAND_STDIN

export CLAZY_CHECKS="implicit-casts,no-implicit-casts"
echo | $CLAZY_COMMAND_STDIN

export CLAZY_CHECKS="level0,no-qenums,no-qgetenv"
echo | $CLAZY_COMMAND_STDIN

export CLAZY_CHECKS="no-qenums"
echo | $CLAZY_COMMAND_STDIN

