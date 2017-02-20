#include <stdio.h>
#include <stdlib.h>
#include "loadbalance.h"

serverid_t LB_RR(lb_t *lb,lb_servergroup_t *lbgrp)
{
  int i,j,k;
  serverid_t sid;

  for(i=0;i<lbgrp->servernum;i++) {
    if(lbgrp->servers[i] == lbgrp->status.wrr_last)
      break;
  }

  if(i == lbgrp->servernum)
    i = lbgrp->servernum - 1;
  
  j = i;
  do {
    j = (j + 1) % lbgrp->servernum;
    sid = lbgrp->servers[j];
    
    for(k=0;k<lb->servernum;k++) {
      if(lb->servers[k].sid == sid)
        break;
    }

    if(k < lb->servernum && lb->servers[k].weight != FAIL_SERVER)
      return sid;
   
  } while (j != i);
  
  return UNKNOW_SERVER;
}


/*
 *    Get the maximum weight of the server group.
 */
int wrr_max_weight(lb_t *lb,lb_servergroup_t *lbgrp)
{
	int i,k,new_weight, weight = 0;
  serverid_t sid;

  for(i=0;i<lbgrp->servernum;i++)
  {
    sid = lbgrp->servers[i];
    for(k=0;k<lb->servernum;k++) {
      if(lb->servers[k].sid == sid) {
        new_weight = lb->servers[k].weight;
        break;
      }
    }

    if(k == lb->servernum)
      new_weight = 0;
      
    if (new_weight > weight)
			weight = new_weight;
  }

	return weight;
}


/*
 * swap - swap value of @a and @b, typeof is GNU C extend.
 */
#define swap(a, b) \
	do { typeof(a) __tmp = (a); (a) = (b); (b) = __tmp; } while (0)


/* Greatest common divisor */
static unsigned long gcd(unsigned long a, unsigned long b)
{
	unsigned long r;

	if (a < b)
		swap(a, b);
	while ((r = a % b) != 0) {
		a = b;
		b = r;
	}
	return b;
}

int wrr_gcd_weight(lb_t *lb,lb_servergroup_t *lbgrp)
{
	int i,k,weight;
	int g = 0;
  serverid_t sid;
  
  for(i=0;i<lbgrp->servernum;i++)
  {
    sid = lbgrp->servers[i];
    for(k=0;k<lb->servernum;k++) {
      if(lb->servers[k].sid == sid) {
        weight = lb->servers[k].weight;
        break;
      }
    }

    if(k == lb->servernum)
      weight = 0;
    
    if (weight > 0) {
			if (g > 0)
				g = gcd(weight, g);
			else
				g = weight;
		}
  }

	return g ? g : 1;
}


serverid_t LB_WRR(lb_t *lb,lb_servergroup_t *lbgrp)
{
  int i,k;
  serverid_t sid;
  int cw;			/* current weight */
	int mw;			/* maximum weight */
	int di;			/* decreasing interval */

  cw = lbgrp->status.wrr_cw;
  mw = lbgrp->status.wrr_mw;
  di = lbgrp->status.wrr_di;
  if(mw < 0) {
	  mw = wrr_max_weight(lb,lbgrp);
    lbgrp->status.wrr_mw = mw;
  }
  if(di <= 0) {
	  di = wrr_gcd_weight(lb,lbgrp);
    lbgrp->status.wrr_di = di;
  }

  for(i=0;i<lbgrp->servernum;i++) {
    if(lbgrp->servers[i] == lbgrp->status.wrr_last)
      break;
  }

  if(i == lbgrp->servernum)
    i = lbgrp->servernum - 1;

  /*
	 * This loop will always terminate, because cw in (0, max_weight]
	 * and at least one server has its weight equal to max_weight.
	 */
  while (1) { 
    i = (i + 1) % lbgrp->servernum;
    if (i == 0) {
      cw = cw - di;
      if (cw <= 0) {
        cw = wrr_max_weight(lb,lbgrp); 
        if (cw == 0) { /* Still zero, which means no available servers. */
          lbgrp->status.wrr_cw = 0;
          lbgrp->status.wrr_mw = 0;
          return UNKNOW_SERVER;
        }
      }
    }

    sid = lbgrp->servers[i];
    for(k=0;k<lb->servernum;k++) {
      if(lb->servers[k].sid == sid)
        break;
    }
    
    if(k < lb->servernum && lb->servers[k].weight >= cw) {
      lbgrp->status.wrr_cw = cw;
      return sid;
    }
  }
}

serverid_t scheduling(lb_t *lb,serverid_t master,int access)
{
  int i;
  serverid_t sid;
  lb_servergroup_t *lbgrp;

  if(lb->type == LB_TYPE_NONE)
    return master;

  for(i=0;i<lb->servernum;i++) {
    if(lb->servers[i].sid == master) {
      if(access == LB_ACCESS_READ) 
        lbgrp = lb->servers[i].readgroup;
      else if(access == LB_ACCESS_WRITE) 
        lbgrp = lb->servers[i].writegroup;
      else
        return UNKNOW_SERVER;

      break;
    }
  }

  if(i == lb->servernum || !lbgrp)
    return UNKNOW_SERVER;

  switch(lb->type) {
    case LB_TYPE_RR:
      sid = LB_RR(lb,lbgrp);
      break;
    case LB_TYPE_WRR:
      sid = LB_WRR(lb,lbgrp);
      break;
    default:
      return UNKNOW_SERVER;
  }

  lbgrp->status.wrr_last = sid;
  return sid;
}


