#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


#include ".\include\hash.h"

uint32_t hashf(const char* str, uint32_t h){
    /* One-byte-at-a-time Murmur hash 
    Source: https://github.com/aappleby/smhasher/blob/master/src/Hashes.cpp */
    for (; *str; ++str) {
        h ^= *str;
        h *= 0x5bd1e995;
        h ^= h >> 15;
    }
    return h;
}

uint32_t hashf2(const char *str) {
    
    uint32_t h = 0;
    for (; *str; ++str) {
        h = 17 * h + *str;
    }
    return h;
}


int  hash_insere(thash * h, void * bucket){
    uint32_t hash = hashf(h->get_key(bucket),SEED);
    uint32_t hash2 = hashf2(h->get_key(bucket));
    int pos = hash %(h->max);
    int hdd = hash2 % (h->max -1) + 1;

    /*se esta cheio*/
    if (h->max == (h->size+1)){
        free(bucket);
        return EXIT_FAILURE;
    }else{  /*fazer a insercao*/
        while(h->table[pos] != 0){
            if (h->table[pos] == h->deleted)
                break;
            pos = (pos+hdd)%h->max;
        }
        h->table[pos] = (uintptr_t)bucket;
        h->size +=1;
    }
    return EXIT_SUCCESS;
}

int hash_constroi(thash * h,int nbuckets, char * (*get_key)(void *) ){
    h->table = calloc(sizeof(void *),nbuckets + 1);
    if (h->table == NULL){
        return EXIT_FAILURE;
    }
    h->max = nbuckets + 1;
    h->size = 0;
    h->deleted = (uintptr_t)&(h->size);
    h->get_key = get_key;
    return EXIT_SUCCESS;
}

void * hash_busca(thash  h, const char * key){
    int pos = hashf(key,SEED) % (h.max);
    int hdd = hashf2(key) % (h.max -1) +1;
    void * ret = NULL;
    
    while(h.table[pos]!=0 && ret == NULL){
        if (strcmp(h.get_key((void*)h.table[pos]),key) == 0){
            ret = (void *) h.table[pos];
        }else{
            pos = (pos+hdd) % h.max;
        }
    }
    
    return ret;

}

int hash_remove(thash * h, const char * key){
    int pos = hashf(key,SEED) % (h->max);
    int hdd = hashf2(key) % (h->max -1) +1;
    while(h->table[pos]!=0){
        if (strcmp(h->get_key((void*)h->table[pos]),key) == 0){ /* se achei remove*/
            free((void *)h->table[pos]);
            h->table[pos] = h->deleted;
            h->size -= 1;
            return EXIT_SUCCESS;
        }else{
            pos = (pos+hdd) % h->max;
        }
    }
    return EXIT_FAILURE;
}

void hash_apaga(thash *h){
    int pos;
    for (pos =0; pos < h->max;pos++){
        if (h->table[pos] != 0){
            if (h->table[pos]!=h->deleted){
                free((void*) h->table[pos]);
            }
        }
    }
    free(h->table);
}

// abb

