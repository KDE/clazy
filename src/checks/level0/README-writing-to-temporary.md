# writing-to-temporary

Catches when calling setters on temporaries.
#### Example
`widget->sizePolicy().setHorizontalStretch(1);`

Which should instead be:
```
QSizePolicy sp = widget->sizePolicy();
sp.setHorizontalStretch(1);
widget->setSizePolicy(sp);
```

#### Requirements
- The method must be of void return type, and must belong to one of these whitelisted classes:
  QList, QVector, QMap, QHash, QString, QSet, QByteArray, QUrl, QVarLengthArray, QLinkedList, QRect, QRectF, QBitmap, QVector2D, QVector3D, QVector4D, QSize, QSizeF, QSizePolicy

- Optionally, if you set env variable `CLAZY_EXTRA_OPTIONS="writing-to-temporary-widen-criteria"`, it will warn on any method with name starting with "set", regardless of it being whitelisted, example:
`foo().setBar()`
