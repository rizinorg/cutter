#include "radarewebserver.h"
#include "qrcore.h"
#include <cassert>


RadareWebServer::RadareWebServer(QRCore *core) :
    core(core),
    started(false)
{
    // MEOW
}

RadareWebServer::~RadareWebServer()
{
}

void RadareWebServer::start()
{
    assert(core != nullptr);

    if (!started && core != nullptr)
    {
        // command: see libr/core/rtr.c
        core->cmd("=h&");
        core->core()->http_up = R_TRUE;
        started = true;
    }
}

void RadareWebServer::stop()
{
    // TODO: =h- waits for ^C
}

bool RadareWebServer::isStarted() const
{
    return started;
}
