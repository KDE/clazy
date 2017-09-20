struct ConstRefMember
{
    const int& m_a;
    ConstRefMember(const int& a) : m_a(a) { } // OK. Bug #379342
};


struct ConstMember
{
    const int m_a;
    ConstMember(const int& a) : m_a(a) { } // Warn
};
