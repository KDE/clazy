namespace clazy::VisitHelper
{

using namespace clang;

bool VisitDecl(Decl *decl, ClazyContext *context, const std::vector<CheckBase *> &checksToVisit);
bool VisitStmt(Stmt *stmt, ClazyContext *context, const std::vector<CheckBase *> &checksToVisit);

}
