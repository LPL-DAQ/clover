//
// Created by lpl on 10/24/25.
//

#ifndef APP_SOCKETGUARD_H
#define APP_SOCKETGUARD_H


class SocketGuard {
public:
    int socket;

    SocketGuard(int s);
    ~SocketGuard();
};


#endif //APP_SOCKETGUARD_H
