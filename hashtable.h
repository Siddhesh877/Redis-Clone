#pragma once

#include<stddef.h>
#include<stdint.h>

//hashtable node and payload embedded in same node
struct HNode
{
    HNode *next=NULL;
    uint64_t hcode=0;
};

//a simple fixed size hashtable
struct HTab
{
    HNode **tab=NULL;
    size_t mask=0;
    size_t size=0;
};

struct HMap
{
    HTab ht1; //new table
    HTab ht2; //older table
    size_t resizing_pos; //position in ht2 from where elements are moved
};

HNode *hm_lookup(HMap *hmap,HNode *key,bool(*eq)(HNode *,HNode *));
void hm_insert(HMap *hmap,HNode *node);
HNode *hm_pop(HMap *hmap, HNode *key, bool (*eq)(HNode *, HNode *));
size_t hm_size(HMap *hmap);
void hm_destroy(HMap *hmap);