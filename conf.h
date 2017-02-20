#ifndef CONF_H
#define CONF_H

enum
{
  CONF_INT = 1,
  CONF_STR,
  CONF_FLOAT,
  CONF_MULTI,
  CONF_DIR,
  CONF_PATH
};

/*
 * �������Ϊ����: ��ѡ����ѡ����������Ҫ֪����Щ�Ǳ�ѡ�ģ���Щ�ǿ�ѡ�ġ�
 * ������ȡֵ��Դֻ������: Ҫô���û�ָ����Ҫô����Ĭ��ֵ��
 * ��ѡ�����������ֵ,��ѡ����������п��ޡ�
 * isnull����ָ��������������Ƿ�һ��Ҫ���û�ָ��ȡֵ��ֻ����û��Ĭ��ֵ�ı�ѡѡ�
 * defvalָ��������û�û��ָ���������Ĭ��ֵ��
*/


typedef struct
{
  char *section;
  char *key;
  int  type;
  int isnull; //ָ�����������ֵ�Ƿ�������û�ָ��������û��Ĭ��ֵ�ı�ѡѡ�
  char *defval;
  void *p1;  //int:int min;multi:char ***mulval;string:char **vals; path: char*home;
  void *p2;  //int:int max;multi:char *sepstr;  string:int valnum;  path: char*dir;
  void *p3;  //int:NULL;   multi:int  *mvnum;   string:int *ivals;  path: NULL
  void *r;
}conf_t;


#ifdef __cplusplus
extern "C"
{
#endif
  void *init_conf(void);
  int load_conf(void *cfgdata,const char *filename);
  int parse_conf(void *cfgdata,char *conf_str);
  int save_conf(void *cfgdata,const char *filename);
  void get_conf(void *cfgdata,conf_t *conf,int confnum);
  void free_conf(void *cfgdata);

  const char *get_key_value(void *cfgdata,const char *section, const char *key_name, const char *def);
  int set_key_value(void *cfgdata,const char *section, const char *key_name, const char *set);

  
  int getconfint(void *cfgdata,const char *section, const char *key, int isnull, int notfound, int min, int max);
  double getconfloat(void *cfgdata,const char *section, const char *key, int isnull, double notfound, double min, double max);
  char *getconfstring(void *cfgdata,const char *section, const char *key, int isnull, char *notfound,
                            char **values, int *ivalues, int valnum, int *retival);
  char *getconfmulti(void *cfgdata,const char *section, const char *key, int isnull, char *notfound,
                     const char *sepstr, char ***values, int *valnum);
  char *getconfdir(void *cfgdata,const char *section, const char *key, int isnull, char *notfound);
  char *getconfpath(void *cfgdata,const char *section, const char *key, int isnull, char *notfound,char *home,char *dir);

#ifdef __cplusplus
}
#endif

#endif

