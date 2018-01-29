#include "CommandServer.h"
#include "cutter.h"

CommandServer::CommandServer(QObject *parent) : QObject(parent)
{

}

CommandServer::~CommandServer()
{

}

/**
 * @brief Open a local TCP server on port 1234 to read and execute commands.
 * This is useful for Jupyter Notebook to access radare2 context using r2pipe
 * Example usage:
 * import r2pipe; r2 = r2pipe.open('tcp://localhost:1234'); print(r2.cmd('i'))
 */
bool CommandServer::startCommandServer()
{
    RCore* core = Core()->core();
    const char* port = "1234";

    unsigned char buf[4097];
    RSocket *ch = NULL;
    RSocket *s;
    int i, ret;
    char *str;

    s = r_socket_new (0);
    r_socket_listen (s, port, NULL);
    if (false) {
        eprintf ("Error listening on port %s\n", port);
        r_socket_free (s);
        return false;
    }

    eprintf ("Listening for commands on port %s\n", port);
    while (isRunning) {
        ch = r_socket_accept (s);
        buf[0] = 0;
        ret = r_socket_read (ch, buf, sizeof (buf) - 1);
        if (ret > 0) {
            buf[ret] = 0;
            for (i = 0; buf[i]; i++) {
                if (buf[i] == '\n') {
                    buf[i] = buf[i + 1]? ';': '\0';
                }
            }
            if ((!r_config_get_i (core->config, "scr.prompt") &&
                 !strcmp ((char *)buf, "q!")) ||
                !strcmp ((char *)buf, ".--")) {
                r_socket_close (ch);
                break;
            }
            str = r_core_cmd_str (core, (const char *)buf);
            if (str && *str)  {
                r_socket_write (ch, str, strlen (str));
            } else {
                const char nl[] = "\n";
                r_socket_write (ch, (void*) nl, 1);
            }
            free (str);
        }
        r_socket_close (ch);
        r_socket_free (ch);
        ch = NULL;
    }
    r_socket_free (s);
    r_socket_free (ch);
    eprintf ("TCP Server (on %s) exited.\n", port);

    return true;
}

void CommandServer::process()
{
    startCommandServer();
    emit finished();
}

/**
 * Stops the server.
 */
void CommandServer::stop()
{
    isRunning = false;
}
