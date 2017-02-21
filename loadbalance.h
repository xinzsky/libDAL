#ifndef LOADBALANCE_H
#define LOADBALANCE_H

#include "sharding.h"

#define LB_TYPE_NONE            0  
#define LB_TYPE_RR              1  // Round Robin
#define LB_TYPE_WRR             2  // Weighted Round Robin

#define LB_ACCESS_READ          1
#define LB_ACCESS_WRITE         2
#define LB_ACCESS_RW            3

#define FAIL_SERVER             -1  // 故障服务器
#define LAST_SERVER             -1  // 上次访问服务器的初始值
#define MAXWEIGHT_INIT          -1  // 最大权重初始值
#define SERVERID_INIT           -1

typedef struct {
  int wrr_cw;			        /* current weight */
  int wrr_mw;			        /* maximum weight [0,+)*/
	int wrr_di;			        /* decreasing interval [1,+)*/
  serverid_t wrr_last;    /* last access server */
}lb_status_t;

typedef struct {
  int servernum;
  serverid_t *servers;
  lb_status_t status;     // 用于服务器组内的轮询
}lb_servergroup_t;

typedef struct {
  serverid_t sid;         // 初始化为-1
  int access;
  int weight;             // 初始值为0，weight = -1,表示服务器故障
  int oriweight;          // 当服务器down时，保存原来权重值
  time_t downtime;        // 服务器down时间
  
  // only used for master
  lb_servergroup_t *readgroup;
  lb_servergroup_t *writegroup;
}lb_server_t;

typedef struct {
  int type;
  int servernum;
  lb_server_t *servers;
  int failkeep;          // 故障持续多久自动恢复
  int failnum;           // 故障服务器个数
}lb_t;


#ifdef __cplusplus
extern "C"
{
#endif

  int wrr_max_weight(lb_t *lb,lb_servergroup_t *lbgrp);
  int wrr_gcd_weight(lb_t *lb,lb_servergroup_t *lbgrp);
  serverid_t LB_RR(lb_t *lb,lb_servergroup_t *lbgrp);
  serverid_t LB_WRR(lb_t *lb,lb_servergroup_t *lbgrp);
  serverid_t scheduling(lb_t *lb,serverid_t master,int access);

#ifdef __cplusplus
}
#endif

#endif


