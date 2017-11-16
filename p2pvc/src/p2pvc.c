#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <p2plib.h>
#include <unistd.h>
#include <signal.h>
#include <locale.h>
#include <fcntl.h>
#include <inttypes.h>
#include <string>

#include <audio.h>
#include <video.h>

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <arpa/inet.h>

#define DEFAULT_WIDTH 100
#define DEFAULT_HEIGHT 40

#include "libzt.h"

std::string home_path;
char *default_adhoc_port = (char*)"7878"; // totally arbitrary

typedef struct {
  char *ipaddr;
  char *port;
} network_options_t;

void *spawn_audio_thread(void *args) {
  fprintf(stderr, "spawn_audio_thread\n");
  network_options_t *netopts = (network_options_t*)args;
  start_audio(netopts->ipaddr, netopts->port);
  return NULL;
}

void audio_shutdown(int signal) {
  kill(getpid(), SIGUSR1);
}

void all_shutdown(int signal) {
  video_shutdown(signal);
  audio_shutdown(signal);
  kill(getpid(), SIGKILL);
  exit(0);
}

// Find where we want to put the config files
void get_config_install_path()
{
  struct passwd *pw = getpwuid(getuid());
  const char *homedir = pw->pw_dir;
  home_path += homedir;
  home_path += "/cathode";
}

void usage(FILE *stream) {
  fprintf(stream,
    "\nUsage: cathode [-h] [server] [options]\n"
    "\nA point to point color terminal video chat.\n"
    "  -v    Enable video chat.\n"
    "  -d    Dimensions of video in either [width]x[height] or [width]:[height]\n"
    "  -A    Audio port.\n"
    "  -V    Video port.\n"
    "  -b    Display incoming bandwidth in the top-right of the video display.\n"
    "  -e    Print stderr (which is by default routed to /dev/null).\n"
    "  -c    Use a specified color (i.e green is 0:100:0).\n"
    "  -B    Render in Braille.\n"
    "  -I    Set threshold for braille.\n"
    "  -E    Use an edge filter.\n"
    "  -a    Use custom ascii to print the video.\n"
    "  -y    About info\n"
    "\n  ---\n\n"
    "  -M                         Print (and/or generate) your ZeroTier ID\n"
    "  -Z <nwid>                  Remote ZeroTier ID\n"
    "  -N <nwid> -R <remote_ID>   Join conventional ZT network and call ZeroTier ID\n"
    "  -N <nwid> -S <remote_IP>   Join conventional ZT network and call IP address\n"
    "\n"
  );
}

