/// @file plugins/bin/snail/ssh/client.cpp

#include <stdio.h>
#include <string.h>

#include "plugins/bin/snail/attributes.h"
#include "plugins/bin/snail/ssh/client.h"

namespace SnailNvimQt {

SshClient::SshClient(SshAuthInfo auth)
    : m_req_id(0), m_auth(auth),
      m_session(NULL), m_channel(NULL), m_helper(NULL)
{
    /* nothing to do */
}

SshClient::~SshClient(void)
{
    delete m_helper;
}

bool SshClient::login(void)
{
    m_session = sshSessionInit();
    if(NULL == m_session)
    {
        return false;
    }

    m_channel = newChannel();
    if(NULL == m_channel)
    {
        return false;
    }

    m_helper = new SshClientHelper(this);

    return true;
}

quint64 SshClient::newRequestId(void)
{
    return m_req_id++;
}

ssh_channel SshClient::newChannel(void)
{
    if(NULL == m_session)
    {
        return NULL;
    }

    m_channel = ssh_channel_new(m_session);;
    if(NULL == m_channel)
    {
        ssh_disconnect(m_session);
        ssh_free(m_session);
        ssh_finalize();
        return NULL;
    }

    return m_channel;
}

SshAuthInfo SshClient::getAuthInfo(void) const
{
    return m_auth;
}

SshRequest *SshClient::remoteExecute(QString cmd, QString args)
{
    FUNC_ARGS_UNUSED_TODO(args);

    if(cmd.isEmpty())
    {
        return NULL;
    }

    quint64 req_id = newRequestId();
    SshRequest *req = new SshRequest(req_id, m_session, m_channel);
    m_requests.insert(req_id, req);

    return req;
}

SshRequest *SshClient::remoteExecute(ssh_channel chl, QString cmd, QString args)
{
    FUNC_ARGS_UNUSED_TODO(args);

    if(NULL == chl || cmd.isEmpty())
    {
        return NULL;
    }

    return NULL;
}

bool SshClient::verifyKnownhost(ssh_session session) const
{
    int state;
    char *hexa;
    char buf[10];
    unsigned char *hash = NULL;

    size_t hlen;
    ssh_key srv_pubkey;

    int ret_val;

    state = ssh_is_server_known(session);

    ret_val = ssh_get_publickey(session, &srv_pubkey);
    if(SSH_OK != ret_val)
    {
        return false;
    }

    ret_val = ssh_get_publickey_hash(srv_pubkey,
                                     SSH_PUBLICKEY_HASH_SHA1,
                                     &hash,
                                     &hlen);
    ssh_key_free(srv_pubkey);
    if(SSH_OK != ret_val)
    {
        return false;
    }

    switch(state)
    {
        case SSH_SERVER_KNOWN_OK:
        {
            break; /* ok */
        }
        case SSH_SERVER_KNOWN_CHANGED:
        {
            hexa = ssh_get_hexa(hash, hlen);
            qDebug() << "Host key for server changed, server's one is now:\n"
                     << "Public key hash: " << hexa;
            qDebug() << "For security reason, connection will be stopped";
            ssh_string_free_char(hexa);
            ssh_clean_pubkey_hash(&hash);
            return false;
        }
        case SSH_SERVER_FOUND_OTHER:
        {
            qDebug() << "The host key for this server was not found but an "
                        "other type of key exists. An attacker might change "
                        "the default server key to confuse your client into "
                        "thinking the key does not exist. We advise you to "
                        "rerun the client with -d or -r for more safety.";
            return false;
        }
        case SSH_SERVER_FILE_NOT_FOUND:
        {
            qDebug() << "Could not find known host file. If you accept the "
                        "host key here, the file will be automatically created.";
            /* fallback to SSH_SERVER_NOT_KNOWN behavior */
        }
        case SSH_SERVER_NOT_KNOWN:
        {
            hexa = ssh_get_hexa(hash, hlen);
            qDebug() << "The server is unknown. Do you trust the host key ?\n"
                        "Public key hash: " << hexa;
            ssh_string_free_char(hexa);

            if(fgets(buf, sizeof(buf), stdin) == NULL)
            {
                ssh_clean_pubkey_hash(&hash);
                return false;
            }

            if(strncasecmp(buf, "yes", 3) != 0)
            {
                ssh_clean_pubkey_hash(&hash);
                return false;
            }

            fprintf(stderr, "This new key will be written on disk "
                            "for further usage. do you agree ?\n");
            if(fgets(buf, sizeof(buf), stdin) == NULL)
            {
                ssh_clean_pubkey_hash(&hash);
                return false;
            }

            if(strncasecmp(buf, "yes", 3) == 0)
            {
                if(SSH_OK != ssh_write_knownhost(session))
                {
                    ssh_clean_pubkey_hash(&hash);
                    qDebug() << "Error: " << strerror(errno);
                    return false;
                }
            }
            break;
        }
        case SSH_SERVER_ERROR:
        {
            ssh_clean_pubkey_hash(&hash);
            qDebug() << "Error: " << ssh_get_error(session);
            return false;
        }
    }

    ssh_clean_pubkey_hash(&hash);

    return true;
}

ssh_auth_e SshClient::authKeyboardInteractive(ssh_session session,
                                              const char *password) const
{
    int err = ssh_userauth_kbdint(session, NULL, NULL);
    while(err == SSH_AUTH_INFO)
    {
        int i;
        int n;
        char buffer[128];
        const char *name;
        const char *instruction;

        name = ssh_userauth_kbdint_getname(session);
        instruction = ssh_userauth_kbdint_getinstruction(session);
        n = ssh_userauth_kbdint_getnprompts(session);

        if(name && strlen(name) > 0)
        {
            qDebug() << name;
        }

        if(instruction && strlen(instruction) > 0)
        {
            qDebug() << instruction;
        }

        for(i = 0; i < n; i++)
        {
            char echo;
            const char *answer;
            const char *prompt;

            prompt = ssh_userauth_kbdint_getprompt(session, i, &echo);
            if(prompt == NULL)
            {
                break;
            }

            if(echo)
            {
                char *p;

                qDebug() << prompt;

                if(fgets(buffer, sizeof(buffer), stdin) == NULL)
                {
                    return SSH_AUTH_ERROR;
                }

                buffer[sizeof(buffer) - 1] = '\0';
                if((p = strchr(buffer, '\n')))
                {
                    *p = '\0';
                }

                if(SSH_OK != ssh_userauth_kbdint_setanswer(session, i, buffer))
                {
                    return SSH_AUTH_ERROR;
                }

                memset(buffer, 0, strlen(buffer));
            }
            else
            {
                if(password && strstr(prompt, "Password:"))
                {
                    answer = password;
                }
                else
                {
                    buffer[0] = '\0';

                    if(ssh_getpass(prompt, buffer, sizeof(buffer), 0, 0) < 0)
                    {
                        return SSH_AUTH_ERROR;
                    }
                    answer = buffer;
                }

                memset(buffer, 0, sizeof(buffer));
                err = ssh_userauth_kbdint_setanswer(session, i, answer);
                if(err != SSH_OK)
                {
                    return SSH_AUTH_ERROR;
                }
            }
        }
        err = ssh_userauth_kbdint(session, NULL, NULL);
    }

    return (ssh_auth_e)err;
}

ssh_auth_e SshClient::authenticateConsole(ssh_session session) const
{
    int method;
    char password[128] = {0};
    char *banner;

    int ret_val;

    // Try to authenticate
    ret_val = ssh_userauth_none(session, NULL);
    if(ret_val == SSH_AUTH_ERROR)
    {
        qDebug() << "Authentication failed: " << ssh_get_error(session);
        return SSH_AUTH_ERROR;
    }

    method = ssh_userauth_list(session, NULL);
    while(ret_val != SSH_AUTH_SUCCESS)
    {
        if(method & SSH_AUTH_METHOD_GSSAPI_MIC)
        {
            ret_val = ssh_userauth_gssapi(session);
            if(ret_val == SSH_AUTH_ERROR)
            {
                qDebug() << "Authentication failed: " << ssh_get_error(session);
                return SSH_AUTH_ERROR;
            }
            else if(ret_val == SSH_AUTH_SUCCESS)
            {
                break;
            }
        }

        // Try to authenticate with public key first
        if(method & SSH_AUTH_METHOD_PUBLICKEY)
        {
            ret_val = ssh_userauth_publickey_auto(session, NULL, NULL);
            if(ret_val == SSH_AUTH_ERROR)
            {
                qDebug() << "Authentication failed: " << ssh_get_error(session);
                return SSH_AUTH_ERROR;
            }
            else if(ret_val == SSH_AUTH_SUCCESS)
            {
                break;
            }
        }

        // Try to authenticate with keyboard interactive";
        if(method & SSH_AUTH_METHOD_INTERACTIVE)
        {
            ret_val = authKeyboardInteractive(session, NULL);
            if(ret_val == SSH_AUTH_ERROR)
            {
                qDebug() << "Authentication failed: " << ssh_get_error(session);
                return SSH_AUTH_ERROR;
            }
            else if(ret_val == SSH_AUTH_SUCCESS)
            {
                break;
            }
        }

        if(SSH_OK != ssh_getpass("Password: ", password,
                                 sizeof(password), 0, 0))
        {
            return SSH_AUTH_ERROR;
        }

        // Try to authenticate with password
        if(method & SSH_AUTH_METHOD_PASSWORD)
        {
            ret_val = ssh_userauth_password(session, NULL, password);
            if(ret_val == SSH_AUTH_ERROR)
            {
                qDebug() << "Authentication failed: " << ssh_get_error(session);
                return SSH_AUTH_ERROR;
            }
            else if(ret_val == SSH_AUTH_SUCCESS)
            {
                break;
            }
        }
        memset(password, 0, sizeof(password));
    }

    banner = ssh_get_issue_banner(session);
    if(banner)
    {
        qDebug() << "issue banner from the server: " << banner;
        ssh_string_free_char(banner);
    }

    return (ssh_auth_e)ret_val;
}

ssh_session SshClient::sshSessionInit(void) const
{
    ssh_session session = ssh_new();

    if(session == NULL)
    {
        return NULL;
    }

    if(!m_auth.m_user.isEmpty())
    {
        QByteArray usr_name = m_auth.m_user.toLatin1();
        if(SSH_OK != ssh_options_set(session,
                                     SSH_OPTIONS_USER,
                                     usr_name.data()))
        {
            ssh_free(session);
            return NULL;
        }
    }

    QByteArray server_addr = m_auth.m_host.toLatin1();
    if(SSH_OK != ssh_options_set(session,
                                 SSH_OPTIONS_HOST,
                                 server_addr.data()))
    {
        ssh_free(session);
        return NULL;
    }

    if(SSH_OK != ssh_options_set(session,
                                 SSH_OPTIONS_LOG_VERBOSITY,
                                 &m_auth.m_verbose))
    {
        ssh_free(session);
        return NULL;
    }

    if(SSH_OK != ssh_connect(session))
    {
        qDebug() << "Connection to SSH server failed: "
                 << ssh_get_error(session);
        ssh_disconnect(session);
        ssh_free(session);
        return NULL;
    }

    if(!verifyKnownhost(session))
    {
        ssh_disconnect(session);
        ssh_free(session);
        return NULL;
    }

    ssh_auth_e auth = authenticateConsole(session);
    if(auth == SSH_AUTH_SUCCESS)
    {
        return session;
    }
    else if(auth == SSH_AUTH_DENIED)
    {
        qDebug() << "Authentication failed.";
    }
    else
    {
        qDebug() << "Error while authenticating : "
                 << ssh_get_error(session);
    }

    ssh_disconnect(session);
    ssh_free(session);
    ssh_finalize();

    return NULL;
}

} // namespace::SnailNvimQt
