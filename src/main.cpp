#include <git2.h>
#include <git2/errors.h>
#include <git2/rebase.h>
#include <git2/revparse.h>

#include <iostream>
#include <sstream>
#include <string_view>
#include <exception>
#include <unistd.h>

/* Exception class for catching libgit2 errors */
class MyGitException final : public std::logic_error
{
  using std::logic_error::logic_error;
};

/* throw exception by error function */
void throwIfError(int error)
{
  if (error)
  {
    // getting error object
     const git_error *e = giterr_last();
     std::stringstream stream; // stream for error message

     // make message and throw
     stream << "Error : " << error << '/' << e->klass << ": " << e->message;
     throw MyGitException(stream.str());
  }
}

int main(int argc, const char *argv[])
{
  /* Declaration of required variables */
  /// I put there some variables to have an oppotunity to make git_*_free()
  git_repository *repo = nullptr;           // repository for rebase
  git_annotated_commit
    *parentAnnotCom = nullptr;              // parent annotated commit of target commit, used in rebase init
  git_rebase *rebase = nullptr;             // rebase object
  git_commit
    *targeCommit,                           // target commit, used for getting parent commit
    *parentCommit;                          // parent of target commit
  git_oid rebaseOID = {{0}};                // rebase's oid, required in git_rebase_commit
  git_rebase_operation *op = nullptr;       // rebase operation, required in git_rebase_next
  int returnCode = 0;                       // return code, 0 if success, -1 otherwise
  char curDirPath[10000];                   // current directory path

  try
  {
    /* initialization of libgit2 */
    git_libgit2_init();

    // getting current directory path
    getcwd(curDirPath, 10000);

    /* open target repository */
    throwIfError(git_repository_open(&repo, curDirPath));

    /* get target annotated commit */
    if (argv[1][0] == 'H')
    {
      /* commit discripted by "HEAD*" */
      git_object *OIDobj = nullptr; // git_oid object with hash of target commit

      // parse HEAD* and get hash of commit
      throwIfError(git_revparse_single(&OIDobj, repo, argv[1]));
      // get target commit
      throwIfError(git_commit_lookup(&targeCommit, repo, git_object_id(OIDobj)));
    }
    else
    {
      /* commit discripted by hash */
      git_oid OID; // git_oid with hash of target commit

      // get commit's git_oid
      throwIfError(git_oid_fromstr(&OID, argv[1]));
      // get target commit by hash
      throwIfError(git_commit_lookup(&targeCommit, repo, &OID));
    }
    // get parent of target commit
    throwIfError(git_commit_parent(&parentCommit, targeCommit, 0));
    // get parent annotated commit
    throwIfError(git_annotated_commit_lookup(&parentAnnotCom, repo, git_commit_id(parentCommit)));

    /* rebase object initialization */
    throwIfError(git_rebase_init(&rebase, repo, nullptr, nullptr, parentAnnotCom, nullptr));

    /* get rebase operation and change message of target commit*/
    throwIfError(git_rebase_next(&op, rebase));
    throwIfError(git_rebase_commit(&rebaseOID, rebase, nullptr, git_commit_author(parentCommit), nullptr, argv[2]));

    /* pass other commits */
    while (git_rebase_next(&op, rebase) == GIT_OK)
      throwIfError(git_rebase_commit(&rebaseOID, rebase, nullptr, git_commit_author(parentCommit), nullptr, nullptr));
  }
  catch (const MyGitException &e)
  {
    std::cerr << e.what() << std::endl;
    returnCode = -1;
  }

  /* Deinitialization of all objects */
  git_rebase_finish(rebase, nullptr);
  git_annotated_commit_free(parentAnnotCom);
  git_commit_free(targeCommit);
  git_commit_free(parentCommit);
  git_rebase_free(rebase);
  git_repository_free(repo);

  return returnCode;
}