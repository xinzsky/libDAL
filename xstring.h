#ifndef XSTRING_H
#define XSTRING_H

#define COUNTOF(array)        (sizeof (array) / sizeof ((array)[0]))
#define countof(array)        (sizeof (array) / sizeof ((array)[0]))


#ifdef __cplusplus
extern "C"
{
#endif
 
  void trim(char *str);
  char *strsqueeze(char *str);
  int strfragnum(const char *str, const char *delimiter);
  int split(char * str, const char *delimiter, char * fragment[], int frags);
  char * stristr(const char *StrBase, const char *SubBase);
  char *bin2hex(unsigned char *bin, int binlen, char *hex);
  int hex2bin(char *hex,unsigned char *bin);
  
#ifdef __cplusplus
}
#endif

#endif



