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
  try
  {
    git_libgit2_init();

    git_repository *repo = nullptr;
    //throwIfError(git_repository_open(&repo, argv[0]));
    throwIfError(git_repository_open(&repo, "pb2019"));

    git_annotated_commit *annotCom = nullptr;
    if (argv[1][0] == 'H')
    {
      git_object *obj = nullptr;
      throwIfError(git_revparse_single(&obj, repo, argv[1]));
      throwIfError(git_annotated_commit_lookup(&annotCom, repo, git_object_id(obj)));
    }
    else
      throwIfError(git_annotated_commit_from_revspec(&annotCom, repo, argv[1]));
    
    git_rebase *rebase = nullptr;
    throwIfError(git_rebase_init(&rebase, repo, nullptr, nullptr, annotCom, nullptr));

    git_commit *com;
    throwIfError(git_commit_lookup(&com, repo, git_annotated_commit_id(annotCom)));

    git_oid rebaseOID = {{0}};
    git_rebase_operation *op = nullptr;
    if (git_rebase_next(&op, rebase) == GIT_OK)
    {
      op->type = GIT_REBASE_OPERATION_REWORD;
      throwIfError(git_rebase_commit(&rebaseOID, rebase, nullptr, git_commit_author(com), nullptr, argv[2]));
    }
    while (git_rebase_next(&op, rebase) == GIT_OK)
    {
      op->type = GIT_REBASE_OPERATION_PICK;
      throwIfError(git_rebase_commit(&rebaseOID, rebase, nullptr, git_commit_author(com), nullptr, "WHAT"));
    }
    git_rebase_finish(rebase, nullptr);
    git_rebase_free(rebase);

    git_repository_free(repo);
    //std::cout << git_rebase_operation_entrycount(rebase) << std::endl;
  }

  catch (const MyGitException &e)
  {
    std::cerr << e.what() << std::endl;
    return -1;
  }

  return 0;
}