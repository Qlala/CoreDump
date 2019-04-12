/* This file was automatically generated.  Do not edit! */
#undef INTERFACE
void style_footer(void);
const char *cgi_parameter(const char *zName,const char *zDefault);
#define PD(x,y)     cgi_parameter((x),(y))
void cgi_printf(const char *zFormat,...);
void style_submenu_element(const char *zLabel,const char *zLink,...);
void style_header(const char *zTitleFormat,...);
void cgi_replace_parameter(const char *zName,const char *zValue);
int cgi_parameter_boolean(const char *zName);
#define PB(x)       cgi_parameter_boolean(x)
void cookie_page(void);
const char *cookie_value(const char *zPName,const char *zDefault);
typedef struct Blob Blob;
char *blob_str(Blob *p);
void cgi_set_cookie(const char *zName,const char *zValue,const char *zPath,int lifetime);
void blob_appendf(Blob *pBlob,const char *zFormat,...);
void blob_append(Blob *pBlob,const char *aData,int nData);
void blob_init(Blob *pBlob,const char *zData,int size);
struct Blob {
  unsigned int nUsed;            /* Number of bytes used in aData[] */
  unsigned int nAlloc;           /* Number of bytes allocated for aData[] */
  unsigned int iCursor;          /* Next character of input to parse */
  unsigned int blobFlags;        /* One or more BLOBFLAG_* bits */
  char *aData;                   /* Where the information is stored */
  void (*xRealloc)(Blob*, unsigned int); /* Function to reallocate the buffer */
};
void cookie_render(void);
void cookie_link_parameter(const char *zQP,const char *zPName,const char *zDflt);
void cookie_write_parameter(const char *zQP,const char *zPName,const char *zDflt);
void cookie_read_parameter(const char *zQP,const char *zPName);
void cgi_set_parameter_nocopy(const char *zName,const char *zValue,int isQP);
int dehttpize(char *z);
int fossil_isspace(char c);
char *mprintf(const char *zFormat,...);
#define P(x)        cgi_parameter((x),0)
void cookie_parse(void);
# define DISPLAY_SETTINGS_COOKIE    "fossil_display_settings"
#define INTERFACE 0
