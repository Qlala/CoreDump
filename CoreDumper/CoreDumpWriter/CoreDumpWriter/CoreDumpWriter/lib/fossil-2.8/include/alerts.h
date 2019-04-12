/* This file was automatically generated.  Do not edit! */
#undef INTERFACE
void cgi_print_all(int showAll,int onConsole);
void announce_page(void);
int captcha_needed(void);
void contact_admin_page(void);
void db_set_int(const char *zName,int value,int globalFlag);
int db_get_int(const char *zName,int dflt);
typedef struct Blob Blob;
void blob_truncate(Blob *p,int sz);
void fossil_trace(const char *zFormat,...);
#define SENDALERT_TRACE       0x0008    /* Trace operation for debugging */
typedef unsigned int u32;
void alert_backoffice(u32 mFlags);
void db_begin_write_real(const char *zStartFile,int iStartLine);
#define db_begin_write()          db_begin_write_real(__FILE__,__LINE__)
void test_add_alert_cmd(void);
void test_alert_cmd(void);
void alert_footer(Blob *pOut);
void email_header(Blob *pOut);
typedef struct Manifest Manifest;
void manifest_destroy(Manifest *p);
char *fossil_strdup(const char *zOrig);
#define CFTYPE_FORUM      8
Manifest *manifest_get(int rid,int cfType,Blob *pErr);
struct Blob {
  unsigned int nUsed;            /* Number of bytes used in aData[] */
  unsigned int nAlloc;           /* Number of bytes allocated for aData[] */
  unsigned int iCursor;          /* Next character of input to parse */
  unsigned int blobFlags;        /* One or more BLOBFLAG_* bits */
  char *aData;                   /* Where the information is stored */
  void (*xRealloc)(Blob*, unsigned int); /* Function to reallocate the buffer */
};
typedef struct ManifestFile ManifestFile;
struct Manifest {
  Blob content;         /* The original content blob */
  int type;             /* Type of artifact.  One of CFTYPE_xxxxx */
  int rid;              /* The blob-id for this manifest */
  char *zBaseline;      /* Baseline manifest.  The B card. */
  Manifest *pBaseline;  /* The actual baseline manifest */
  char *zComment;       /* Decoded comment.  The C card. */
  double rDate;         /* Date and time from D card.  0.0 if no D card. */
  char *zUser;          /* Name of the user from the U card. */
  char *zRepoCksum;     /* MD5 checksum of the baseline content.  R card. */
  char *zWiki;          /* Text of the wiki page.  W card. */
  char *zWikiTitle;     /* Name of the wiki page. L card. */
  char *zMimetype;      /* Mime type of wiki or comment text.  N card.  */
  char *zThreadTitle;   /* The forum thread title. H card */
  double rEventDate;    /* Date of an event.  E card. */
  char *zEventId;       /* Artifact hash for an event.  E card. */
  char *zTicketUuid;    /* UUID for a ticket. K card. */
  char *zAttachName;    /* Filename of an attachment. A card. */
  char *zAttachSrc;     /* Artifact hash for document being attached. A card. */
  char *zAttachTarget;  /* Ticket or wiki that attachment applies to.  A card */
  char *zThreadRoot;    /* Thread root artifact.  G card */
  char *zInReplyTo;     /* Forum in-reply-to artifact.  I card */
  int nFile;            /* Number of F cards */
  int nFileAlloc;       /* Slots allocated in aFile[] */
  int iFile;            /* Index of current file in iterator */
  ManifestFile *aFile;  /* One entry for each F-card */
  int nParent;          /* Number of parents. */
  int nParentAlloc;     /* Slots allocated in azParent[] */
  char **azParent;      /* Hashes of parents.  One for each P card argument */
  int nCherrypick;      /* Number of entries in aCherrypick[] */
  struct {
    char *zCPTarget;    /* Hash for cherry-picked version w/ +|- prefix */
    char *zCPBase;      /* Hash for cherry-pick baseline. NULL for singletons */
  } *aCherrypick;
  int nCChild;          /* Number of cluster children */
  int nCChildAlloc;     /* Number of closts allocated in azCChild[] */
  char **azCChild;      /* Hashes of referenced objects in a cluster. M cards */
  int nTag;             /* Number of T Cards */
  int nTagAlloc;        /* Slots allocated in aTag[] */
  struct TagType {
    char *zName;           /* Name of the tag */
    char *zUuid;           /* Hash of artifact that the tag is applied to */
    char *zValue;          /* Value if the tag is really a property */
  } *aTag;              /* One for each T card */
  int nField;           /* Number of J cards */
  int nFieldAlloc;      /* Slots allocated in aField[] */
  struct {
    char *zName;           /* Key or field name */
    char *zValue;          /* Value of the field */
  } *aField;            /* One for each J card */
};
typedef struct EmailEvent EmailEvent;
EmailEvent *alert_compute_event_text(int *pnEvent,int doDigest);
void alert_free_eventlist(EmailEvent *p);
struct EmailEvent {
  int type;          /* 'c', 'f', 'm', 't', 'w' */
  int needMod;       /* Pending moderator approval */
  Blob hdr;          /* Header content, for forum entries */
  Blob txt;          /* Text description to appear in an alert */
  char *zFromName;   /* Human name of the sender */
  EmailEvent *pNext; /* Next in chronological order */
};
#define LOCAL_INTERFACE 0
void style_table_sorter(void);
char *human_readable_age(double rAge);
typedef struct Stmt Stmt;
i64 db_column_int64(Stmt *pStmt,int N);
int db_prepare_blob(Stmt *pStmt,Blob *pSql);
void blob_append_sql(Blob *pBlob,const char *zFormat,...);
void subscriber_list_page(void);
void unsubscribe_page(void);
NORETURN void cgi_redirect(const char *zURL);
int validate16(const char *zIn,int nIn);
void alert_page(void);
const char *cgi_parameter_checked(const char *zName,int iValue);
#define PCK(x)      cgi_parameter_checked(x,1)
#define CAPTCHA 3  /* Which captcha rendering to use */
#if CAPTCHA==1
char *captcha_render(const char *zPw);
#endif
#if CAPTCHA==3
char *captcha_render(const char *zPw);
#endif
#if CAPTCHA==2
char *captcha_render(const char *zPw);
#endif
const char *captcha_decode(unsigned int seed);
unsigned int captcha_seed(void);
const char *cgi_parameter(const char *zName,const char *zDefault);
#define PD(x,y)     cgi_parameter((x),(y))
void form_begin(const char *zOtherArgs,const char *zAction,...);
void cgi_set_parameter_nocopy(const char *zName,const char *zValue,int isQP);
char *db_text(const char *zDefault,const char *zSql,...);
int db_last_insert_rowid(void);
int cgi_parameter_boolean(const char *zName);
#define PB(x)       cgi_parameter_boolean(x)
char *cgi_parameter_trimmed(const char *zName,const char *zDefault);
#define PT(x)       cgi_parameter_trimmed((x),0)
int cgi_csrf_safe(int requirePost);
NORETURN void cgi_redirectf(const char *zFormat,...);
int login_is_individual(void);
void subscribe_page(void);
void alert_append_confirmation_message(Blob *pMsg,const char *zCode);
int db_exists(const char *zSql,...);
int captcha_is_correct(int bAlwaysNeeded);
#define P(x)        cgi_parameter((x),0)
void prompt_for_user_comment(Blob *pComment,Blob *pPrompt);
#define ExtFILE    0  /* Always follow symlinks */
sqlite3_int64 blob_read_from_file(Blob *pBlob,const char *zFilename,int eFType);
int db_int(int iDflt,const char *zSql,...);
typedef struct Setting Setting;
void print_setting(const Setting *pSetting);
void db_set(const char *zName,const char *zValue,int globalFlag);
Setting *db_find_setting(const char *zName,int allowPrefix);
int db_open_config(int useAttach,int isOptional);
const Setting *setting_info(int *pnCount);
struct Setting {
  const char *name;     /* Name of the setting */
  const char *var;      /* Internal variable name used by db_set() */
  int width;            /* Width of display.  0 for boolean values. */
  int versionable;      /* Is this setting versionable? */
  int forceTextArea;    /* Force using a text area for display? */
  const char *def;      /* Default value */
};
void alert_send_alerts(u32 flags);
#define SENDALERT_STDOUT      0x0004    /* Print emails instead of sending */
#define SENDALERT_PRESERVE    0x0002    /* Do not mark the task as done */
#define SENDALERT_DIGEST      0x0001    /* Send a digest */
void prompt_user(const char *zPrompt,Blob *pIn);
#if defined(FOSSIL_ENABLE_TCL)
#include "tcl.h"
#endif
#if defined(FOSSIL_ENABLE_JSON)
#include "cson_amalgamation.h"
#include "json_detail.h"
#endif
#if defined(HAVE_BACKTRACE)
#include <execinfo.h>
#endif
const char *find_option(const char *zLong,const char *zShort,int hasArg);
int db_finalize(Stmt *pStmt);
int db_column_int(Stmt *pStmt,int N);
const char *db_column_text(Stmt *pStmt,int N);
int db_step(Stmt *pStmt);
int db_prepare(Stmt *pStmt,const char *zFormat,...);
void usage(const char *zFormat);
void verify_all_options(void);
struct Stmt {
  Blob sql;               /* The SQL for this statement */
  sqlite3_stmt *pStmt;    /* The results of sqlite3_prepare_v2() */
  Stmt *pNext, *pPrev;    /* List of all unfinalized statements */
  int nStep;              /* Number of sqlite3_step() calls */
  int rc;                 /* Error from db_vprepare() */
};
void db_find_and_open_repository(int bFlags,int nArgUsed);
void alert_cmd(void);
typedef struct SmtpSession SmtpSession;
int smtp_send_msg(SmtpSession *p,const char *zFrom,int nTo,const char **azTo,const char *zMsg);
int blob_write_to_file(Blob *pBlob,const char *zFilename);
#include <dirent.h>
char *file_time_tempname(const char *zDir,const char *zSuffix);
void blob_add_final_newline(Blob *pBlob);
char *cgi_rfc822_datestamp(time_t now);
void blob_appendf(Blob *pBlob,const char *zFormat,...);
typedef struct AlertSender AlertSender;
void alert_send(AlertSender *p,Blob *pHdr,Blob *pBody,const char *zFromName);
void email_header_to_free(int nTo,char **azTo);
void *fossil_realloc(void *p,size_t n);
void email_header_to(Blob *pMsg,int *pnTo,char ***pazTo);
void fossil_print(const char *zFormat,...);
void alert_test_mailbox_hashname(void);
char *alert_hostname(const char *zAddr);
void alert_find_emailaddr_func(sqlite3_context *context,int argc,sqlite3_value **argv);
char *alert_find_emailaddr(const char *zIn);
char *mprintf(const char *zFormat,...);
int fossil_isalnum(char c);
char *email_copy_addr(const char *z,char cTerm);
int fossil_isspace(char c);
#define blob_buffer(X)  ((X)->aData)
#define blob_size(X)  ((X)->nUsed)
int blob_trim(Blob *p);
int blob_line(Blob *pFrom,Blob *pTo);
void blob_rewind(Blob *p);
int email_header_value(Blob *pMsg,const char *zField,Blob *pValue);
int smtp_client_startup(SmtpSession *p);
SmtpSession *smtp_session_new(const char *zFrom,const char *zDest,u32 smtpFlags,...);
#define SMTP_TRACE_STDOUT   0x00001     /* Debugging info to console */
#define SMTP_DIRECT         0x00008     /* Skip the MX lookup */
void blob_init(Blob *pBlob,const char *zData,int size);
void *fossil_malloc(size_t n);
AlertSender *alert_sender_new(const char *zAltDest,u32 mFlags);
void alert_sender_free(AlertSender *p);
NORETURN void fossil_fatal(const char *zFormat,...);
char *vmprintf(const char *zFormat,va_list ap);
void fossil_free(void *p);
void blob_reset(Blob *pBlob);
void smtp_session_free(SmtpSession *pSession);
int smtp_client_quit(SmtpSession *p);
#define ALERT_TRACE            0x0002   /* Log sending process on console */
#define ALERT_IMMEDIATE_FAIL   0x0001   /* Call fossil_fatal() on any error */
struct SmtpSession {
  const char *zFrom;        /* Domain from which we are sending */
  const char *zDest;        /* Domain that will receive the email */
  char *zHostname;          /* Hostname of SMTP server for zDest */
  u32 smtpFlags;            /* Flags changing the operation */
  FILE *logFile;            /* Write session transcript to this log file */
  Blob *pTranscript;        /* Record session transcript here */
  int atEof;                /* True after connection closes */
  char *zErr;               /* Error message */
  Blob inbuf;               /* Input buffer */
};
struct AlertSender {
  sqlite3 *db;               /* Database emails are sent to */
  sqlite3_stmt *pStmt;       /* Stmt to insert into the database */
  const char *zDest;         /* How to send email. */
  const char *zDb;           /* Name of database file */
  const char *zDir;          /* Directory in which to store as email files */
  const char *zCmd;          /* Command to run for each email */
  const char *zFrom;         /* Emails come from here */
  SmtpSession *pSmtp;        /* SMTP relay connection */
  Blob out;                  /* For zDest=="blob" */
  char *zErr;                /* Error message */
  u32 mFlags;                /* Flags */
  int bImmediateFail;        /* On any error, call fossil_fatal() */
};
#define INTERFACE 0
void blob_append(Blob *pBlob,const char *aData,int nData);
void blob_append_char(Blob *pBlob,char c);
char *blob_str(Blob *p);
void db_end_transaction(int rollbackFlag);
void multiple_choice_attribute(const char *zLabel,const char *zVar,const char *zQP,const char *zDflt,int nChoice,const char *const *azChoice);
void entry_attribute(const char *zLabel,int width,const char *zVar,const char *zQParm,const char *zDflt,int disabled);
void login_insert_csrf_secret(void);
void stats_for_email(void);
void db_begin_transaction_real(const char *zStartFile,int iStartLine);
#define db_begin_transaction()    db_begin_transaction_real(__FILE__,__LINE__)
void login_needed(int anonOk);
void login_check_credentials(void);
void setup_notification(void);
void style_submenu_element(const char *zLabel,const char *zLink,...);
typedef struct Global Global;
typedef struct Th_Interp Th_Interp;
typedef struct UrlData UrlData;
struct UrlData {
  int isFile;      /* True if a "file:" url */
  int isHttps;     /* True if a "https:" url */
  int isSsh;       /* True if an "ssh:" url */
  char *name;      /* Hostname for http: or filename for file: */
  char *hostname;  /* The HOST: parameter on http headers */
  char *protocol;  /* "http" or "https" */
  int port;        /* TCP port number for http: or https: */
  int dfltPort;    /* The default port for the given protocol */
  char *path;      /* Pathname for http: */
  char *user;      /* User id for http: */
  char *passwd;    /* Password for http: */
  char *canonical; /* Canonical representation of the URL */
  char *proxyAuth; /* Proxy-Authorizer: string */
  char *fossil;    /* The fossil query parameter on ssh: */
  unsigned flags;  /* Boolean flags controlling URL processing */
  int useProxy;    /* Used to remember that a proxy is in use */
  char *proxyUrlPath;
  int proxyOrigPort; /* Tunneled port number for https through proxy */
};
typedef struct FossilUserPerms FossilUserPerms;
struct FossilUserPerms {
  char Setup;            /* s: use Setup screens on web interface */
  char Admin;            /* a: administrative permission */
  char Delete;           /* d: delete wiki or tickets */
  char Password;         /* p: change password */
  char Query;            /* q: create new reports */
  char Write;            /* i: xfer inbound. check-in */
  char Read;             /* o: xfer outbound. check-out */
  char Hyperlink;        /* h: enable the display of hyperlinks */
  char Clone;            /* g: clone */
  char RdWiki;           /* j: view wiki via web */
  char NewWiki;          /* f: create new wiki via web */
  char ApndWiki;         /* m: append to wiki via web */
  char WrWiki;           /* k: edit wiki via web */
  char ModWiki;          /* l: approve and publish wiki content (Moderator) */
  char RdTkt;            /* r: view tickets via web */
  char NewTkt;           /* n: create new tickets */
  char ApndTkt;          /* c: append to tickets via the web */
  char WrTkt;            /* w: make changes to tickets via web */
  char ModTkt;           /* q: approve and publish ticket changes (Moderator) */
  char Attach;           /* b: add attachments */
  char TktFmt;           /* t: create new ticket report formats */
  char RdAddr;           /* e: read email addresses or other private data */
  char Zip;              /* z: download zipped artifact via /zip URL */
  char Private;          /* x: can send and receive private content */
  char WrUnver;          /* y: can push unversioned content */
  char RdForum;          /* 2: Read forum posts */
  char WrForum;          /* 3: Create new forum posts */
  char WrTForum;         /* 4: Post to forums not subject to moderation */
  char ModForum;         /* 5: Moderate (approve or reject) forum posts */
  char AdminForum;       /* 6: Set or remove capability 4 on other users */
  char EmailAlert;       /* 7: Sign up for email notifications */
  char Announce;         /* A: Send announcements */
  char Debug;            /* D: show extra Fossil debugging features */
  /* These last two are included to block infinite recursion */
  char XReader;          /* u: Inherit all privileges of "reader" */
  char XDeveloper;       /* v: Inherit all privileges of "developer" */
};
#if defined(FOSSIL_ENABLE_TCL)
typedef struct TclContext TclContext;
struct TclContext {
  int argc;              /* Number of original (expanded) arguments. */
  char **argv;           /* Full copy of the original (expanded) arguments. */
  void *hLibrary;        /* The Tcl library module handle. */
  void *xFindExecutable; /* See tcl_FindExecutableProc in th_tcl.c. */
  void *xCreateInterp;   /* See tcl_CreateInterpProc in th_tcl.c. */
  void *xDeleteInterp;   /* See tcl_DeleteInterpProc in th_tcl.c. */
  void *xFinalize;       /* See tcl_FinalizeProc in th_tcl.c. */
  Tcl_Interp *interp;    /* The on-demand created Tcl interpreter. */
  int useObjProc;        /* Non-zero if an objProc can be called directly. */
  int useTip285;         /* Non-zero if TIP #285 is available. */
  char *setup;           /* The optional Tcl setup script. */
  void *xPreEval;        /* Optional, called before Tcl_Eval*(). */
  void *pPreContext;     /* Optional, provided to xPreEval(). */
  void *xPostEval;       /* Optional, called after Tcl_Eval*(). */
  void *pPostContext;    /* Optional, provided to xPostEval(). */
};
#endif
#define MX_AUX  5
struct Global {
  int argc; char **argv;  /* Command-line arguments to the program */
  char *nameOfExe;        /* Full path of executable. */
  const char *zErrlog;    /* Log errors to this file, if not NULL */
  int isConst;            /* True if the output is unchanging & cacheable */
  const char *zVfsName;   /* The VFS to use for database connections */
  sqlite3 *db;            /* The connection to the databases */
  sqlite3 *dbConfig;      /* Separate connection for global_config table */
  char *zAuxSchema;       /* Main repository aux-schema */
  int dbIgnoreErrors;     /* Ignore database errors if true */
  const char *zConfigDbName;/* Path of the config database. NULL if not open */
  sqlite3_int64 now;      /* Seconds since 1970 */
  int repositoryOpen;     /* True if the main repository database is open */
  unsigned iRepoDataVers;  /* Initial data version for repository database */
  char *zRepositoryOption; /* Most recent cached repository option value */
  char *zRepositoryName;  /* Name of the repository database file */
  char *zLocalDbName;     /* Name of the local database file */
  char *zOpenRevision;    /* Check-in version to use during database open */
  int localOpen;          /* True if the local database is open */
  char *zLocalRoot;       /* The directory holding the  local database */
  int minPrefix;          /* Number of digits needed for a distinct UUID */
  int eHashPolicy;        /* Current hash policy.  One of HPOLICY_* */
  int fSqlTrace;          /* True if --sqltrace flag is present */
  int fSqlStats;          /* True if --sqltrace or --sqlstats are present */
  int fSqlPrint;          /* True if --sqlprint flag is present */
  int fCgiTrace;          /* True if --cgitrace is enabled */
  int fQuiet;             /* True if -quiet flag is present */
  int fJail;              /* True if running with a chroot jail */
  int fHttpTrace;         /* Trace outbound HTTP requests */
  int fAnyTrace;          /* Any kind of tracing */
  char *zHttpAuth;        /* HTTP Authorization user:pass information */
  int fSystemTrace;       /* Trace calls to fossil_system(), --systemtrace */
  int fSshTrace;          /* Trace the SSH setup traffic */
  int fSshClient;         /* HTTP client flags for SSH client */
  int fNoHttpCompress;    /* Do not compress HTTP traffic (for debugging) */
  char *zSshCmd;          /* SSH command string */
  int fNoSync;            /* Do not do an autosync ever.  --nosync */
  int fIPv4;              /* Use only IPv4, not IPv6. --ipv4 */
  char *zPath;            /* Name of webpage being served */
  char *zExtra;           /* Extra path information past the webpage name */
  char *zBaseURL;         /* Full text of the URL being served */
  char *zHttpsURL;        /* zBaseURL translated to https: */
  char *zTop;             /* Parent directory of zPath */
  const char *zContentType;  /* The content type of the input HTTP request */
  int iErrPriority;       /* Priority of current error message */
  char *zErrMsg;          /* Text of an error message */
  int sslNotAvailable;    /* SSL is not available.  Do not redirect to https: */
  Blob cgiIn;             /* Input to an xfer www method */
  int cgiOutput;          /* 0: command-line 1: CGI. 2: after CGI */
  int xferPanic;          /* Write error messages in XFER protocol */
  int fullHttpReply;      /* True for full HTTP reply.  False for CGI reply */
  Th_Interp *interp;      /* The TH1 interpreter */
  char *th1Setup;         /* The TH1 post-creation setup script, if any */
  int th1Flags;           /* The TH1 integration state flags */
  FILE *httpIn;           /* Accept HTTP input from here */
  FILE *httpOut;          /* Send HTTP output here */
  int xlinkClusterOnly;   /* Set when cloning.  Only process clusters */
  int fTimeFormat;        /* 1 for UTC.  2 for localtime.  0 not yet selected */
  int *aCommitFile;       /* Array of files to be committed */
  int markPrivate;        /* All new artifacts are private if true */
  int clockSkewSeen;      /* True if clocks on client and server out of sync */
  int wikiFlags;          /* Wiki conversion flags applied to %W */
  char isHTTP;            /* True if server/CGI modes, else assume CLI. */
  char javascriptHyperlink; /* If true, set href= using script, not HTML */
  Blob httpHeader;        /* Complete text of the HTTP request header */
  UrlData url;            /* Information about current URL */
  const char *zLogin;     /* Login name.  NULL or "" if not logged in. */
  const char *zSSLIdentity;  /* Value of --ssl-identity option, filename of
                             ** SSL client identity */
  int useLocalauth;       /* No login required if from 127.0.0.1 */
  int noPswd;             /* Logged in without password (on 127.0.0.1) */
  int userUid;            /* Integer user id */
  int isHuman;            /* True if access by a human, not a spider or bot */
  int comFmtFlags;        /* Zero or more "COMMENT_PRINT_*" bit flags, should be
                          ** accessed through get_comment_format(). */

