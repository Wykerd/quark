# Quark
Modular implementations of various Web Standards. 

# Dependencies

Event driven APIs are built on top of libuv.

Crypto APIs are built on top of OpenSSL.

# Modules

## Underlying
Modules used as underlying implementations for Web APIs
- Standard Library (quarkstd)
  - Memory allocation
    - Leak Detection
    - Allocation Stack Traces
  - HashTables
    - SpookyHash
  - Dynamic Buffers
    - Strings 
- Networking (quarknet)
  - DNS
  - TCP
  - TLS 
  - WS
