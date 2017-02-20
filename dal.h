#ifndef DAL_H
#define DAL_H

#define DAL_ACCESS_READ          1
#define DAL_ACCESS_WRITE         2

#ifdef __cplusplus
extern "C"
{
#endif

  void * dal_new(const char *confpath);
  void dal_del(void *dal);
  int dal_getserver(void *dal,const char *pk,int pklen,int access,char *server);
  void dal_setfailserver(void *dal,int serverid);
  void dal_flushlog(void *dal,int serverid);


#ifdef __cplusplus
}
#endif

#endif