  /* Information used to populate the RCVFROM table */
  int rcvid;              /* The rcvid.  0 if not yet defined. */
  char *zIpAddr;          /* The remote IP address */
  char *zNonce;           /* The nonce used for login */

  /* permissions available to current user */
  struct FossilUserPerms perm;

  /* permissions available to current user or to "anonymous".
  ** This is the logical union of perm permissions above with
  ** the value that perm would take if g.zLogin were "anonymous". */
  struct FossilUserPerms anon;

#ifdef FOSSIL_ENABLE_TCL
  /* all Tcl related context necessary for integration */
  struct TclContext tcl;
#endif

  /* For defense against Cross-site Request Forgery attacks */
  char zCsrfToken[12];    /* Value of the anti-CSRF token */
  int okCsrf;             /* Anti-CSRF token is present and valid */

  int parseCnt[10];       /* Counts of artifacts parsed */
  FILE *fDebug;           /* Write debug information here, if the file exists */
#ifdef FOSSIL_ENABLE_TH1_HOOKS
  int fNoThHook;          /* Disable all TH1 command/webpage hooks */
#endif
  int thTrace;            /* True to enable TH1 debugging output */
  Blob thLog;             /* Text of the TH1 debugging output */

  int isHome;             /* True if rendering the "home" page */

