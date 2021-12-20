# Quark
Modular implementations of various Web Standards. 

# Dependencies

Event driven APIs are built on top of libuv.

Crypto APIs are built on top of OpenSSL.

Language bindings are provided for V8 and QuickJS.

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

## WHATWG APIs
Modules that implement WHATWG APIs
- URL (quarkurl)

## Runtimes
Base runtime executables for various engines. These runtimes are to be used as a base for the bindings provided by quark modules.

Engines:
- V8 (quarkjs-v8)
- QuickJS (quarkjs-qjs)
