
// DO NOT USE THIS FOR ANYTHING!!
/**

[*] ECDH Test    : OK

 // ECDH shared secret
 0x19, 0x7d, 0xa3, 0x09, 0xe8, 0x40, 0x49, 0xac,
 0x62, 0x43, 0x8a, 0xd0, 0xab, 0xd3, 0x60, 0x20,
 0x3e, 0xb0, 0xec, 0x95, 0x43, 0xbb, 0xf0, 0x43,
 0xca, 0xf1, 0xa1, 0x3f, 0x0c, 0x30, 0x9b, 0x3d,

[*] RFC7748 Test            : OK
[*] RFC7748 Public Key Test : OK
[*] RFC7748 Shared Key Test : OK

 // RFC shared secret
 0x4a, 0x5d, 0x9d, 0x5b, 0xa4, 0xce, 0x2d, 0xe1,
 0x72, 0x8e, 0x3b, 0xf4, 0x80, 0x35, 0x0f, 0x25,
 0xe0, 0x7e, 0x21, 0xc9, 0x47, 0xd1, 0x9e, 0x33,
 0x76, 0xf0, 0x9b, 0x3c, 0x1e, 0x16, 0x17, 0x42,

*/


#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

typedef long long int u64;
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

typedef u64 gf[4];

void bin2hex(const char *s, uint8_t x[], int len);

void modulo(void *x);
void expmod(void *x);
void mulmod(void *r, void *a, void *b);
void submod(void *r, void *a, void *b);
void addmod(void *r, void *a, void *b);
void cswap(int, void *a, void *b);

// scalar multiplication on Montgomery curve
void scalarmult(void *r, void *k_in, void *m) {
      u8 k[32];
      gf v[8];

      #define x  v[0]
      #define a  v[1]
      #define b  v[2]
      #define c  v[3]
      #define d  v[4]
      #define e  v[5]
      #define f  v[6]
      #define g  v[7]

      int   i, p, bit, prev=0;

      memset(v, 0, sizeof(v));
      memcpy(k, k_in, 32);

      // clamp
      k[ 0] &= 248;
      k[31] &= 127;
      k[31] |= 64;

      // main loop
      memcpy(x, m, 32);

      x[31] &= 127;

      memcpy(b, x, 32);

      // g = 121665
      g[0] = 121665;

      // a =1; d = 1;
      a[0] = d[0] = 1;

      for (i=254; i>=0; --i) {
          bit = (k[i >> 3] >> (i & 7)) & 1;  // bit set?

          p = bit ^ prev;
          prev = bit;

          cswap(p, a, b);
          cswap(p, c, d);

          addmod(e, a, c);  // e = a + c
          submod(a, a, c);  // a = a - c

          addmod(c, b, d);  // c = b + d
          submod(b, b, d);  // b = b - d

          mulmod(d, e, e);  // d = e * e
          mulmod(f, a, a);  // f = a * a
          mulmod(a, c, a);  // a = c * a
          mulmod(c, b, e);  // c = b * e

          addmod(e, a, c);  // e = a + c
          submod(a, a, c);  // a = a - c

          mulmod(b, a, a);  // b = a * a
          submod(c, d, f);  // c = d - f

          mulmod(a, c, g);  // a = c * g
          addmod(a, a, d);  // a = a + d

          mulmod(c, c, a);  // c = c * a
          mulmod(a, d, f);  // a = d * f
          mulmod(d, b, x);  // d = b * x
          mulmod(b, e, e);  // b = e * e
      }
      expmod(c);
      mulmod(r, a, c);
      modulo(r);

      #undef x
      #undef a
      #undef b
      #undef c
      #undef d
      #undef e
      #undef f
      #undef g
}

#ifndef ASM

#if defined(_WIN32) || defined(_WIN64)
#define WINDOWS
#include <windows.h>
#include <wincrypt.h>
#pragma comment(lib, "advapi32")
#else
#define NIX
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#endif

void bin2hex(const char *s, uint8_t x[], int len) {
    int i;
    printf ("\n // %s", s);
    for (i=0; i<len; i++) {
      if ((i & 7)==0) putchar('\n');
      printf (" 0x%02x,", x[i]);
    }
    putchar('\n');
}

#if defined(NIX)

int random(void *out, size_t outlen)
{
    int     fd;
    ssize_t u=0, len;
    uint8_t *p=(uint8_t*)out;

    fd = open("/dev/urandom", O_RDONLY);

    if (fd >= 0) {
      for (u=0; u<outlen;) {
        len = read(fd, p + u, outlen - u);
        if (len<0) break;
        u += (size_t)len;
      }
      close(fd);
    }
    return u==outlen;
}

