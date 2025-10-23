//
// Created by lpl on 10/23/25.
//

#include "Server.h"

#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>

#include <zephyr/net/net_pkt.h>

LOG_MODULE_REGISTER(Server, CONFIG_LOG_DEFAULT_LEVEL);

#define MAX_OPEN_CLIENTS 4
/// Index to connection_threads for unused thread slot. Must be in [0, MAX_OPEN_CLIENTS).
static int next_open_connection = 0;

static k_thread client_threads[MAX_OPEN_CLIENTS] = {nullptr};
#define CONNECTION_THREAD_STACK_SIZE (4 * 1024)
K_THREAD_STACK_ARRAY_DEFINE(client_stacks, MAX_OPEN_CLIENTS, CONNECTION_THREAD_STACK_SIZE);


/// Handles a client connection. Should run in its own thread.
static void handle_client(void *p1_client_socket, void *, void *) {
    int client_socket = reinterpret_cast<int>(p1_client_socket);
    LOG_INF("Handling socket: %d", client_socket);
    char message[] = "Hello, World!";

    // Send message to the client
    int sent = zsock_send(client_socket, message, sizeof(message) - 1, 0);
    LOG_INF("Sent: %d", sent);

    // Close the client socket
    int err = zsock_close(client_socket);
    if (err) {
        LOG_INF("Failed to close socket: %d", err);
    }
}

/// Opens a TCP server, listens for incoming clients, and spawns new threads to serve these connections. This function
/// blocks indefinitely.
int serve_connections() {
    LOG_INF("Opening socket");
    int server_socket = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket < 0) {
        LOG_ERR("Failed to create TCP socket: %d", errno);
        return 1;
    }

    sockaddr_in bind_addr = {};
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind_addr.sin_port = htons(19690);

    LOG_INF("Binding socket to address");
    int err = zsock_bind(server_socket, reinterpret_cast<sockaddr *>(&bind_addr), sizeof(bind_addr));
    if (err < 0) {
        LOG_ERR("Failed to bind to socket `%d`: %d", server_socket, err);
        return err;
    }
    LOG_INF("Listening for open connections");
    err = zsock_listen(server_socket, 4);
    if (err < 0) {
        LOG_ERR("Failed to listen on socket `%d`: %d", server_socket, err);
        return err;
    }

    // Serve new connections indefinitely
    while (true) {
        int client_socket = zsock_accept(server_socket, nullptr, nullptr);
        LOG_INF("Spawning thread to serve socket %d", client_socket);
        k_thread_create(&client_threads[next_open_connection],
                        reinterpret_cast<k_thread_stack_t *>(&client_stacks[next_open_connection]),
                        CONNECTION_THREAD_STACK_SIZE,
                        handle_client,
                        reinterpret_cast<void *>(client_socket), nullptr, nullptr, 5, 0, K_NO_WAIT
        );
        next_open_connection += 1;
        // TODO - reap dead threads
    }
}