  /* Storage for the aux() and/or option() SQL function arguments */
  int nAux;                    /* Number of distinct aux() or option() values */
  const char *azAuxName[MX_AUX]; /* Name of each aux() or option() value */
  char *azAuxParam[MX_AUX];      /* Param of each aux() or option() value */
  const char *azAuxVal[MX_AUX];  /* Value of each aux() or option() value */
  const char **azAuxOpt[MX_AUX]; /* Options of each option() value */
  int anAuxCols[MX_AUX];         /* Number of columns for option() values */
  int allowSymlinks;             /* Cached "allow-symlinks" option */
  int mainTimerId;               /* Set to fossil_timer_start() */
  int nPendingRequest;           /* # of HTTP requests in "fossil server" */
  int nRequest;                  /* Total # of HTTP request */
#ifdef FOSSIL_ENABLE_JSON
  struct FossilJsonBits {
    int isJsonMode;            /* True if running in JSON mode, else
                                  false. This changes how errors are
                                  reported. In JSON mode we try to
                                  always output JSON-form error
                                  responses and always exit() with
                                  code 0 to avoid an HTTP 500 error.
                               */
    int resultCode;            /* used for passing back specific codes
                               ** from /json callbacks. */
    int errorDetailParanoia;   /* 0=full error codes, 1=%10, 2=%100, 3=%1000 */
    cson_output_opt outOpt;    /* formatting options for JSON mode. */
    cson_value *authToken;     /* authentication token */
    const char *jsonp;         /* Name of JSONP function wrapper. */
    unsigned char dispatchDepth /* Tells JSON command dispatching
                                   which argument we are currently
                                   working on. For this purpose, arg#0
                                   is the "json" path/CLI arg.
                                */;
    struct {                   /* "garbage collector" */
      cson_value *v;
      cson_array *a;
    } gc;
    struct {                   /* JSON POST data. */
      cson_value *v;
      cson_array *a;
      int offset;              /* Tells us which PATH_INFO/CLI args
                                  part holds the "json" command, so
                                  that we can account for sub-repos
                                  and path prefixes.  This is handled
                                  differently for CLI and CGI modes.
                               */
      const char *commandStr   /*"command" request param.*/;
    } cmd;
    struct {                   /* JSON POST data. */
      cson_value *v;
      cson_object *o;
    } post;
    struct {                   /* GET/COOKIE params in JSON mode. */
      cson_value *v;
      cson_object *o;
    } param;
    struct {
      cson_value *v;
      cson_object *o;
    } reqPayload;              /* request payload object (if any) */
    cson_array *warnings;      /* response warnings */
    int timerId;               /* fetched from fossil_timer_start() */
  } json;
#endif /* FOSSIL_ENABLE_JSON */
};
extern Global g;
void alert_submenu_common(void);
void style_footer(void);
void cgi_printf(const char *zFormat,...);
void style_header(const char *zTitleFormat,...);
int alert_enabled(void);
void alert_triggers_disable(void);
int db_table_has_column(const char *zDb,const char *zTable,const char *zColumn);
void alert_triggers_enable(void);
int db_multi_exec(const char *zSql,...);
char *db_get(const char *zName,const char *zDefault);
int fossil_strcmp(const char *zA,const char *zB);
void alert_schema(int bOnlyIfEnabled);
int db_table_exists(const char *zDb,const char *zTable);
int alert_tables_exist(void);
struct ManifestFile {
  char *zName;           /* Name of a file */
  char *zUuid;           /* Artifact hash for the file */
  char *zPerm;           /* File permissions */
  char *zPrior;          /* Prior name if the name was changed */
};
