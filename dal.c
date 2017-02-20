#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "xmalloc.h"
#include "conf.h"
#include "xstring.h"
#include "sharding.h"
#include "loadbalance.h"

typedef struct {
  char *name;
  char *host;
  int   port;
  int   logmeth; // log method: r w rw.
  char *logdir;
  FILE *logfp_r;
  FILE *logfp_w;
}server_t;

typedef struct {
  int servernum;
  server_t *servers;
}serverpool_t;

typedef struct {
  serverpool_t serverpool;
  sharding_t   shard;         
  lb_t         lb;
}dal_t;

static int dal_initcfg(dal_t *cfg,const char *confpath)
{
  int i,j,k,num,servernum,masternum;
  char *server,**servers,*master,**masters,*p,**pp,*t,*token;
  char *shardtype[] = {"MOD","RANGE","LOOKUP"};
  int ishardtype[] = {SHARD_TYPE_MOD,SHARD_TYPE_RANGE,SHARD_TYPE_LOOKUP};
  char *lbtype[] = {"NONE","RR","WRR"};
  int ilbtype[] = {LB_TYPE_NONE,LB_TYPE_RR,LB_TYPE_WRR};
  char *pktypes[] = {"uint","uint_str","uint_hex","string","binary"};
  int pktype[] = {PKTYPE_UINT,PKTYPE_UINT_STR,PKTYPE_UINT_HEX,PKTYPE_STRING,PKTYPE_BINARY};
  char *hbtime;
  char key[128];
  server_t *s;
  lb_server_t *ls,*bs;
  serverid_t sid;
  void *cfgdata;
  
  conf_t items[] = {
    {"ServerPool",  "Servers",   CONF_MULTI, 0,  NULL, (void *)&servers,(void *)" ",(void *)&servernum, &server},
    {"Sharding",    "Type",      CONF_STR,   0,  NULL, (void *)shardtype,(void *)COUNTOF(shardtype), (void *)ishardtype, &cfg->shard.type},
    {"Sharding",    "PKType",    CONF_STR,   0,  NULL, (void *)pktypes,(void *)countof(pktypes), (void *)pktype, &cfg->shard.pktype},
    {"Sharding",    "Servers",   CONF_MULTI, 0,  NULL, (void *)&masters,(void *)" ",(void *)&masternum, &master},
    {"LoadBalance", "Type",      CONF_STR,   1,  "NONE",(void *)lbtype,(void *)COUNTOF(lbtype), (void *)ilbtype, &cfg->lb.type},
    {"LoadBalance", "FailKeep",  CONF_STR,   1,  NULL,  NULL, NULL, NULL, &hbtime}
  };
  
  cfgdata = init_conf();
  if (load_conf(cfgdata,confpath) < 0) {
    printf("can't load %s\n",confpath);
    return -1;
  }

  get_conf(cfgdata,items,COUNTOF(items));
  
  cfg->serverpool.servernum = servernum;
  cfg->serverpool.servers = (server_t *)xcalloc(servernum,sizeof(server_t));
  for(i=0;i<servernum;i++) {
    cfg->serverpool.servers[i].name = xstrdup(servers[i]);
    p = getconfmulti(cfgdata,"ServerPool",servers[i],0,NULL,":",&pp,&num);
    if(num != 2 && num !=4) {
      printf("[%s] %s set error.\n","ServerPool",servers[i]);
      return -1;
    }
    cfg->serverpool.servers[i].host = xstrdup(pp[0]);
    cfg->serverpool.servers[i].port = atoi(pp[1]);
    if(num == 4) {
      if(strcasecmp(pp[2],"r") == 0)
        cfg->serverpool.servers[i].logmeth = LB_ACCESS_READ;
      else if(strcasecmp(pp[2],"w") == 0) 
        cfg->serverpool.servers[i].logmeth = LB_ACCESS_WRITE;
      else if(strcasecmp(pp[2],"rw") == 0 || strcasecmp(pp[2],"wr") == 0) 
        cfg->serverpool.servers[i].logmeth = LB_ACCESS_RW;
      else {
        printf("[%s] %s log method set error.\n","ServerPool",servers[i]);
        return -1;
      }

      cfg->serverpool.servers[i].logdir = xstrdup(pp[3]);
      if(cfg->serverpool.servers[i].logdir[strlen(cfg->serverpool.servers[i].logdir)-1] == '/') 
        cfg->serverpool.servers[i].logdir[strlen(cfg->serverpool.servers[i].logdir)-1] = 0;
    }
    xfree(pp);
    xfree(p);
  }

  cfg->shard.servernum = masternum;
  cfg->shard.servers = (serverid_t *)xcalloc(masternum,sizeof(serverid_t));
  for(i=0;i<masternum;i++) {
    for(j=0;j<servernum;j++) {
      if(strcasecmp(masters[i],servers[j]) == 0) {
        cfg->shard.servers[i] = j;
        break;
      }
    }
    if(j == servernum) {
      printf("server-%s isn't in server pool.\n",masters[i]);
      return -1;
    }
  }

  if(cfg->shard.type == SHARD_TYPE_RANGE) {
    cfg->shard.idrange = (idrange_t *)xcalloc(cfg->shard.servernum,sizeof(idrange_t));
    for(i=0;i<cfg->shard.servernum;i++) {
      sid = cfg->shard.servers[i];
      s = cfg->serverpool.servers + sid;
      sprintf(key,"%s_range",s->name);
      p = getconfmulti(cfgdata,"Sharding",key,0,NULL," ",&pp,&num);
      if(num != 2) {
        printf("[%s] %s set error.\n","Sharding",key);
        return -1;
      }
      cfg->shard.idrange[i].min = atoi(pp[0]);
      cfg->shard.idrange[i].max = atoi(pp[1]);
      if(cfg->shard.idrange[i].min > cfg->shard.idrange[i].max) {
        printf("[%s] %s set error.\n","Sharding",key);
        return -1;
      }
      xfree(p);
      xfree(pp);
    }
  }

  if(cfg->lb.type != LB_TYPE_NONE) {
    cfg->lb.servernum = cfg->serverpool.servernum;
    cfg->lb.servers = (lb_server_t *)xcalloc(cfg->lb.servernum,sizeof(lb_server_t));
    for(i=0;i<cfg->lb.servernum;i++)
      cfg->lb.servers[i].sid = SERVERID_INIT;
    
    for(i=0;i<cfg->shard.servernum;i++) {
      sid = cfg->shard.servers[i];
      s = cfg->serverpool.servers + sid;
      ls = cfg->lb.servers + sid;
      p = getconfmulti(cfgdata,"LoadBalance",s->name,0,NULL," ",&pp,&num);

      ls->sid = sid;
      ls->readgroup = (lb_servergroup_t *)xcalloc(1,sizeof(lb_servergroup_t));
      ls->readgroup->servernum = 0;
      ls->readgroup->servers = (serverid_t *)xcalloc(num,sizeof(serverid_t));
      ls->writegroup = (lb_servergroup_t *)xcalloc(1,sizeof(lb_servergroup_t));
      ls->writegroup->servernum = 0;
      ls->writegroup->servers = (serverid_t *)xcalloc(num,sizeof(serverid_t));
      
      for(j=0;j<num;j++) {
        t = pp[j];
        token = strsep(&t,":");  // server name
        for(k=0;k<cfg->serverpool.servernum;k++) {
          if(strcasecmp(token,cfg->serverpool.servers[k].name) == 0)
            break;
        }
        if(k == cfg->serverpool.servernum) {
          printf("[%s] %s server name set error.\n","LoadBalance",s->name);
          return -1;
        }

        bs = cfg->lb.servers + k;
        bs->sid = k;
        
        token = strsep(&t,":"); // access: r w rw
        if(!token) {
          printf("[%s] %s server access set error.\n","LoadBalance",s->name);
          return -1;
        }
        
        if(strcasecmp(token,"r") == 0) {
          ls->readgroup->servers[ls->readgroup->servernum++] = k;
          bs->access = LB_ACCESS_READ;
        } else if(strcasecmp(token,"w") == 0) {
          ls->writegroup->servers[ls->writegroup->servernum++] = k;
          bs->access = LB_ACCESS_WRITE;
        } else if(strcasecmp(token,"rw") == 0 || strcasecmp(token,"wr") == 0) {
          ls->readgroup->servers[ls->readgroup->servernum++] = k;
          ls->writegroup->servers[ls->writegroup->servernum++] = k;
          bs->access = LB_ACCESS_RW;
        } else {
          printf("[%s] %s server access set error.\n","LoadBalance",s->name);
          return -1;
        }

        if(cfg->lb.type == LB_TYPE_WRR) {
          token = strsep(&t,":");  // weight,optional
          if(token) 
            bs->weight = atoi(token);
        }
      }

      xfree(pp);
      xfree(p);
    }
  }

  if(hbtime) { // mins/secs/hours/days
    if((p = stristr(hbtime,"secs"))) {
      *p = 0;
      trim(hbtime);
      cfg->lb.failkeep = atoi(hbtime);
    } else if((p = stristr(hbtime,"mins"))) {
      *p = 0;
      trim(hbtime);
      cfg->lb.failkeep = atoi(hbtime) * 60;
    } else if((p = stristr(hbtime,"hours"))) {
      *p = 0;
      trim(hbtime);
      cfg->lb.failkeep = atoi(hbtime) * 3600;
    } else if((p = stristr(hbtime,"days"))) {
      *p = 0;
      trim(hbtime);
      cfg->lb.failkeep = atoi(hbtime) * 86400;
    }

    xfree(hbtime);
  }

  xfree(masters);
  xfree(master);
  xfree(servers);
  xfree(server);
  free_conf(cfgdata);
  return 0;
}

