/* @file p2plib.c
 * @brief Implements p2plib.
 */

#include <stdio.h>
#include <string.h>
#include <p2plib.h>
#include <errno.h>
#include <err.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <time.h>

#include "sdk.h"

#define UDP_FLAGS         0
#define BANDWIDTH_BUFLEN  1024

static struct timespec prevPacket, currPacket;
static long delta = -1;
static unsigned int bandwidth_index;
static double bandwidth_buf[BANDWIDTH_BUFLEN];

/* @brief gives the bandwidth for a given packet size 
 * @return a double that represents the bandwidth in bytes/nanoseconds
 */
double p2p_bandwidth() {
  double tot = 0;
  unsigned int i = 0;
  for (i = 0; i < BANDWIDTH_BUFLEN; i++) {
    tot += bandwidth_buf[i];
  }
  return tot/BANDWIDTH_BUFLEN;
}


/* @brief tells if a packet is used for p2p reasons
 * @param con who sent the data
 * @param data is the data
 * @param datasize is data size
 * @param cons current peer's connections (to append if CONS_HEADER)
 * @param conslen
 * @return 1 if its a p2plib packet else 0
 */
int p2p_data(connection_t *con, void *data, size_t datasize,
             connection_t **cons, size_t *conslen) {

  if (P2P_HEADER != ((p2p_header_t *)data)->check) {
    return 0;
  }

  if (((p2p_header_t *)data)->act == PASS_HEADER) { 
    /*XXX we can just use sum bytes for this as well */
    char msg[32] = "password already sent";
    p2p_send(con, msg, 20);
  } else if (((p2p_header_t *)data)->act == CONS_HEADER) {
    size_t h_cons, i;

    memcpy(&h_cons, (long*)data + sizeof(p2p_header_t), sizeof(size_t));

    *cons = (connection_t*)realloc(*cons, (*conslen + h_cons));
    
    /* copy the connection_ts into the library */
    for (i=0; i<h_cons; i++) {
      memcpy(&cons[*conslen + i], ((long*)data + sizeof(size_t) + sizeof(p2p_header_t) + i*sizeof(connection_t)), sizeof(connection_t));
    }

    *conslen += h_cons;
  }
  return (1);
}

/* @brief Connects to a server.
 * @param server_name The IP address of the server.
 * @param server_port The port to connect on. 
 * @param c The connection to be populated.
 * @return 0 on success -1 on error.
 */
int p2p_connect(char *server_name, char *server_port, connection_t *con) {
  
  int port;
  sscanf(server_port, "%d", &port);
  struct sockaddr_in6 serv_addr;
  struct hostent *server; 
  port = atoi(server_port);
  server = gethostbyname2(server_name,AF_INET6);
  memset((char *) &serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin6_flowinfo = 0;
  serv_addr.sin6_family = AF_INET6;
  memmove((char *) &serv_addr.sin6_addr.s6_addr, (char *) server->h_addr, server->h_length);
  serv_addr.sin6_port = htons(port);

  con->addr.sin6_family = AF_INET6;
  con->addr.sin6_port = htons(port);

  memcpy((void*)&con->addr.sin6_addr, &serv_addr.sin6_addr, server->h_length);
  con->addr_len = sizeof(con->addr);
  
  con->socket = zts_socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);

  return 0;
}

/* @brief Create a server that can cold up to max_connections.
 * @param port The port to initialize on.
 * @param sockfd A reference to populate once the socket is initialized.
 * @return An errno or 0 on success.
 */
