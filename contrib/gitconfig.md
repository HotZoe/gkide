# Git Configurations

- Make sure the repo is always using **LF**
- Make sure the working directory text files end of line is **LF** for Linux/Macos
- Make sure the working directory text files end of line is **CRLF** for Windows

# All the command you should to run:

## Windows:
  - ```$ git config --global core.safecrlf=true```
  - ```$ git config commit.template ${PWD}/.gitcommitstyle```

## Linux/Macos:
  - ```$ git config --global core.safecrlf=true```
  - ```$ git config --global core.autocrlf=input```
  - ```$ git config commit.template ${PWD}/.gitcommitstyle```

# eol = native      [default]

- when under Windows:
  working directory text files end of line using **CRLF**

- when under Linux/Macos:
  working directory text files end of line using **LF**

# autocrlf = true   [default]

- This will auto convert **LF** between ```core.eol```
- when under Windows: ```core.eol``` by default is **CRLF**
  - commit, auto convert **CRLF** to **LF** for working directory text files
  - checkout, auto convert **LF** to **CRLF** for working directory text files

- when under Linux/Macos: ```core.eol``` by default is **LF**
  - commit, do no converting
  - checkout, do no converting

# To Make Sure
  - accidentally get into repo's **CRLF** will be auto converted to **LF**
  - nerver mixed **CRLF** and **LF** in the repo
  - all the git commit message are standardized

