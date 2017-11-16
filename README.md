```
             _    _                 _       
            | |  | |               | |      
  ___  __ _ | |_ | |__    ___    __| |  ___ 
 / __|/ _` || __|| '_ \  / _ \  / _` | / _ \
| (__| (_| || |_ | | | || (_) || (_| ||  __/
 \___|\__,_| \__||_| |_| \___/  \__,_| \___|
 Now with ENCRYPTED PACKET technology!
```

# cathode
Encrypted P2P video chat in your terminal

***
Cathode is a weakly-interacting coupling of two peices of software. First we start with [p2pvc](https://github.com/mofarrell/p2pvc) (the brilliant little app that renders the audio/video in your terminal), and then [ZeroTier](https://github.com/zerotier) (an extremely small but very robust encrypted P2P virtual networking hypervisor). We use the library form found [here](https://github.com/zerotier/libzt). We then replace all of the socket API system calls with their libzt equivalents aaaaannnd we're done.

Hint: Use the [Glass TTY VT220](Glass_TTY_VT220.ttf) font we've included. We think pairs well with cathode, installing this font and increasing your terminal's font size to about 24pt makes this whole experience much more immersive. Have fun!

***

### macOS

```
brew install pkg-config
brew tap homebrew/science
brew install portaudio opencv
make cathode
make install
```

### linux

```
yum install portaudio opencv
make cathode
make install
```
 - You might also need: `portaudio-devel` `opencv-devel` `ncurses-devel`


## Usage

Generate ID (or display a previously generated ID)

- `cathode -M`

To view yourself:

- `cathode -Z [your_id] -v`

If you have a friend's ZeroTier ID or ZeroTier IP address you can connect to them via:

- `cathode -Z [id] -v`
- `cathode -N [nwid] -R [id] -v`
- `cathode -N [nwid] -S [zt-ipv6-address] -v`

Where `-v` is to enable video (as opposed to only audio)

## Uninstall
 - `make uninstall`

***

## Notes

This app is meant to be a simple demonstration of how you can add ZeroTier to your application. In this example there's only about ten points where modifications to `p2pvc` were needed. Since we want all network traffic to be handled by ZeroTier we swapped out the system's native calls with our own. 

For instance, the `p2plib.c`'s `p2p_connect()` function, we see:

```
con->socket = socket(curr_addr->ai_family, curr_addr->ai_socktype, curr_addr->ai_protocol);
```

We change it to the following:

```
con->socket = zts_socket(curr_addr->ai_family, curr_addr->ai_socktype, curr_addr->ai_protocol);
```

We also do this for every instance of `bind()`, `connect()`, `accept()`, `listen()`, etc. The substituted calls are designed to be perfectly compatible with each system's variation so substitution could be as simple as a find+replace.
