#ifndef CLOVER_SERVER_H
#define CLOVER_SERVER_H

void serve_connections();

int send_fully(int sock, const char *buf, int len);

#endif //CLOVER_SERVER_H
