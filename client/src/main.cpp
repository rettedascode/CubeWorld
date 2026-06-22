#include "CubeWorldLayer.h"

#include <ce/core/Application.h>

#include <net/Host.h>

#include <iostream>
#include <memory>
#include <string>

int main(int argc, char** argv) {
    std::cout << std::unitbuf; // flush log lines immediately

    // ENet must be initialized once before any networking (and shut down once).
    net::initialize();

    {
        // Optional arg: a server host to connect to (disables the integrated
        // server). With no arg, run singleplayer (self-hosted over loopback).
        const bool        multiplayer = argc > 1;
        const std::string host        = multiplayer ? argv[1] : "127.0.0.1";

        ce::WindowProps props;
        props.title  = multiplayer ? "CubeWorld (client)" : "CubeWorld";
        props.width  = 1280;
        props.height = 720;

        ce::Application app(props);
        app.pushLayer(std::make_unique<cw::CubeWorldLayer>(host, !multiplayer));
        app.run();
    } // Application (and the layer + NetClient) destroyed before net::shutdown()

    net::shutdown();
    return 0;
}
