// regular stuff
#include <stdio.h>  // printf, perror
#include <stdlib.h> // exit
#include <string.h> // memset, strncpy, memcpy(?)
#include <unistd.h> // close
#include <fcntl.h>  // open

// socket related libs more as we go
#include <sys/socket.h> // socket(), connect(), sendmsg()
#include <sys/un.h>     // sockaddr_un
#include <string.h>     // strcpy

int main() {
    // we need to connect to the other socket htru the same sock file ig?
    struct sockaddr_un send_addr;
    // 0 out all the other bits so that we dont have random values in there that mess up everything
    memset(&send_addr, 0, sizeof(send_addr));
    send_addr.sun_family = AF_UNIX;
    strcpy(send_addr.sun_path, "temp/temp.sock");
    // the socket!!
    int conn_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    // i mean if this doesnt work then theres no client fr so yeah
    if (conn_fd < 0) {perror("client socket isnt socketing"); exit(EXIT_FAILURE);}
    
    // lets connect the socket now
    if (connect(conn_fd, (struct sockaddr *) &send_addr, sizeof(send_addr)) != 0) {
        perror("connection ain't connecting");
        exit(EXIT_FAILURE);
    }
    // we send a message neow
    char msg[] = "KAEYA BAYBAY\n"; // sizeof doesnt work if you do char * msg bcs it checks the size of the pointer itself
    int cookie_fd = open("temp/cookie.txt", O_RDONLY);
    int banana_fd = open("temp/banana.txt", O_RDONLY);
    int fds[2] = {cookie_fd, banana_fd};

    // need to use a union for alignment, this will hold the ancillary data itself
    union {
        char buf[CMSG_SPACE(sizeof(fds))];
        struct cmsghdr align;
    } cmsg;

    // main message header
    struct msghdr header;
    memset(&header, 0, sizeof(header));
    // socket is alr connected to we don't need to specify a destination address
    header.msg_name = NULL;
    header.msg_namelen = 0;
    // now we have to store the message itself
    struct iovec tosend; // its a ds sys calls use for arrays
    tosend.iov_base = msg; tosend.iov_len = sizeof(msg);
    // now lets bind that iov to the message header
    header.msg_iov = &tosend;
    header.msg_iovlen = 1; // this isnt supposed to be the actual size of the array itself, but rather the number of buffers/arrays were dealin with
    // now we bind the control message i.e. the ancillary data itself
    memset(cmsg.buf, 0, sizeof(cmsg.buf)); //  gotta build a habit of zeroing this out for when we add multiple control messages
    header.msg_control = (struct msghdr *) cmsg.buf;
    header.msg_controllen = sizeof(cmsg.buf);

    // control message header
    struct cmsghdr * cmsg_header; // needs to be a pointer for the header's sake i think?
    // now neva forget: we have to make sure were modifying the cmsg buffer
    // the firsthdr thingy returns a pointer to the first cmsg header in the ancillary data buffer we linked before (rememebr the union?)
    cmsg_header = CMSG_FIRSTHDR(&header);
    cmsg_header->cmsg_level = SOL_SOCKET;
    cmsg_header->cmsg_type = SCM_RIGHTS;
    cmsg_header->cmsg_len = CMSG_LEN(sizeof(fds)); //cant set it directly cus length differs bcs the paddding differs across different archs me thinks
    memcpy(CMSG_DATA(cmsg_header), fds, sizeof(fds)); 
    
    // now we finally send the message
    ssize_t n = sendmsg(conn_fd, &header, 0);
    if (n < 0 ) perror("sending didnt work out");

    // welp business is done, we close the socket, and the other open fds on our side atleast
    close(conn_fd); close(cookie_fd); close(banana_fd);
}