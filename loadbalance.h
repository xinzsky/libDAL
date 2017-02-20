#ifndef LOADBALANCE_H
#define LOADBALANCE_H

#include "sharding.h"

#define LB_TYPE_NONE            0  
#define LB_TYPE_RR              1  // Round Robin
#define LB_TYPE_WRR             2  // Weighted Round Robin

#define LB_ACCESS_READ          1
#define LB_ACCESS_WRITE         2
#define LB_ACCESS_RW            3

#define FAIL_SERVER             -1  // ���Ϸ�����
#define LAST_SERVER             -1  // �ϴη��ʷ������ĳ�ʼֵ
#define MAXWEIGHT_INIT          -1  // ���Ȩ�س�ʼֵ
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
  lb_status_t status;     // ���ڷ��������ڵ���ѯ
}lb_servergroup_t;

typedef struct {
  serverid_t sid;         // ��ʼ��Ϊ-1
  int access;
  int weight;             // ��ʼֵΪ0��weight = -1,��ʾ����������
  int oriweight;          // ��������downʱ������ԭ��Ȩ��ֵ
  time_t downtime;        // ������downʱ��
  
  // only used for master
  lb_servergroup_t *readgroup;
  lb_servergroup_t *writegroup;
}lb_server_t;

typedef struct {
  int type;
  int servernum;
  lb_server_t *servers;
  int failkeep;          // ���ϳ�������Զ��ָ�
  int failnum;           // ���Ϸ���������
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


