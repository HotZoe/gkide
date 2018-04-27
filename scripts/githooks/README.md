Those are the git hooks scripts for local use, so all
things will be consistent and make it simple.

# How To Use These Hooks
- for git older than 2.9.0, just copy those files to the `xx/.git/hooks/`
- for git newer than 2.9.0, run `git config core.hooksPath path/to/githooks`

# About Git Hooks
As of [2.9.0 the docs address][git_scm_docs_githooks_url]:

Before Git invokes a hook, it changes its working directory to either the root of
the working tree in a non-bare repository, or to the `$GIT_DIR` in a bare repository.

[git_scm_docs_githooks_url]: https://git-scm.com/docs/githooks/2.9.0

# Reference
- [Git config(en)][git_config_url]
- [Missing Git Hooks Docs][missing_git_hooks_docs_url]

[git_config_url]: https://git-scm.com/docs/git-config
[missing_git_hooks_docs_url]: https://longair.net/blog/2011/04/09/missing-git-hooks-documentation
