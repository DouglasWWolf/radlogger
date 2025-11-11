#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <cstdarg>
#include <cstring>
#include <deque>
#include <thread>
#include <mutex>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <netinet/in.h>

// This is the queue that our incoming log entries get written to
std::deque<std::string> queue;
std::mutex queue_mtx;

// When this is true, our main TCP server is connected
bool isConnected = false;

int  createUdpServer(int port);
int  createTcpServer(int port);
void backdoorServer(int sd);
void udpServer(int sd);
void execute();


// This used to notify the writer of an incoming message
int message_pipe[2];

// Used to notify the writer of a "backdoor" reset
int writer_pipe[2];

int main(int argc, const char** argv)
{
    try
    {
        execute();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }

    exit(1);
}




//=============================================================================
// throwRuntime() - Throws a runtime exception
//=============================================================================
static void throwRuntime(const char* fmt, ...)
{
    char buffer[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buffer, sizeof buffer, fmt, ap);
    va_end(ap);

    throw std::runtime_error(buffer);
}
//=============================================================================


//=============================================================================
// drain() - Drains all bytes from a file-descriptor
//=============================================================================
void drain(int fd)
{
    char buffer;
    int  count;
    
    while (true)
    {
        // Fetch the number of bytes available for reading
        ioctl(fd, FIONREAD, &count);

        // If there's no bytes ready to read, this is drained
        if (count == 0) return;

        // Read and throw away every available byte
        while (count--)
        {
            int rc = read(fd, &buffer, 1); rc;
        }
    }
}
//=============================================================================


//=============================================================================
// This sends a string to a socket
//=============================================================================
bool sendtcp(int sd, const std::string& s)
{
    // Find out how long the string is
    uint32_t length = s.size();
    
    // Get a pointer to the start of the string
    const char* p = s.c_str();

    // Call write() as many times as it takes to send
    // the entire string
    while (length)
    {
        int bytes = write(sd, p, length);
        if (bytes < 0) return false;
        length -= bytes;
        p += bytes;
    }

    // Tell the caller that the data was send
    return true;   
}
//=============================================================================


//=============================================================================
// The main work of the program happens here
//=============================================================================
void execute()
{
    int rc, sd;
    fd_set rfds;

    // Build the pipe that notifies the main thread of a message
    rc = pipe(message_pipe); if (rc) exit(1);
    
    // Build the pipe that notifies the main thread to reset
    rc = pipe(writer_pipe); if (rc) exit(1);

    // Create the UDP server socket
    sd = createUdpServer(40000);

    // Launch the UDP server
    std::thread th1(udpServer, sd);
    th1.detach();

    // Launch the backdoor server
    sd = createTcpServer(40001);
    std::thread th2(backdoorServer, sd);

    // Create the primary TCP server
    int server_sd = createTcpServer(40000);

    // Begin listening for connections
    listen(server_sd, 1);

WaitForConnection:

    // Tell anyone that cares that we don't have a client connected
    isConnected = false;

    // Accept a connection
    sd = accept(server_sd, nullptr, nullptr);

    // Throw away any leftover notifications in the pipes
    drain(writer_pipe[0]);
    drain(message_pipe[0]);

    // We are connected to a peer
    isConnected = true;

    // Loop forever
    while (true)
    {
        // Find out how many entries are in the queue
        queue_mtx.lock();
        uint32_t entries = queue.size();
        queue_mtx.unlock();

        while (entries)
        {
            // Fetch the log entry at the front of the queue
            queue_mtx.lock();
            const std::string& s = queue.front();
            queue_mtx.unlock();

            // Send it to the TCP client
            if (!sendtcp(sd, s))
            {
                isConnected = false;
                break;
            }

            // Remove the entry from the front of the queue
            queue_mtx.lock();            
            queue.pop_front();
            queue_mtx.unlock();            

            // We have one fewer entries to fetch
            --entries;
        }

        // If the TCP client closed the connection,
        // go back to waiting for an incoming comnection
        if (!isConnected)
        {
            close(sd);
            goto WaitForConnection;
        }

        // We're going to wait for a notification that
        // either a message has arrived, or we need to 
        // close the main input socket and wait for a 
        // new connection
        FD_ZERO(&rfds);
        FD_SET( writer_pipe[0], &rfds);
        FD_SET(message_pipe[0], &rfds);
        FD_SET(             sd, &rfds);

        // Among the writer_pipe, the message_pipe, and the
        // TCP socket, find out who has the largest FD
        int maxfd = writer_pipe[0];
        if (message_pipe[0] > maxfd) maxfd = message_pipe[0];
        if (sd > maxfd) maxfd = sd;

        // Wait for a notification to arrive
        select(maxfd+1, &rfds, nullptr, nullptr, nullptr);

        // If we're being told to close the main TCP pipe
        // do so, and go wait for another connection
        if (FD_ISSET(writer_pipe[0], &rfds))
        {
            close(sd);
            goto WaitForConnection;
        }

        // If we're the socket has suddenly become readable,
        // go wait for another connection
        if (FD_ISSET(sd, &rfds))
        {
            close(sd);
            goto WaitForConnection;
        }

    }

    
}
//=============================================================================


