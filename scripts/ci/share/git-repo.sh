SKIP_ROBOT_COMMIT_PUSH="true"

# check if github token is set or not
function has_gh_token() 
{
    (
        set +o xtrace
        2>&1 >/dev/null test -n "$GH_TOKEN"
    )
}

# config 'robot' git user for future use
function robot_git_user_config()
{
    if [ "${CONTINUOUS_INTEGRATION}" = "true" -a has_gh_token ]; then
        SKIP_ROBOT_COMMIT_PUSH="false"
    
        local git_usr_name="gkide-robot"
        local git_usr_email="ifcoding@163.com"
    
        git config --local user.name "${git_usr_name}"
        git config --local user.email "${git_usr_email}"
        
        # ~/.netrc for 'git push' using https
        printf "machine github.com\n"         >> "$HOME/.netrc";
        printf "login ${GH_TOKEN:-unknown}\n" >> "$HOME/.netrc";
        printf "password x-oauth-basic\n\n"   >> "$HOME/.netrc";

        printf "machine api.github.com\n"     >> "$HOME/.netrc";
        printf "login ${GH_TOKEN:-unknown}\n" >> "$HOME/.netrc";
        printf "password x-oauth-basic\n\n"   >> "$HOME/.netrc";

        printf "machine uploads.github.com\n" >> "$HOME/.netrc";
        printf "login ${GH_TOKEN:-unknown}\n" >> "$HOME/.netrc";
        printf "password x-oauth-basic\n"     >> "$HOME/.netrc";

        return
    fi
}
robot_git_user_config

# Commit and push to a Git repo.
# ${1}: variable prefix.
# ${2}: git add target subtree or file
function git_repo_commit_push()
{
    local prefix="${1}"
    local repo_dir="${prefix}_DIR"
    local repo_url="${prefix}_REPO"
    local repo_branch="${prefix}_BRANCH"
    local add_target="$2"

    cd ${!repo_dir} || return

    # add changed subtree
    git add "${add_target}"

    # CI build and commit
    local commit_msg="robot: update ${add_target} at $(date "+%Y-%m-%d %z")"
    git commit -m "${commit_msg}" || true

    local try_cnt=10

    while test $(( try_cnt-=1 )) -gt 0 ; do
        if git pull --rebase "${!repo_url}" "${!repo_branch}"; then
            if git push ${!repo_url} ${!repo_branch}; then
                echo "Pushed to ${!repo_url}:${!repo_branch}"
                return 0
            fi
        fi

        echo "Retry[${try_cnt}] pushing to ${!repo_url}:${!repo_branch}"
        # Pause for 2 seconds, then retry
        sleep 2
    done

    return 1
}

# Clone a Git repo.
# ${1}: Variable prefix.
function git_repo_clone()
{
    local prefix="${1}"
    local repo_dir="${prefix}_DIR"
    local repo_url="${prefix}_REPO"
    local repo_branch="${prefix}_BRANCH"

    if ! [ -d ${!repo_dir} ] ; then
        # CI build or repo not exist
        rm -rf ${!repo_dir}
        git clone --depth=1 --branch ${!repo_branch} ${!repo_url} ${!repo_dir}
    fi
}

# Clone a Git repo.
# ${1}: Variable prefix.
# ${2}: Branch
# ${3}: Subtree
function git_repo_sha1()
{
    local prefix="${1}"
    local repo_branch="$2"
    local repo_subtree="$3"
    local repo_dir="${prefix}_DIR"

    if ! [ -d ${!repo_dir} ] ; then
        # if not exist, clone now
        git_repo_clone ${prefix}
    fi

    local repo_sha1=""
    # double check for just in case
    if [ -d ${!repo_dir} ] ; then
        # get the given subtree commit full SHA-1
        cd ${!repo_dir} > /dev/null
        git checkout ${repo_branch} --force > /dev/null 2>&1
        repo_sha1="$(git --git-dir=${!repo_dir}/.git log --pretty=oneline -1 ${repo_subtree})"
        cd - > /dev/null
    fi

    echo "$(echo ${repo_sha1} | cut -d ' ' -f 1)"
}

# clone 'autodocs'
function clone_autodocs()
{
    git_repo_clone AUTODOCS
}

# commit and push
# ${1}: git add target subtree or file
function commit_push_autodocs()
{
    git_repo_commit_push AUTODOCS "$1"
}

# upload gkide build log to 'autodocs/dev/log/...'
function update_autodocs_dev_log()
{
    if [ "${SKIP_ROBOT_COMMIT_PUSH}" = "true" -o "${UPLOAD_BUILD_LOG}" != "ON" ]; then
        # skip upload build log:
        # - for just skip it
        # - for missing private github token
        return
    fi

    clone_autodocs

    local build_log="dev/log/ci_${TRAVIS_OS_NAME}-gkide_build.log"

    cp "${GKIDESRC_DIR}/build.log" "${AUTODOCS_DIR}/${build_log}"
    
    commit_push_autodocs "${build_log}"
}
