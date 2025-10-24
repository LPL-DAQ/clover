//
// Created by lpl on 10/24/25.
//

#include <zephyr/net/socket.h>
#include "SocketGuard.h"


/// RAII guard for a socket connection. s should be an already-connected socket/
SocketGuard::SocketGuard(int s) : socket{s} {

}

/// Closes the wrapped socket when this goes out of scope.
SocketGuard::~SocketGuard() {
    zsock_close(socket);
}
