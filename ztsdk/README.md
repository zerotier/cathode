This directory includes all of the parts necessary to use the ZeroTierSDK in a statically-linked linked manner.

`libzt.a` is the static library containing ZeroTier
`libpicotcp.so` is a dynamic library which contains the picoTCP stack (you don't really need to know this)
`sdk.h` is the header required to be included in your app