//=============================================================================
// backdoorServer() - Listens for connections on the "backdoor" TCP port, and 
//                    when we receive one, it uses "backdoor_pipe" to signal
//                    then main thread that it should close and reopen the
//                    TCP server socket
//=============================================================================
void backdoorServer(int sd)
{
    char zero[1] = {0};

    listen(sd, 2);

    while (true)
    {
        int newsd = accept(sd, nullptr, nullptr);
        close(newsd);
        int rc = write(writer_pipe[1], zero, 1); rc;
    }
}
//=============================================================================


//=============================================================================
// createUdpServer() - Returns the file-descriptor of an open UDP server socket
//=============================================================================
int createUdpServer(int port)
{
    // Create the socket and complain if we can't
    int sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sd < 0)
    {
        throwRuntime("Failed while creating UDP socket");
    }

    // Build the address structure of the UDP server
    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)port);

    // Bind the socket to the port
    if (bind(sd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0)
    {
        throwRuntime("Unable to bind UDP server to port %i", port);
    }

    // Return the socket descriptor to the caller
    return sd;
}
//=============================================================================



//=============================================================================
// This is a UDP server that runs in its own thread.  It waits for UDP messages
// to arrive and stuffs them into the deque
//=============================================================================
void udpServer(int sd)
{
    char buffer[0x1000];
    int rc;

    while (true)
    {
        // Wait for a UDP message to arrive
        int byteCount = recvfrom(sd, buffer, sizeof(buffer)-4, 0, nullptr, nullptr);

        // Make sure the buffer is nul-terminated
        buffer[byteCount] = 0;

        // Ignore empty messages
        if (byteCount == 0) continue;

        // Point to the last character in the message
        char * p = buffer + byteCount - 1;

        // Back up until we find a non-nul byte
        while (p > buffer && *p == 0) --p;

        // Make sure it's terminated with a linefeed, a period, and another linefeed
        if (*p == '\n')
            strcpy(p+1, ".\n");
        else
            strcpy(p+1, "\n.\n");

        // Turn out buffer into a std::string
        const std::string& s = buffer;

        // Add this entry to the queue, and if there is a client
        // connected, notify the main thread that we just added
        // a log entry to the queue
        queue_mtx.lock();
        bool hasRoom = queue.size() < 100;
        if (hasRoom) queue.push_back(s);
        queue_mtx.unlock();

        // If we just added an entry to the queue and we're 
        // connected to a TCP client, notify the main thread
        // that a message is waiting
        if (hasRoom && isConnected)
        {
            rc = write(message_pipe[1], buffer, 1); rc;
        }
    }

}
//=============================================================================



//=============================================================================
// createTcpServer() - Returns the file-descriptor of an open TCP server socket
//=============================================================================
int createTcpServer(int port)
{
    // Create the socket and complain if we can't
    int sd = socket(AF_INET, SOCK_STREAM, 0);
    if (sd < 0)
    {
        throwRuntime("Failed while creating TCP socket");
    }

    // Build the address structure of the UDP server
    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)port);

    // Bind the socket to the port
    if (bind(sd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0)
    {
        throwRuntime("Unable to bind TCP server to port %i", port);
    }

    // Return the socket descriptor to the caller
    return sd;
}
//=============================================================================