static void dal_freecfg(dal_t *cfg)
{
  int i;

  for(i=0;i<cfg->serverpool.servernum;i++) {
    xfree(cfg->serverpool.servers[i].name);
    xfree(cfg->serverpool.servers[i].host);
    xfree(cfg->serverpool.servers[i].logdir);
  }
  xfree(cfg->serverpool.servers);

  xfree(cfg->shard.servers);
  xfree(cfg->shard.idrange);

  for(i=0;i<cfg->lb.servernum;i++) {
    if(cfg->lb.servers[i].readgroup) {
      xfree(cfg->lb.servers[i].readgroup->servers);
      xfree(cfg->lb.servers[i].readgroup);
    }

    if(cfg->lb.servers[i].writegroup) {
      xfree(cfg->lb.servers[i].writegroup->servers);
      xfree(cfg->lb.servers[i].writegroup);
    }
  }
  xfree(cfg->lb.servers);
}

void * dal_new(const char *confpath)
{
  int i;
  dal_t *dal;
  char path[1024];

  dal = (dal_t *)xcalloc(1,sizeof(dal_t));
  if(dal_initcfg(dal,confpath) < 0) {
    xfree(dal);
    return NULL;
  }

  for(i=0;i<dal->serverpool.servernum;i++) {
    if(dal->serverpool.servers[i].logdir) {
      if(dal->serverpool.servers[i].logmeth == LB_ACCESS_READ) {
        snprintf(path,1024,"%s/%s-r.id",dal->serverpool.servers[i].logdir,dal->serverpool.servers[i].name);
        dal->serverpool.servers[i].logfp_r = fopen(path,"ab");
        if(!dal->serverpool.servers[i].logfp_r) {
          printf("[%s] %s log file (r) open error.\n","ServerPool",dal->serverpool.servers[i].name);
          return NULL;
        }
      } else if(dal->serverpool.servers[i].logmeth == LB_ACCESS_WRITE) {
        snprintf(path,1024,"%s/%s-w.id",dal->serverpool.servers[i].logdir,dal->serverpool.servers[i].name);
        dal->serverpool.servers[i].logfp_w = fopen(path,"ab");
        if(!dal->serverpool.servers[i].logfp_w) {
          printf("[%s] %s log file (w) open error.\n","ServerPool",dal->serverpool.servers[i].name);
          return NULL;
        }
      } else if(dal->serverpool.servers[i].logmeth == LB_ACCESS_RW) {
        snprintf(path,1024,"%s/%s-r.id",dal->serverpool.servers[i].logdir,dal->serverpool.servers[i].name);
        dal->serverpool.servers[i].logfp_r = fopen(path,"ab");
        if(!dal->serverpool.servers[i].logfp_r) {
          printf("[%s] %s log file (r) open error.\n","ServerPool",dal->serverpool.servers[i].name);
          return NULL;
        }

        snprintf(path,1024,"%s/%s-w.id",dal->serverpool.servers[i].logdir,dal->serverpool.servers[i].name);
        dal->serverpool.servers[i].logfp_w = fopen(path,"ab");
        if(!dal->serverpool.servers[i].logfp_w) {
          printf("[%s] %s log file (w) open error.\n","ServerPool",dal->serverpool.servers[i].name);
          return NULL;
        }
      } 
    }
  }
  
  for(i=0;i<dal->lb.servernum;i++)
  {
    if(dal->lb.servers[i].readgroup) {
      dal->lb.servers[i].readgroup->status.wrr_cw = 0;
      dal->lb.servers[i].readgroup->status.wrr_mw = MAXWEIGHT_INIT;
      dal->lb.servers[i].readgroup->status.wrr_di = 0;
      dal->lb.servers[i].readgroup->status.wrr_last = LAST_SERVER;
    }

    if(dal->lb.servers[i].writegroup) {
      dal->lb.servers[i].writegroup->status.wrr_cw = 0;
      dal->lb.servers[i].writegroup->status.wrr_mw = MAXWEIGHT_INIT;
      dal->lb.servers[i].writegroup->status.wrr_di = 0;
      dal->lb.servers[i].writegroup->status.wrr_last = LAST_SERVER;
    }
  }
  
  return dal;
}

