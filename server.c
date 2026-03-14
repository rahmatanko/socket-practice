// regular stuff
#define _GNU_SOURCE // lets us use the full featured ver of the signal lib
#include <signal.h> // handling ctrl+C so that everything is cleaned up nicely
#include <stdio.h>  // printf, perror
#include <stdlib.h> // exit
#include <string.h> // memset, strncpy
#include <unistd.h> // close

// socket specific libs - ill get there when i get there
#include <sys/socket.h> // socket(), bind(), listen()
#include <sys/un.h> // sockaddr_un

// ouh i remember this from microprocessors, 
// we set the var to volatile cus otherwise the compiler might see that the variable doesnt change in main
// and that the handler isnt called in main so the var is optimized away
volatile int running = 1;

void handler(int sig) {
    (void) sig;
    running = 0;
    printf("\nshutting down neow,,,\n");
}


int main() {
    // signal handling settings
    struct sigaction settings;
    // gotta fill the unused fields with 0's so that theyre unused
    memset(&settings, 0, sizeof(settings));
    // we bind our handling func here
    settings.sa_handler = handler;
    sigaction(SIGINT, &settings, NULL);

    // make a socket
    int listen_fd = socket(AF_UNIX, SOCK_STREAM, 0); // ipc , sock stream cus it seems the most basic , 0 cus well i guess i only need 1 protocol?
    // if the return value is -1 then it's an error
    if (listen_fd < 0) {perror("socket isn't socketing"); exit(EXIT_FAILURE);}

    // now we need a sock address cus of the way bind is formatted
    struct sockaddr_un address;
    memset(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;
    // couldnt directly set the string as equal to another one to had to use strncpy
    strcpy(address.sun_path, "temp/temp.sock");

    // remember that bind creates that temp.sock file so we have to use unlink to delete that file
    unlink("temp/temp.sock");

    // bind the socket: assign the adress specified by the sockaddr to the socket referred to by sockfd
    // bind creates the socket file (temp.sock from earlier) that will hold the communications
    if (bind(listen_fd, (struct sockaddr *) &address, (socklen_t) sizeof(address)) < 0) {perror("bind isn't binding"); exit(EXIT_FAILURE);}

    // listen for connections, max 20 in the queue
    if (listen(listen_fd, 20) < 0) {perror("listenin aint workin"); exit(EXIT_FAILURE);}

    
    // accept connection and what was sent, this created a connected socket
    struct sockaddr_un conn_address; // dont have to set anything here since were accepting the client
    socklen_t conn_size = sizeof(conn_address); // need it as a varibale to it could be reference in accept()
    // so this socket is specifically for that specific accepted connection, while our prev socket is the one that welcomes all the connections in general
    int accept_fd;
    // buffer for our main message
    char main_message[1024]; char fd_buf[1024];
    int fds[2];

    // need to use a union for alignment, this will hold the ancillary data itself
    union {
        char buf[CMSG_SPACE(2 * sizeof(int))];
        struct cmsghdr align;
    } cmsg;

    // message header were going to receive
    struct msghdr header;
    memset(&header, 0, sizeof(header));
    // control message header
    // socket is alr connected to we don't need to specify a destination address
    header.msg_name = NULL;
    header.msg_namelen = 0;
    // now we have to store the message itself
    struct iovec tosend; // its a ds sys calls use for arrays
    tosend.iov_base = main_message; tosend.iov_len = sizeof(main_message);
    // now lets bind that iov to the message header
    header.msg_iov = &tosend;
    header.msg_iovlen = 1; 
    // now we bind the control message i.e. the ancillary data itself
    memset(cmsg.buf, 0, sizeof(cmsg.buf)); //  gotta build a habit of zeroing this out for when we add multiple control messages
    header.msg_control = (void *) cmsg.buf;
    header.msg_controllen = sizeof(cmsg.buf);
    struct cmsghdr * cmsg_header = CMSG_FIRSTHDR(&header);

    while (running) {
        // emptying the buffer after each run
        memset(fd_buf, 0, sizeof(fd_buf));
        // this means that if it fails, we shouldn't be shutting the whole thing down, just skip over this one
        // btw accept is blocking (checl that out with strace), thats why multithreaded servers are so gudddt
        accept_fd = accept(listen_fd, (struct sockaddr *) &conn_address, &conn_size);
        if (accept_fd < 0) {
            // if its bcs of the signal handler then we just break de loop
            if (!running) break;
            perror("connected socket isn't socketing, onto the next one"); close(accept_fd); continue;
        }
        // lets retrieve our messageee :3
        if (recvmsg(accept_fd, &header, 0) == -1) perror("receivin went wrong");
        // now the original message is stored in its complementary variable
        printf("the main message: %s\n", main_message);
        memcpy(fds, CMSG_DATA(cmsg_header), sizeof(fds));
        for (int i = 0; i < 2; i++) {
            if (read(fds[i], fd_buf, 1024) == -1) perror("aint read");
            printf("file %i: %s\n", i, fd_buf);
        }
        close(accept_fd); // still need to close it
        for (int i = 0; i < 2; i++) close(fds[i]); // close the file fds
    }
    
    // remember that bind creates that temp.sock file so we have to use unlink to delete that file
    unlink("temp/temp.sock");
    // i think we have to decouple and close stuff here but we won't reach it for now
    close(accept_fd); close(listen_fd);
    printf("\ncleanup is done!!\n");
}

