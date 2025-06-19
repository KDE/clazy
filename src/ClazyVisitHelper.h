namespace clazy::VisitHelper
{

using namespace clang;

bool VisitDecl(Decl *decl, ClazyContext *context, const std::vector<CheckBase *> &checksToVisit, const std::vector<CheckBase *> &checksToVisitAllTypedefs);
bool VisitStmt(Stmt *stmt, ClazyContext *context, const std::vector<CheckBase *> &checksToVisit);

}
