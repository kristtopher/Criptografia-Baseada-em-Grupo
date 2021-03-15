#include <iostream>
#include <string.h>

int count = 0;

void xtea_encrypt(const void *pt, void *ct, uint32_t *skey) {
    uint8_t i;
    uint32_t v0=((uint32_t*)pt)[0], v1=((uint32_t*)pt)[1];
    uint32_t sum=0, delta=0x9E3779B9;
    for(i=0; i<32; i++) {
        v0 += ((v1 << 4 ^ v1 >> 5) + v1) ^ (sum + ((uint32_t*)skey)[sum & 3]);      //8
        sum += delta;                                                               //1
        v1 += ((v0 << 4 ^ v0 >> 5) + v0) ^ (sum + ((uint32_t*)skey)[sum>>11 & 3]);  //9
        count += 18;
    }
    ((uint32_t*)ct)[0]=v0; ((uint32_t*)ct)[1]=v1;
}

void xtea_decrypt(const void *ct, void *pt, uint32_t *skey) {
    uint8_t i;
    uint32_t v0=((uint32_t*)ct)[0], v1=((uint32_t*)ct)[1];
    uint32_t sum=0xC6EF3720, delta=0x9E3779B9;
    for(i=0; i<32; i++) {
        v1 -= ((v0 << 4 ^ v0 >> 5) + v0) ^ (sum + ((uint32_t*)skey)[sum>>11 & 3]);
        sum -= delta;
        v0 -= ((v1 << 4 ^ v1 >> 5) + v1) ^ (sum + ((uint32_t*)skey)[sum & 3]);
    }
    ((uint32_t*)pt)[0]=v0; ((uint32_t*)pt)[1]=v1;
}

int main() {
    int block_size = 8;
    unsigned char ct[block_size];
    unsigned char pt[block_size]= {0x28, 0x85, 0x84, 0x2e, 0xa3, 0xe6, 0x99, 0x9b};//v0[i] = 0x2885842e;v1[i] = 0xa3e6999b;
    uint32_t skey[4] = {0x9474B8E8, 0xC73BCA7D, 0x53239142, 0xf3c3121a};

    unsigned char key[16]  = {0x94,0x74,0xB8,0xE8,0xC7,0x3B,0xCA,0x7D,0x53,0x23,0x91,0x42,0xf3,0xc3,0x12,0x1a};

   printf("XTEA\ninput\n");
    for (int i = 0; i < block_size; ++i) {
        printf("%x ",pt[i]);
    }
    printf("\n");

    xtea_encrypt(pt,ct,skey);
    printf(">%d\n",count);

    printf("Crypt\n");
    for (int i = 0; i < block_size; ++i) {
        printf("%x ",ct[i]);
    }
    printf("\n");

    xtea_decrypt(ct, pt,skey);

    printf("Decrypt\n");
    for (int i = 0; i < block_size; ++i) {
        printf("%x ",pt[i]);
    }
    return 0;
}