int main(int argc, char **argv) {
  if (argc < 2) {
    usage(stderr);
    exit(1);
  }

  char *peer = argv[1];
  /* Check if the user actually wanted help. */
  if (!strncmp(peer, "-h", 2)) {
    usage(stdout);
    exit(0);
  }
  char *audio_port = (char*)"8888";
  char *video_port = (char*)"7878";
  vid_options_t vopt;
  int spawn_video = 0, print_error = 0;
  int c, width = DEFAULT_WIDTH, height = DEFAULT_HEIGHT;

  setlocale(LC_ALL, "");

  memset(&vopt, 0, sizeof(vid_options_t));
  vopt.width = DEFAULT_WIDTH;
  vopt.height = DEFAULT_HEIGHT;
  vopt.render_type = 0;
  vopt.refresh_rate = 20;
  vopt.saturation = -1.0;

  uint64_t nwid;
  std::string padding = "";
  std::string nwidstr = "";
  std::string remote_devid;
  int join_adhoc = 0, display_my_id = 0, call_remote_id = 0, call_remote_ip = 0, conventional_zt_network = 0;

  while ((c = getopt (argc, &(argv[0]), "byvnpdz:A::Z:D:N:S:R:V:MheBI:E:s:c:a:r")) != -1) {
    switch (c) {
      case 'v':
        spawn_video = 1;
        break;
      case 'A':
        audio_port = optarg;
        break;
      case 'V':
        video_port = optarg;
        break;
      case 'M':
        display_my_id = 1;
        break;
      case 'N':
        nwidstr = optarg;
        conventional_zt_network = 1;
        break;
      case 'R':
        remote_devid = optarg;
        call_remote_id = 1;
        break;
      case 'S':
        peer = optarg;
        call_remote_ip = 1;
        break;
      case 'd':
        sscanf(optarg, "%dx%d", &width, &height);
        vopt.width = width;
        vopt.height = height;
        break;
      case 'b':
        vopt.disp_bandwidth = 1;
        break;
      case 'r':
        sscanf(optarg, "%lu", &vopt.refresh_rate);
        break;
      case 'Z':
        // Used as the video stream port AND the basis for the ad-hoc network ID
        video_port = default_adhoc_port;
        remote_devid = optarg;
        join_adhoc = 1;
        call_remote_id = 1;
        break;
      case 'B':
        vopt.render_type = 1;
        break;
      case 'I':
        sscanf(optarg, "%d", &vopt.intensity_threshold);
        break;
      case 'E':
        vopt.edge_filter = 1;
        sscanf(optarg, "%d:%d", &vopt.edge_lower, &vopt.edge_upper);
        break;
      case 's':
        sscanf(optarg, "%lf", &vopt.saturation);
        break;
      case 'c':
        //vopt.monochrome = 1;
        //sscanf(optarg, "%" SCNd8 ":%" SCNd8 ":%" SCNd8, &vopt.r, &vopt.g, &vopt.b);
        break;
      case 'a':
        vopt.ascii_values = optarg;
        break;
      case 'y':
        fprintf(stderr, "Cathode (based on p2pvc) - By ZeroTier, Inc.\n");
        exit(0);
        break;
      case 'e':
        print_error = 1;
        break;
      case 'h':
        usage(stderr);
        break;
      default:
        break;
    }
  }


  // --- BEGIN ZEROTIER INTEGRATION ---

  get_config_install_path(); // Where ZeroTier's files are kept

  // Print (or generate AND print) ZeroTier ID
  if(display_my_id) {
    char myID[16];
    memset(myID, 0, 16);
    uint64_t res = 0;
    // Attempt to read ZeroTier ID, if not present, generate new ID
    while(res <= 0) {
      if((res = zts_get_node_id_from_file(home_path.c_str())) > 0) {
        fprintf(stderr, "You are %llx\n", res);
        exit(0);
      }
      else {
        fprintf(stderr, "Configs will be stored in: %s\n", home_path.c_str());
        fprintf(stderr, "Generating new cryptographic identity. Please wait...\n");
        // An identity couldn't be found, we will now generate one for you
        join_adhoc = 1;
        nwid = 0xff00000000000000;
        zts_startjoin(home_path.c_str(),nwid);
        zts_leave(0xff00000000000000);
        zts_stop();
      } 
    }
    exit(0);
  }

  // Using the 'default_adhoc_port', create ad-hoc <nwid> and join it.
  if(join_adhoc) {
    // Assemble address
    if(atoi(video_port) < 10)
      padding = "000";
    else if(atoi(video_port) < 100)
      padding = "00";
    else if(atoi(video_port) < 1000)
      padding = "0";
    // We will join ad-hoc network ffSSSSEEEE000000
    // Where SSSS = start port
    //       EEEE =   end port
    padding = padding+video_port; // SSSS
    nwidstr = "ff" + padding + padding + "000000"; // ff + SSSS + EEEE + 000000
    fprintf(stderr, " - Joining ad-hoc 6PLANE ZeroTier network: %llx\n", nwid);
  }

  // Start the ZeroTier background service
  // Later on, when we make our first socket API call in p2plib.c, it will initialize
  // an RPC interface between the application and the ZeroTier service.
  //
  // For this particular use case of ZeroTier we don't use a tranditional TAP driver but
  // instead run everything through a user-mode TCP/IP stack in libpicotcp.so 
  nwid = strtoull(argv[2],NULL,16);
  fprintf(stderr, " - Starting ZeroTier (path=%s, nwid=%llx)\n", home_path.c_str(), nwid);
  zts_startjoin(home_path.c_str(),nwid);

  if(call_remote_id) {
    // Generate 6PLANE IPv6 address for remote based on given <devID>
    struct sockaddr_storage sixplane_addr;
    uint64_t peer_id = strtoull(peer,NULL,16);
    zts_get_6plane_addr(&sixplane_addr, nwid, peer_id);

    char peer_addrstr[INET6_ADDRSTRLEN];
    struct sockaddr_in6 *p6 = (struct sockaddr_in6*)&sixplane_addr;
    inet_ntop(AF_INET6, &(p6->sin6_addr), peer_addrstr, INET6_ADDRSTRLEN);
    fprintf(stderr, "6plane_addr=%s", peer_addrstr);
  }
  fprintf(stderr, " - Calling: %s\n", peer);
  exit(-1);

  // --- END ZEROTIER INTEGRATION ---



  if (!print_error) {
    int fd = open("/dev/null", O_WRONLY);
    dup2(fd, STDERR_FILENO);
  }
  else {
    int fd = open("log.txt", O_WRONLY);
    dup2(fd, STDERR_FILENO);
  }

  if (spawn_video) {
    signal(SIGINT, all_shutdown);
    pthread_t thr;
    network_options_t netopts;
    netopts.ipaddr = peer;
    netopts.port = audio_port;
    pthread_create(&thr, NULL, spawn_audio_thread, (void *)&netopts);
    start_video(peer, video_port, &vopt);
  } else {
    signal(SIGINT, audio_shutdown);
    start_audio(peer, audio_port);
  }
  return 0;
}

