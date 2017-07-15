# Git Configurations
- Make sure the repo is always using **LF**
- Make sure the working directory text files end of line is **LF** for Linux/Macos
- Make sure the working directory text files end of line is **CRLF** for Windows

# All the command you should to run
Run the following commands in the project root directory:
  - `$ git config core.safecrlf true`
  - `$ git config core.autocrlf true`, for Windows only
  - `$ git config core.autocrlf input`, for Linux/Macos only
  - `$ git config commit.template ${PWD}/.gitcommitstyle`

# How to install nodejs & npm
- install **Node.js** and **npm**
  - download **node-vx.x.x-x86/64.msi** and install for windows
  - run the following commands for ubuntu
    - `$ sudo apt install nodejs`
    - `$ sudo apt install nodejs-legacy`
  - To check is node.js and npm install correctly
    - `$ npm --version`
    - `$ node --version`

# eol = native      [default]
- when under Windows:
  working directory text files end of line using **CRLF**

- when under Linux/Macos:
  working directory text files end of line using **LF**

# autocrlf = true   [default]
- This will auto convert **LF** between `core.eol`
- when under Windows: `core.eol` by default is **CRLF**
  - commit, auto convert **CRLF** to **LF** for working directory text files
  - checkout, auto convert **LF** to **CRLF** for working directory text files

- when under Linux/Macos: `core.eol` by default is **LF**
  - commit, do no converting
  - checkout, do no converting

# To Make Sure
- accidentally get into repo's **CRLF** will be auto converted to **LF**
- nerver mixed **CRLF** and **LF** in the repo
- all the git commit message are standardized

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
