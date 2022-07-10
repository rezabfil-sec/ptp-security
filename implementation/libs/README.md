# Hashing Libraries
Pre-built hashing libraries are stored here. For Blake2, the ready-to-use `libb2-dev` package is used.
For libsodium, the `libsodium-dev` package is used. Please make sure these dependencies are met.

## libblake3.so 
SSE version, built from the local blake3 directory from git revision `4e84c8c7ae3da71` with the following command (as per https://github.com/BLAKE3-team/BLAKE3/tree/master/c):
```
gcc -shared -O3 -o libblake3.so blake3.c blake3_dispatch.c blake3_portable.c \
blake3_sse2_x86-64_unix.S blake3_sse41_x86-64_unix.S blake3_avx2_x86-64_unix.S \
blake3_avx512_x86-64_unix.S
```
Linked in `makefile:24`.
    