int p2p_init(int port, int *sockfd) {
  int _sockfd_local;
  struct sockaddr_in6 me;

  /* create socket */
  if ((_sockfd_local = zts_socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
    fprintf(stderr, "error in opening socket: %s\n", strerror(errno));
    return (errno);
  }

  /* If caller wants, populate socket fd */

  if (sockfd != NULL) {
    *sockfd = _sockfd_local;
  }

  /* XXX: Make sure these fields are OK, possibly switch to variables.*/
  memset(&me, 0, sizeof(me));
  me.sin6_family = AF_INET6;
  me.sin6_port = htons(port);


  struct sockaddr_in6 serv_addr;
  struct hostent *server; 
  server = gethostbyname2("::",AF_INET6);
  memset((char *) &serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin6_flowinfo = 0;
  serv_addr.sin6_family = AF_INET6;
  memmove((char *) &me.sin6_addr.s6_addr, (char *)server->h_addr, server->h_length);


  if (zts_bind(_sockfd_local, (struct sockaddr *)&me, sizeof(me)) == -1) {
    fprintf(stderr, "error in binding socket: %s\n", strerror(errno));
    return (errno);
  }

  /* success */
  return 0;
}


int p2p_send_pass(connection_t *con, char *password) {
  p2p_header_t head;
  head.act = PASS_HEADER;
  head.check = P2P_HEADER;
  
  size_t sendbufsize = strlen(password) + 1 + sizeof(p2p_header_t);
  void *sendbuf = malloc(sendbufsize);

  memcpy(sendbuf, &head, sizeof(p2p_header_t));
  memcpy((long*)sendbuf + sizeof(p2p_header_t), password, strlen(password) + 1);

  int rv = p2p_send(con, sendbuf, sendbufsize);

  free(sendbuf);
  return (rv);
}

int p2p_send_conns(connection_t *con, connection_t *cons, size_t conslen) {
  size_t i;
  p2p_header_t head;
  head.act = CONS_HEADER;
  head.check = P2P_HEADER;

  size_t sendbufsize =  sizeof(size_t) + sizeof(p2p_header_t) + conslen * sizeof(connection_t);
  void *sendbuf = malloc(sendbufsize);

  memcpy(sendbuf, &head, sizeof(p2p_header_t));
  memcpy((long*)sendbuf + sizeof(p2p_header_t), &conslen, sizeof(size_t));

  for (i=0; i<conslen; i++) {
    memcpy((long*)sendbuf + sizeof(size_t) + sizeof(p2p_header_t) + i*sizeof(connection_t), &cons[i], sizeof(connection_t));
  }

  int rv = p2p_send(con, sendbuf, sendbufsize);

  free(sendbuf);
  return (rv);
}

/* @brief Send data to a connection.
 * @param con The connection to send the data to.
 * @param buf The data to send.
 * @param buflen The length of the data to send.
 * @return Negative value on error, 0 on success.
 */
int p2p_send(connection_t *con, const void *buf, size_t buflen) {
  return zts_sendto(con->socket, buf, buflen, UDP_FLAGS, (struct sockaddr *)&con->addr, con->addr_len);
}

/* @brief Send data to all connections.
 * @param cons A reference to a connection array.
 * @param conslen A reference to the length of the connection array.
 * @param consmutex A mutex to access the connection array.
 * @param buf The data to send.
 * @param buflen The length of the data to send.
 * @return Negative value on error, 0 on success.
 */
int p2p_broadcast(connection_t **cons, size_t *conslen, pthread_mutex_t *consmutex, const void *buf, size_t buflen) {
  if(consmutex) {
    pthread_mutex_lock(consmutex);
  }

  int i;
  for (i = 0; i < *conslen; i++) {
    p2p_send(&((*cons)[i]), buf, buflen);
  }

  if(consmutex) {
    pthread_mutex_unlock(consmutex);
  }
  return 0;
}

/* @brief Initialize a listener.
 * @param cons A reference to a connection array.
 * @param conslen A reference to the length of the connection array.
 * @param consmutex A mutex to access the connection array.
 * Note: The reason there are references is so the array can be updated.
 *       In the case of a new connection, new_callback will be called.
 * @param callback The standard callback, passing in the data and the
 *        connection associated with the data.
 * @param new_callback Called when a new connection is discovered.
 *        If null nothing happens.
 * @return Negative value on error.
 */
int p2p_listener(connection_t **cons, size_t *conslen,
    pthread_mutex_t *consmutex,
    void (*callback)(connection_t *, void *, size_t),
    void (*new_callback)(connection_t *, void *, size_t),
    int socket,
    unsigned long max_packet_size) {

  /* A stack allocated connection struct to store any data
     about the connection we recieve. */
  connection_t con;
  char buf[max_packet_size];


  /* Loop on recvfrom. */
  while (1) {
    memset(buf, 0, max_packet_size);
    int recv_len = zts_recvfrom(socket, buf, max_packet_size, UDP_FLAGS, (struct sockaddr *)&(con.addr), &(con.addr_len));

    fprintf(stderr, "recvfrom()=%d\n", recv_len);
#ifdef __linux__
/* Temporarily disable bandwidth.  Broken for OSX. */
    if (delta == -1) {
      clock_gettime(CLOCK_MONOTONIC, &prevPacket);
      delta = 0;
    } else {
      clock_gettime(CLOCK_MONOTONIC, &currPacket);
      delta = currPacket.tv_nsec - prevPacket.tv_nsec;
      clock_gettime(CLOCK_MONOTONIC, &prevPacket);
      bandwidth_index = (bandwidth_index + 1) % BANDWIDTH_BUFLEN;
      bandwidth_buf[bandwidth_index] = (double)recv_len/delta;
    }
#endif

    /* Handle error UDP style (try again). */
    if (recv_len < 0) {
      fprintf(stderr, "Recieve failed. errno: %d\n", errno);
      continue;
    }

    if(consmutex) {
      pthread_mutex_lock(consmutex);
    }

    /* Check if the connection we recieved from is in our array. */
    int i, new_connection = 1;
    for (i = 0; i < *conslen; i++) {
      if (con.addr.sin6_addr.s6_addr == (*cons)[i].addr.sin6_addr.s6_addr) {
        new_connection = 0;
        break;
      }
    }

    if(consmutex) {
      pthread_mutex_unlock(consmutex);
    }

    /* Now invoke callbacks. */
    //if (new_connection) {
      /* Ignore new_callback if not defined. */
    //  if (new_callback) {
    //    (*new_callback)(&con, buf, recv_len);
    //  }
    //} else {
    //  (*callback)(&con, buf, recv_len);
    //}
    /* Commented out the above, we should add connections
    as the original author intended once the prototype is finished */
    (*callback)(&con, buf, recv_len);
  }

  return -1;
}

