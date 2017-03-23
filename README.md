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
A private P2P terminal video chat app

***
Cathode is a weakly-interacting coupling of two peices of software. First we start with [p2pvc](https://github.com/mofarrell/p2pvc) (the brilliant little app that renders the audio/video in your terminal), and then [ZeroTier](https://github.com/zerotier) (an extremely small but very robust encrypted P2P VPN layer). SDK can be found [here](https://github.com/zerotier/ZeroTierSDK). We then replace all of the socket API system calls with their ZeroTierSDK equivalents aaaaannnd we're done.

Hint: Use the [Glass TTY VT220](Glass_TTY_VT220.ttf) font we've included. We think pairs well with cathode, installing this font and increasing your terminal's font size to about 24pt makes this whole experience much more immersive. Have fun!

***

## Installation (from prebuilt binaries in `dist/`)

### macOS
 - brew install portaudio opencv
 - make install

### linux
 - yum install portaudio opencv
 - make install
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

## Build
### ZeroTierSDK Library
 - Get source from [here](https://github.com/zerotier/ZeroTierSDK)
 - macOS:
 	- `make osx_static_lib SDK_IPV6=1 SDK_PICOTCP=1`
 - linux:
 	- `make linux_static_lib SDK_IPV6=1 SDK_PICOTCP=1`

### Cathode
 - Copy `build/` contents from `ZeroTierSDK` into `cathode/zt/` under the correct system folder
 - make
 - make install

***

## Security Implications
There are two very distinct modes of operation here: one secure, one *not*. If you chat with a friend via `cathode -Z [id] -v` that will broadcast over a public ZeroTier ad-hoc network and anyone who has either ID can easily tune in and watch/listen to either of you. However, if you use `cathode -Z [nwid] ...` this limits your traffic to only participants of the network you've created or joined. 

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

***

If statically-linking isn't an option for your application, we've developed a number of workarounds including function interposition via LD_PRELOAD and SOCKS5 proxy. You can read more about this [here](https://github.com/zerotier/ZeroTierSDK/blob/master/docs/walkthrough.md)
