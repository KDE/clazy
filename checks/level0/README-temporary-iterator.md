# temporary-iterator

Detects when you're using using functions returning iterators (eg. `begin()` or `end()`) on a temporary container.

#### Example

    // temporary list returned by function
    QList<type> getList()
    {
        QList<type> list;
        ... add some items to list ...
        return list;
    }

    // Will cause a crash if iterated using:

    for (QList<type>::iterator it = getList().begin(); it != getList().end(); ++it)
    {
      ...
    }

because the end iterator was returned from a different container object than the begin iterator.
