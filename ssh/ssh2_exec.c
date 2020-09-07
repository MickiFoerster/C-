#include <arpa/inet.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <libssh2.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

static int waitsocket(int socket_fd, LIBSSH2_SESSION *session);
static bool is_host_known(LIBSSH2_SESSION *session, const char *hostname);

int main(int argc, char *argv[])
{
    const char *hostname = "127.0.0.1";
    const char *commandline = "uptime";
    unsigned long hostaddr;
    int sock = -1;
    struct sockaddr_in sin;
    LIBSSH2_SESSION *session;
    LIBSSH2_CHANNEL *channel;
    int rc;
    int exitcode;
    char *exitsignal = (char *)"none";
    int bytecount = 0;

    if (argc > 1) /* must be ip address only */
      hostname = argv[1];
 
    rc = libssh2_init(0);
    if(rc != 0) {
        fprintf(stderr, "libssh2 initialization failed (%d)\n", rc);
        return 1;
    }
 
    hostaddr = inet_addr(hostname);
    sock = socket(AF_INET, SOCK_STREAM, 0);
    sin.sin_family = AF_INET;
    sin.sin_port = htons(22);
    sin.sin_addr.s_addr = hostaddr;
    if(connect(sock, (struct sockaddr*)(&sin),
                sizeof(struct sockaddr_in)) != 0) {
        fprintf(stderr, "failed to connect!\n");
        return -1;
    }

    // create non-blocking session
    session = libssh2_session_init();
    if (!session) {
      goto fatalerror;
    }
    libssh2_session_set_blocking(session, 0);

    do {
      rc = libssh2_session_handshake(session, sock);
    } while (rc == LIBSSH2_ERROR_EAGAIN);
    if(rc) {
        fprintf(stderr, "Failure establishing SSH session: %d\n", rc);
        goto fatalerror;
    }

    if (!is_host_known(session, hostname)) {
      fprintf(stderr,
              "warning: host %s is NOT known or hostkey does not match\n",
              hostname);
    }

    LIBSSH2_AGENT *agent = NULL;
    struct libssh2_agent_publickey *identity, *prev_identity = NULL;
    agent = libssh2_agent_init(session);
    assert(agent);

    rc = libssh2_agent_connect(agent);
    assert(rc == 0);

    rc = libssh2_agent_list_identities(agent);
    assert(rc == 0);

    for (;;) {
      rc = libssh2_agent_get_identity(agent, &identity, prev_identity);
      if (rc == 1)
        break;
      if (rc < 0) {
        fprintf(stderr, "Failure obtaining identity from ssh-agent support\n");
        exit(1);
      }
      const char username[] = "pi";
      while (libssh2_agent_userauth(agent, username, identity) ==
             LIBSSH2_ERROR_EAGAIN)
        ;
      if (rc) {
        fprintf(stderr,
                "\tAuthentication with username %s and "
                "public key %s failed! (%d)\n",
                username, identity->comment, rc);
      } else {
        fprintf(stderr,
                "\tAuthentication with username %s and "
                "public key %s succeeded! (%d)\n",
                username, identity->comment, rc);
        break;
      }
      prev_identity = identity;
    }
    assert(rc == 0);

    /* Exec non-blocking on the remove host */
    while ((channel = libssh2_channel_open_session(session)) == NULL &&
           libssh2_session_last_error(session, NULL, NULL, 0) ==
               LIBSSH2_ERROR_EAGAIN) {
      waitsocket(sock, session);
    }
    if (channel == NULL) {
      fprintf(stderr, "Error\n");
      exit(1);
    }
    while ((rc = libssh2_channel_exec(channel, commandline)) == LIBSSH2_ERROR_EAGAIN) {
      waitsocket(sock, session);
    }
    if (rc != 0) {
      fprintf(stderr, "Error\n");
      exit(1);
    }
    for (;;) {
      /* loop until we block */
      int rc;
      do {
        char buffer[0x4000];
        rc = libssh2_channel_read(channel, buffer, sizeof(buffer));
        if (rc > 0) {
          int i;
          bytecount += rc;
          fprintf(stderr, "We read:\n");
          for (i = 0; i < rc; ++i)
            fputc(buffer[i], stderr);
          fprintf(stderr, "\n");
        } else {
          if (rc != LIBSSH2_ERROR_EAGAIN)
            /* no need to output this for the EAGAIN case */
            fprintf(stderr, "libssh2_channel_read returned %d\n", rc);
        }
      } while (rc > 0);

      /* this is due to blocking that would occur otherwise so we loop on
         this condition */
      if (rc == LIBSSH2_ERROR_EAGAIN) {
        waitsocket(sock, session);
      } else
        break;
    }
    exitcode = 127;
    while ((rc = libssh2_channel_close(channel)) == LIBSSH2_ERROR_EAGAIN)
      waitsocket(sock, session);

    if (rc == 0) {
      exitcode = libssh2_channel_get_exit_status(channel);
      libssh2_channel_get_exit_signal(channel, &exitsignal,
                                      NULL, NULL, NULL, NULL, NULL);
    }

    if (exitsignal)
      fprintf(stderr, "\nGot signal: %s\n", exitsignal);
    else
      fprintf(stderr, "\nEXIT: %d bytecount: %d\n", exitcode, bytecount);

    libssh2_channel_free(channel);

    channel = NULL;

    // shutdown:
    libssh2_session_disconnect(session,
                               "Normal Shutdown, Thank you for playing");
    libssh2_session_free(session);
    close(sock);
    fprintf(stderr, "all done\n");

    libssh2_exit();

    return 0;

fatalerror:
  close(sock);
  return 1;
}

