# cathode
A private P2P terminal video chat app

***
Cathode is a weakly-interacting coupling of two peices of software. First we start with [p2pvc](https://github.com/mofarrell/p2pvc) (the brilliant little app that renders the audio/video in your terminal), and then [ZeroTier](https://github.com/zerotier) (an extremely small but very robust encrypted P2P VPN layer). SDK can be found [here](https://github.com/zerotier/ZeroTierSDK). We then replace all of the socket API system calls with their ZeroTierSDK equivalents aaaaannnd we're done.

This app is meant to be a simple demonstration of how you can add ZeroTier to your application. In this example there's only about ten points where modifications to `p2pvc` were needed. Since we want all network traffic to be handled by ZeroTier we swapped out the system's native calls with our own. 

For instance, the `p2plib.c`'s `p2p_connect()` function, we see:

```
...
con->socket = socket(curr_addr->ai_family, curr_addr->ai_socktype, curr_addr->ai_protocol);
...
```

```
...
con->socket = zts_socket(curr_addr->ai_family, curr_addr->ai_socktype, curr_addr->ai_protocol);
...
```

We also do this for every instance of `bind()`, `connect()`, `accept()`, `listen()`, etc. The substituted calls are designed to be perfectly compatible with each system's variation so substitution could be as simple as a find+replace.

***

# Installation

- `make`
- `make install`

# Usage

Via ZeroTier-issued IP address:

`cathode 10.6.6.8 -v -port 4545`

or, via ZeroTier identity:

`cathode 92a0fe61ba -v -port 4545`
