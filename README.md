# cathode
A private P2P terminal video chat app

***
Cathode is a weakly-interacting coupling of two peices of software. First we start with [p2pvc](https://github.com/mofarrell/p2pvc) (the brilliant little app that renders the audio/video in your terminal), and [ZeroTier](https://github.com/zerotier) (an extremely small but very robust encrypted P2P VPN layer). We then replace all of the socket API system calls with their ZeroTierSDK equivalents aaaaannnd we're done.

***

# Installation

- `make`
- `make install`

# Usage

Via ZeroTier-issued IP address:

`cathode 10.6.6.8 -v -port 4545'

or, via ZeroTier identity:

`cathode 92a0fe61ba -v -port 4545' 
