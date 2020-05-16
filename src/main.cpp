#include <git2.h>
#include <git2/errors.h>
#include <git2/rebase.h>
#include <git2/revparse.h>

#include <iostream>
#include <sstream>
#include <string_view>
#include <exception>
#include <dirent.h>

class MyGitException final : public std::logic_error
{
  using std::logic_error::logic_error;
};

void throwIfError(int error)
{
  if (error)
  {
     const git_error *e = giterr_last();
     std::stringstream stream;
     stream << "Error : " << error << '/' << e->klass << ": " << e->message;
     throw MyGitException(stream.str());
  }
}

int main(int argc, const char *argv[])
{
  /* Declaration of required variables */
  // I put there all variables to have an oppotunity to make git_*_free()
  git_repository *repo = nullptr;           // repository for rebase
  git_annotated_commit *annotCom = nullptr; // annotated commit for changing its message, used in rebase init
  git_object *OID = nullptr;                // git_oid object with hash of target commit
  git_rebase *rebase = nullptr;             // rebase object
  git_commit *com;                          // target commit, used for getting commit's author
  git_oid rebaseOID = {{0}};                // rebase's oid, required in git_rebase_commit
  git_rebase_operation *op = nullptr;       // rebase operation, required in git_rebase_next

  int returnCode = 0;

  try
  {
    /* initialization of libgit2 */
    git_libgit2_init();

    /* open target repository */
    // TODO: CHANGE next line to :throwIfError(git_repository_open(&repo, argv[0]));
    throwIfError(git_repository_open(&repo, "pb2019"));

    /* get target annotated commit */
    if (argv[1][0] == 'H')
    {
      // commit discripted by "HEAD*"
      throwIfError(git_revparse_single(&OID, repo, argv[1]));
      throwIfError(git_annotated_commit_lookup(&annotCom, repo, git_object_id(OID)));
    }
    else
    {
      // commit discripted by hash
      throwIfError(git_annotated_commit_from_revspec(&annotCom, repo, argv[1]));
    }

    /* rebase object initialization */
    throwIfError(git_rebase_init(&rebase, repo, nullptr, nullptr, annotCom, nullptr));

    /* getting commit from hash of target annotated_commit.
     * I will be used for getting commit author signature.
     */
    throwIfError(git_commit_lookup(&com, repo, git_annotated_commit_id(annotCom)));

    /* get rebase operation and change commit message */
    if (git_rebase_next(&op, rebase) == GIT_OK)
      throwIfError(git_rebase_commit(&rebaseOID, rebase, nullptr, git_commit_author(com), nullptr, argv[2]));

    /* pass other commits */
    while (git_rebase_next(&op, rebase) == GIT_OK)
      throwIfError(git_rebase_commit(&rebaseOID, rebase, nullptr, git_commit_author(com), nullptr, nullptr));
  }
  catch (const MyGitException &e)
  {
    std::cerr << e.what() << std::endl;
    returnCode = -1;
  }

  /* Deinitialization of all objects */
  git_rebase_finish(rebase, nullptr);
  git_annotated_commit_free(annotCom);
  git_commit_free(com);
  git_rebase_free(rebase);
  git_repository_free(repo);

  return returnCode;
}