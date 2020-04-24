
// Crude approximation of #include <QAndroidJniObject> so we don't need QtAndroidExtras installed
class QAndroidJniObject
{
    public:
    QAndroidJniObject(const char *className, const char *signature);

    template <typename T>
    void callObjectMethod(const char *methodName, const char *signature);

    template <typename T>
    void callMethod(const char *methodName, const char *signature);

    template <typename T>
    static void callStaticMethod(const char *methodName, const char *signature);

    template <typename T>
    static void callStaticObjectMethod(const char *methodName, const char *signature);
};

int main() {

    QAndroidJniObject obj("java/lang/String", "(");
    obj.callObjectMethod<int>("someMethod", "()V");
    obj.callObjectMethod<int>("someMethod", "(III)V");
    obj.callObjectMethod<int>("someMethod", "(III");

    obj.callMethod<int>("someMethod", "()V");
    obj.callMethod<int>("someMethod", "(III)V");
    obj.callMethod<int>("someMethod", "()Ljava/lang/String");

    QAndroidJniObject::callStaticMethod<int>("someMethod", "()Ljava/lang/String");

    QAndroidJniObject::callStaticObjectMethod<int>("someOtherMethod", "(II)");
}
