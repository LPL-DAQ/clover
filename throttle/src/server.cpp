//
// Created by lpl on 10/23/25.
//

#include <cctype>
#include <zephyr/logging/log.h>
#include <zephyr/net/socket.h>
#include <zephyr/sys/errno_private.h>
#include <zephyr/net/net_pkt.h>

#include "ThrottleValve.h"
#include "Server.h"
#include "guards/SocketGuard.h"

LOG_MODULE_REGISTER(Server, CONFIG_LOG_DEFAULT_LEVEL);

#define MAX_OPEN_CLIENTS 4

/// Main server thread must acquire one of these before accepting a connection. It must then scan through the thread
/// array to find an open slot.
K_SEM_DEFINE(num_open_connections, MAX_OPEN_CLIENTS, MAX_OPEN_CLIENTS);

K_MUTEX_DEFINE(has_thread_lock);
bool has_thread[MAX_OPEN_CLIENTS] = {false};
static k_thread client_threads[MAX_OPEN_CLIENTS] = {nullptr};
#define CONNECTION_THREAD_STACK_SIZE (4 * 1024)
K_THREAD_STACK_ARRAY_DEFINE(client_stacks, MAX_OPEN_CLIENTS, CONNECTION_THREAD_STACK_SIZE);

/// Handles a client connection. Should run in its own thread.
static void handle_client(void *p1_client_socket, void *, void *) {
    SocketGuard client_guard{reinterpret_cast<int>(p1_client_socket)};
    LOG_INF("Handling socket: %d", client_guard.socket);

    while (true) {
        // Read one byte at a time till we get a #-terminated command
        constexpr int MAX_COMMAND_LEN = 512;
        char command_buf[MAX_COMMAND_LEN + 1];
        int next_command_byte = 0;
        while (true) {
            ssize_t bytes_read = zsock_recv(client_guard.socket, command_buf + next_command_byte, 1, 0);
            if (bytes_read < 0) {
                LOG_WRN("Failed to read bytes: errno %d", errno);
                return;
            }
            if (bytes_read) {
                // Ignore whitespace
                if (std::isspace(command_buf[next_command_byte])) {
                    continue;
                }
                if (command_buf[next_command_byte] == '#') {
                    command_buf[next_command_byte + 1] = '\0';
                    break;
                }
            }
            next_command_byte += bytes_read;
            if (next_command_byte == MAX_COMMAND_LEN) {
                LOG_WRN("Didn't find command terminator `#` after %d bytes", MAX_COMMAND_LEN);
                return;
            }
        }

        LOG_INF("Got command: %s", command_buf);

        if(strcmp(command_buf, "calibrate#") == 0) {
            throttle_valve_start_calibrate();
        }
        else {
            LOG_WRN("Unknown command.");
        }
    }


    char message[] = "Hello, World!";

    // Send message to the client
    int sent = zsock_send(client_guard.socket, message, sizeof(message) - 1, 0);
    LOG_INF("Sent: %d", sent);

    // Close the client socket
    int err = zsock_close(client_guard.socket);
    if (err) {
        LOG_INF("Failed to close socket: %d", err);
    }
}

/// Attempts to join connection handler threads, allowing the thread slots to be reused to service new connection.
[[noreturn]] static void reap_dead_connections(void *, void *, void *) {
    bool freed_threads[MAX_OPEN_CLIENTS] = {false};
    while (true) {
        k_mutex_lock(&has_thread_lock, K_FOREVER);
        for (int i = 0; i < MAX_OPEN_CLIENTS; ++i) {
            if (has_thread[i]) {
                k_thread_join(&client_threads[i], K_NO_WAIT);
                has_thread[i] = false;
                freed_threads[i] = true;
                k_sem_give(&num_open_connections);
            }
        }
        k_mutex_unlock(&has_thread_lock);

        // Log freed threads outside mutex
        for (int i = 0; i < MAX_OPEN_CLIENTS; ++i) {
            if (freed_threads[i]) {
                LOG_INF("Freed thread at slot %d", i);
                freed_threads[i] = false;
            }
        }

        k_sleep(K_MSEC(50));
    }
}

K_THREAD_DEFINE(server_reaper, 1024, reap_dead_connections, nullptr, nullptr, nullptr, 1, 0, 0);


/// Opens a TCP server, listens for incoming clients, and spawns new threads to serve these connections. This function
/// blocks indefinitely.
void serve_connections() {
    LOG_INF("Opening socket");
    int server_socket = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket < 0) {
        LOG_ERR("Failed to create TCP socket: %d", errno);
        return;
    }

    sockaddr_in bind_addr = {};
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind_addr.sin_port = htons(19690);

    LOG_INF("Binding socket to address");
    int err = zsock_bind(server_socket, reinterpret_cast<sockaddr *>(&bind_addr), sizeof(bind_addr));
    if (err) {
        LOG_ERR("Failed to bind to socket `%d`: %d", server_socket, err);
        return;
    }
    LOG_INF("Listening for open connections");
    err = zsock_listen(server_socket, 0);
    if (err) {
        LOG_ERR("Failed to listen on socket `%d`: %d", server_socket, err);
        return;
    }

    // Serve new connections indefinitely
    while (true) {
        // Wait for free thread slot
        err = k_sem_take(&num_open_connections, K_FOREVER);
        if (err) {
            LOG_INF("Failed to acquire semaphore: %d", err);
            return;
        }

        // Find open connection index
        int connection_index = 0;
        k_mutex_lock(&has_thread_lock, K_FOREVER);
        for (connection_index = 0; connection_index < MAX_OPEN_CLIENTS; ++connection_index) {
            if (!has_thread[connection_index]) {
                break;
            }
        }
        k_mutex_unlock(&has_thread_lock);
        if (connection_index == MAX_OPEN_CLIENTS) {
            LOG_ERR("Consistency error: Server acquired connection semaphore but no thread slots were open");
            return;
        }

        // Spawn thread to service client connection
        int client_socket = zsock_accept(server_socket, nullptr, nullptr);
        LOG_INF("Spawning thread in slot %d to serve socket %d", connection_index, client_socket);
        k_thread_create(&client_threads[connection_index],
                        reinterpret_cast<k_thread_stack_t *>(&client_stacks[connection_index]),
                        CONNECTION_THREAD_STACK_SIZE,
                        handle_client,
                        reinterpret_cast<void *>(client_socket), nullptr, nullptr, 5, 0, K_NO_WAIT
        );

        k_mutex_lock(&has_thread_lock, K_FOREVER);
        has_thread[connection_index] = true;
        k_mutex_unlock(&has_thread_lock);
    }
}