void dal_del(void *dal)
{
  int i;
  dal_t *d = (dal_t *)dal;
 
  for(i=0;i<d->serverpool.servernum;i++) {
    if(d->serverpool.servers[i].logfp_r) fclose(d->serverpool.servers[i].logfp_r);
    if(d->serverpool.servers[i].logfp_w) fclose(d->serverpool.servers[i].logfp_w);
  }

  dal_freecfg(d);
  xfree(d);
}

// 返回服务器id和主机、服务器名
int dal_getserver(void *dal,const char *pk,int pklen,int access,char *server)
{
  int i;
  serverid_t master,sid;
  dal_t *d = (dal_t *)dal;
  time_t now;

  master = sharding(&d->shard,pk,pklen);
  if(master == UNKNOW_SERVER)
    return UNKNOW_SERVER;

  if(d->lb.type == LB_TYPE_NONE) {
    sid = master;
    goto END;
  }

  //检查是否有故障的服务器需要恢复
  if(d->lb.failnum && d->lb.failkeep) {
    now = time(NULL);
    for(i=0;i<d->lb.servernum;i++) {
      if(d->lb.servers[i].weight == FAIL_SERVER) {
        if(now - d->lb.servers[i].downtime > d->lb.failkeep) {
          d->lb.servers[i].weight = d->lb.servers[i].oriweight;
          d->lb.servers[i].downtime = 0;
          d->lb.servers[i].oriweight = 0;
          d->lb.failnum--;
          if(!d->lb.failnum)
            break;
        }
      }
    }
  }

  sid = scheduling(&d->lb,master,access);
  if(sid == UNKNOW_SERVER)
    return UNKNOW_SERVER;

END:
  if(d->serverpool.servers[sid].logdir) {
    char pkbuf[1024],*k;
    if(d->shard.pktype == PKTYPE_UINT) {
      snprintf(pkbuf,sizeof(pkbuf),"%u",(unsigned int)pk);
      k = pkbuf;
    } else if(d->shard.pktype == PKTYPE_UINT_STR) {
      memcpy(pkbuf,pk,pklen);
      pkbuf[pklen] = 0;
      k = pkbuf;
    } else if(d->shard.pktype == PKTYPE_UINT_HEX) {
      memcpy(pkbuf,pk,pklen);
      pkbuf[pklen] = 0;
      k = pkbuf;
    } else if(d->shard.pktype == PKTYPE_STRING) {
      k = xstrdupdelim(pk,pk+pklen);
    } else if(d->shard.pktype == PKTYPE_BINARY) {
      k = xcalloc(1,pklen*2+1);
      bin2hex((unsigned char *)pk,pklen,k);
    }
    
    if(access == LB_ACCESS_READ && d->serverpool.servers[sid].logfp_r)
      fprintf(d->serverpool.servers[sid].logfp_r,"%s\n",k);
    else if(access == LB_ACCESS_WRITE && d->serverpool.servers[sid].logfp_w)
      fprintf(d->serverpool.servers[sid].logfp_w,"%s\n",k);

    if(d->shard.pktype == PKTYPE_STRING || d->shard.pktype == PKTYPE_BINARY)
      xfree(k);
  }

  sprintf(server,"%s:%d:%s",d->serverpool.servers[sid].host,d->serverpool.servers[sid].port,d->serverpool.servers[sid].name);
  return sid;
}

