# Git Commit Message Format

```
<type>(<scope>): <subject>
<HERE-SHOULD-BE-ONE-BLANK-LINE>
<body>
<HERE-SHOULD-BE-ONE-BLANK-LINE>
<footer>
```

- expect `<type>` & `<subject>`, others are optional
- All message lines prefer not being longer than 100 characters

# <type> must be one of
- **ci**: Changes to CI configuration files and scripts
- **fix**: A bug fix
- **feat**: A new feature or something
- **docs**: Documentation only changes
- **perf**: A code change that improves performance
- **test**: Adding missing tests or correcting existing tests
- **build**: Changes that affect the build system or external dependencies
- **style**: Changes that do not affect the meaning of the code (white-space, formatting, missing semi-colons, etc)
- **chore**: Other changes that don't modify src or test files
- **revert**: Reverts a previous commit
- **refactor**: A code change that neither fixes a bug nor adds a feature

# `<scope>` is optional

`<scope>` if any, it should be one word that specifying the place of the commit change, for example:

- You can use * when the change affects more than a single scope
- **module**: the commit changes related to a module
- **compile**: the commit changes related to compile

# `<subject>` is one short line of succinct description of the change

The subject following the rules:

- use the imperative, present tense: "change" not "changed" nor "changes"
- do not capitalize first letter
- no dot (.) at the end

# `<body>` is optional

`<body>` if any, it should include the motivation for the change and following the rules:

- use the imperative, present tense: "change" not "changed" nor "changes"
- what this commit changes, and why?

# `<footer>` is optional

`<footer>` if any, it should only contain information about the following ones:

- Breaking Changes, it should be the following format:
```
[BREAKING CHANGE]: a short description message
    - more details information
```

- GitHub Issues Reference that this commit closes, the format as following:
```
[CLOSES ISSUE]: a short description message contains the issue reference
```

# Reference
- [commitizen(en)][commitizen_url]
- [Conventional Commits][conventionalcommits_url]
- [Linus about git-commit][linus_about_git_commit_url]
- [Git Commit Messages: 50/72 Formatting][git_commit_msg_50_72_url]
- [How to Write a Git Commit Message][how_to_write_git_msg_url]
- [CommitMessage & ChangeLog 编写指南][gitmsg_and_changelog_url]
- [怎么写 Git Commit Message][cn_how_to_write_git_msg_url]
- [GitCommitMessage工作流规范][commitizen_usage_example_url]
- [package.json文件][what_is_package_json_url]

[commitizen_url]: http://commitizen.github.io/cz-cli
[linus_about_git_commit_url]: https://github.com/torvalds/linux/pull/17#issuecomment-5659933
[git_commit_msg_50_72_url]: https://stackoverflow.com/questions/2290016/git-commit-messages-50-72-formatting
[how_to_write_git_msg_url]: https://chris.beams.io/posts/git-commit
[gitmsg_and_changelog_url]: http://www.ruanyifeng.com/blog/2016/01/commit_message_change_log.html
[cn_how_to_write_git_msg_url]: http://www.jianshu.com/p/0117334c75fc
[commitizen_usage_example_url]: https://www.qcloud.com/community/article/509422001489391615
[what_is_package_json_url]: http://javascript.ruanyifeng.com/nodejs/packagejson.html
[conventionalcommits_url]: http://conventionalcommits.org/