static int waitsocket(int socket_fd, LIBSSH2_SESSION *session) {
  struct timeval timeout;
  int rc;
  fd_set fd;
  fd_set *writefd = NULL;
  fd_set *readfd = NULL;
  int dir;

  timeout.tv_sec = 10;
  timeout.tv_usec = 0;

  FD_ZERO(&fd);
  FD_SET(socket_fd, &fd);

  /* now make sure we wait in the correct direction */
  dir = libssh2_session_block_directions(session);
  if (dir & LIBSSH2_SESSION_BLOCK_INBOUND)
    readfd = &fd;
  if (dir & LIBSSH2_SESSION_BLOCK_OUTBOUND)
    writefd = &fd;

  rc = select(socket_fd + 1, readfd, writefd, NULL, &timeout);

  return rc;
}

static bool is_host_known(LIBSSH2_SESSION *session, const char *hostname) {
  LIBSSH2_KNOWNHOSTS *nh;
  nh = libssh2_knownhost_init(session);
  if (!nh) {
    fprintf(stderr, "libssh2_knownhost_init() failed\n");
    return false;
  }

  char known_host_file[4096];
  const char *fingerprint;
  snprintf(known_host_file, sizeof(known_host_file), "%s/.ssh/known_hosts",
           getenv("HOME"));
  int rc = libssh2_knownhost_readfile(nh, known_host_file,
                                      LIBSSH2_KNOWNHOST_FILE_OPENSSH);
  if (rc <= 0) {
    fprintf(stderr, "error: reading known host file error %d\n", rc);
    libssh2_knownhost_free(nh);
    return false;
  }

  size_t len;
  int type;
  fingerprint = libssh2_session_hostkey(session, &len, &type);
  if (!fingerprint) {
    fprintf(stderr, "error: libssh2_session_hostkey() failed\n");
    libssh2_knownhost_free(nh);
    return false;
  }

  struct libssh2_knownhost *host;
  int check = libssh2_knownhost_checkp(
      nh, hostname, 22, fingerprint, len,
      LIBSSH2_KNOWNHOST_TYPE_PLAIN | LIBSSH2_KNOWNHOST_KEYENC_RAW, &host);
  libssh2_knownhost_free(nh);

  switch (check) {
  case LIBSSH2_KNOWNHOST_CHECK_MATCH:
    return true;
  case LIBSSH2_KNOWNHOST_CHECK_FAILURE:
  case LIBSSH2_KNOWNHOST_CHECK_NOTFOUND:
  case LIBSSH2_KNOWNHOST_CHECK_MISMATCH:
  default:
    return false;
  }
}

