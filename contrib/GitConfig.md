# Git Configurations
- Make sure the repo is always using **LF**
- Make sure all the git commit message are standardized
- Make sure nerver mixed **CRLF** and **LF** in the repo
- Make sure the working directory text's EOL is always using **LF**
- Make sure accidentally get into repo's **CRLF** will be auto converted to **LF**

# All the command you should to run
Run the following commands in the project root directory:
  - `$ git config core.eol lf`, make sure working directory file's EOL is always **LF**
  - `$ git config core.safecrlf true`, make sure no mixing **LF** and **CRLF**
  - `$ git config core.autocrlf input`, make sure convert **CRLF** to **LF** if ayn.
  - `$ git config commit.template ${PWD}/.gitcommitstyle`

# Install nodejs & npm

[node_js_url]: https://nodejs.org/en/
[node_js_npm_url]: https://www.npmjs.com/
[validate_commit_msg_url]: https://github.com/conventional-changelog/validate-commit-msg
[standard_version_url]: https://github.com/conventional-changelog/standard-version

- install [node.js][node_js_url] and [npm][node_js_npm_url]
  - download **node-vx.x.x-x86/64.msi** and install for windows
  - run the following commands for ubuntu
    - `$ sudo apt install nodejs`
    - `$ sudo apt install nodejs-legacy`
  - To check is node.js and npm install correctly
    - `$ npm --version`
    - `$ node --version`
- install [validate-commit-msg][validate_commit_msg_url]
  - `$ npm install -g validate-commit-msg`
- install [standard-version][standard_version_url]
  - `$ npm install -g standard-version`

# About Git Hooks
As of [2.9.0 the docs address][git_scm_docs_githooks_url]:

Before Git invokes a hook, it changes its working directory to either the root of the working tree
in a non-bare repository, or to the `$GIT_DIR` in a bare repository.

[git_scm_docs_githooks_url]: https://git-scm.com/docs/githooks/2.9.0

# Reference
- [Git config(en)][git_config_url]
- [Missing Git Hooks Docs][missing_git_hooks_docs_url]

[git_config_url]: https://git-scm.com/docs/git-config
[missing_git_hooks_docs_url]: https://longair.net/blog/2011/04/09/missing-git-hooks-documentation