void dal_setfailserver(void *dal,int serverid)
{
  int i;
  dal_t *d = (dal_t *)dal;

  for(i=0;i<d->lb.servernum;i++) {
    if(d->lb.servers[i].sid == serverid) {
      d->lb.servers[i].oriweight = d->lb.servers[i].weight;
      d->lb.servers[i].weight = FAIL_SERVER;
      d->lb.servers[i].downtime = time(NULL);
      d->lb.failnum++;
      break;
    }
  }
}

//  serverid < 0: flush all servers log.
void dal_flushlog(void *dal,int serverid) 
{
  dal_t *d = (dal_t *)dal;
  int i;

  if(serverid >= 0) {
    if(d->serverpool.servers[serverid].logfp_r) 
      fflush(d->serverpool.servers[serverid].logfp_r);

    if(d->serverpool.servers[serverid].logfp_w)
      fflush(d->serverpool.servers[serverid].logfp_w);
  } else {
    for(i=0;i<d->serverpool.servernum;i++) {
      if(d->serverpool.servers[i].logfp_r) 
        fflush(d->serverpool.servers[i].logfp_r);

      if(d->serverpool.servers[i].logfp_w)
        fflush(d->serverpool.servers[i].logfp_w);
    }
  }
}


#ifdef TEST_DAL
int main()
{
  void *dal;
  char buf[1024],server[1024];
  int sid,cnt;

  dal = dal_new("../../etc/dal-page.conf");
  if(!dal) {
    printf("dal_new() error.\n");
    return -1;
  }

  cnt = 0;
  while(fgets(buf,sizeof(buf),stdin)) {
    trim(buf);
    if(!buf[0]) continue;
    if(strcasecmp(buf,"exit") == 0) break;

    sid = dal_getserver(dal,buf,strlen(buf),LB_ACCESS_READ,server);
    if(sid >= 0) {
      printf("server: %d-%s\n",sid,server);
      if(cnt <= 3)
        dal_setfailserver(dal,sid);
      cnt++;
    } else 
      printf("all servers is invalid.\n");

    dal_flushlog(dal,sid);
  }

  dal_del(dal);
  return 0;
}

#endif