#else

int random(void *out, size_t outlen)
{
    HCRYPTPROV hp;
    BOOL       r=FALSE;

    if (CryptAcquireContext(&hp, 0, 0, PROV_RSA_FULL,
      CRYPT_VERIFYCONTEXT | CRYPT_SILENT))
    {
      r = CryptGenRandom(hp, outlen, out);
      CryptReleaseContext(hp, 0);
    }
    return r;
}

#endif

size_t hex2bin (void *bin, char hex[]) {
    size_t len, i;
    int x;
    uint8_t *p=(uint8_t*)bin;

    len = strlen (hex);

    if ((len & 1) != 0) {
      return 0;
    }

    for (i=0; i<len; i++) {
      if (isxdigit((int)hex[i]) == 0) {
        return 0;
      }
    }

    for (i=0; i<len / 2; i++) {
      sscanf (&hex[i * 2], "%2x", &x);
      p[i] = (uint8_t)x;
    }
    return len / 2;
}

// ===========================================================

// Doris private key, a:
char a_sk[]="77076d0a7318a57d3c16c17251b26645df4c2f87ebc0992ab177fba51db92c2a";

// Doris public key, X25519(a, 9):
char a_pk[]="8520f0098930a754748b7ddcb43ef75a0dbf3a0d26381af4eba4a98eaa9b4e6a";

// Boris private key, b:
char b_sk[]="5dab087e624a8a4b79e17f8b83800ee66f3bb1292618b6fd1c2f8b27ff88e0eb";

// Boris public key, X25519(b, 9):
char b_pk[]="de9edb7d7b7dc1b4d35b61c2ece435373f8343c85b78674dadfc7e146f882b4f";

// Their shared secret, K:
char ab_key[]="4a5d9d5ba4ce2de1728e3bf480350f25e07e21c947d19e3376f09b3c1e161742";

void test_rfc(void) {
    int equ;
    u8  sk1[32], sk2[32], pk1[32], pk2[32], k1[32], k2[32], key[32];
    u8  base[32] = {9};
    u8 t1[32], t2[32];

    // set private key for Doris and Boris
    hex2bin(sk1, a_sk);
    hex2bin(sk2, b_sk);

    // set public key for Doris and Boris
    hex2bin(pk1, a_pk);
    hex2bin(pk2, b_pk);

    // set key that we need
    hex2bin(key, ab_key);

    // generate public key for Doris
    scalarmult(t1, sk1, base);
    // generate shared secret for Doris
    scalarmult(t2, sk1, pk2);

    // set shared secret for Doris and Boris
    scalarmult(k1, sk1, pk2);
    scalarmult(k2, sk2, pk1);

    // check if equal with test vector
    equ = (memcmp(k1, key, sizeof(key))==0);
    printf("\n[*] RFC7748 Test            : %s\n", equ ? "OK" : "FAILED");

    equ = (memcmp(t1, pk1, sizeof(key))==0);
    printf("[*] RFC7748 Public Key Test : %s\n", equ ? "OK" : "FAILED");

    equ = (memcmp(t2, k2, sizeof(key))==0);
    printf("[*] RFC7748 Shared Key Test : %s\n", equ ? "OK" : "FAILED");

    bin2hex("RFC shared secret", k1, 32);
}

void test_ecdh(void) {
    u8  sk1[32], pk1[32], k1[32]; // keys for Doris
    u8  sk2[32], pk2[32], k2[32]; // keys for Boris
    u8  base[32] = {9};
    int equ, i;

    // Doris generates private key
    random(sk1, 32);

    // Boris generates private key
    random(sk2, 32);

    // Doris generates public key
    scalarmult(pk1, sk1, base);

    // Boris generates public key
    scalarmult(pk2, sk2, base);

    // Doris sends pk1 to Boris
    // Boris sends pk2 to Doris

    // Doris does
    scalarmult(k1, sk1, pk2);

    // Boris does
    scalarmult(k2, sk2, pk1);

    // k1 and k2 should be equal
    equ = (memcmp(k1, k2, 32)==0);

    printf("\n[*] ECDH Test    : %s\n", equ ? "OK" : "FAILED");
    bin2hex("ECDH shared secret", k1, 32);
}

int main(void)
{
    test_ecdh();
    test_rfc();
    return 0;
}

#endif
