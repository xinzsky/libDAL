#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xstring.h"
#include "sharding.h"

static serverid_t sharding_mod(sharding_t *shard,unsigned int id)
{
  return (serverid_t)(id % shard->servernum);
}

static serverid_t sharding_range(sharding_t *shard,unsigned int id)
{
  int i;
  serverid_t sid;

  for(i=0;i<shard->servernum;i++) {
    sid = shard->servers[i];
    if(id >= shard->idrange[i].min && id < shard->idrange[i].max) // [min,max)
      return sid;
  }

  return UNKNOW_SERVER;
}

static serverid_t sharding_lookup(sharding_t *shard,unsigned int id)
{
  return UNKNOW_SERVER;
}


static unsigned long hashpjw(char *key, unsigned int key_length)
{
  unsigned long h, g;
  char *p;

  h = 0;
  p = key + key_length;
  while (key < p)
  {
    h = (h << 4) + *key++;
    if ((g = (h & 0xF0000000)))
    {
      h = h ^(g >> 24);
      h = h ^ g;
    }
  }

  return h;
}

serverid_t sharding(sharding_t *shard,const char *pk,int pklen)
{
  unsigned int id;
  serverid_t serverid;
  char pkbuf[1024];

  if(!pk || pklen <= 0) //pk[0]='\0'
    return UNKNOW_SERVER;

  if(shard->pktype == PKTYPE_UINT) {
    if(pklen != sizeof(unsigned int)) 
      return UNKNOW_SERVER;
    memcpy(&id,pk,pklen);
  } else if(shard->pktype == PKTYPE_UINT_HEX) {
    if(pklen >= sizeof(pkbuf)) 
      return UNKNOW_SERVER;
    memcpy(pkbuf,pk,pklen);
    pkbuf[pklen] = 0;
    hex2bin(pkbuf,(unsigned char *)&id);
  } else if(shard->pktype == PKTYPE_UINT_STR) {
    if(pklen >= sizeof(pkbuf)) 
      return UNKNOW_SERVER;
    memcpy(pkbuf,pk,pklen);
    pkbuf[pklen] = 0;
    id = (unsigned int)strtoul(pkbuf,NULL,10);
  } else if(shard->pktype == PKTYPE_STRING || shard->pktype == PKTYPE_BINARY) {
    id = (unsigned int)hashpjw((char *)pk,pklen);
  } else {
    return UNKNOW_SERVER;
  }

  switch(shard->type)
  {
    case SHARD_TYPE_MOD:
      serverid = sharding_mod(shard,id);
      break;
    case SHARD_TYPE_RANGE:
      serverid = sharding_range(shard,id);
      break;
    case SHARD_TYPE_LOOKUP:
      serverid = sharding_lookup(shard,id);
      break;
    default:
      serverid = UNKNOW_SERVER;
  }

  return serverid;
}

