# qrequiredresult-candidates

Suggests methods that could use `Q_REQUIRED_RESULT`.
The criteria is:
- private const methods,
- returning the same type as their class,
- and ending with 'ed'

For example:
`Q_REQUIRED_RESULT QRect intersected(const QRect &other) const`
