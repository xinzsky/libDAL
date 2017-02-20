#ifndef SHARDING_H
#define SHARDING_H

typedef  int  serverid_t;

#define UNKNOW_SERVER       -1

#define SHARD_TYPE_MOD      1
#define SHARD_TYPE_RANGE    2
#define SHARD_TYPE_LOOKUP   3

typedef struct {
  unsigned int min;
  unsigned int max;
}idrange_t;

enum {
  PKTYPE_UINT = 0,
  PKTYPE_UINT_STR,
  PKTYPE_UINT_HEX,
  PKTYPE_STRING,
  PKTYPE_BINARY
};

typedef struct {
  int type;
  int pktype;
  int servernum;
  serverid_t *servers;
  idrange_t *idrange;
}sharding_t;


#ifdef __cplusplus
extern "C"
{
#endif

  serverid_t sharding(sharding_t *shard,const char *pk,int pklen);

#ifdef __cplusplus
}
#endif

#endif




