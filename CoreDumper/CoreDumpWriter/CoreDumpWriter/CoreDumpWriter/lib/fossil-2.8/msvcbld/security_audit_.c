#line 1 "..\\src\\security_audit.c"
/*
** Copyright (c) 2017 D. Richard Hipp
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the Simplified BSD License (also
** known as the "2-Clause License" or "FreeBSD License".)

** This program is distributed in the hope that it will be useful,
** but without any warranty; without even the implied warranty of
** merchantability or fitness for a particular purpose.
**
** Author contact information:
**   drh@hwaci.com
**   http://www.hwaci.com/drh/
**
*******************************************************************************
**
** This file implements various web pages use for running a security audit
** of a Fossil configuration.
*/
#include "config.h"
#include <assert.h>
#include "security_audit.h"

/*
** Return TRUE if any of the capability letters in zTest are found
** in the capability string zCap.
*/
static int hasAnyCap(const char *zCap, const char *zTest){
  while( zTest[0] ){
    if( strchr(zCap, zTest[0]) ) return 1;
    zTest++;
  }
  return 0;
}


/*
** WEBPAGE: secaudit0
**
** Run a security audit of the current Fossil setup.
** This page requires administrator access
*/
void secaudit0_page(void){
  const char *zAnonCap;      /* Capabilities of user "anonymous" and "nobody" */
  const char *zPubPages;     /* GLOB pattern for public pages */
  const char *zSelfCap;      /* Capabilities of self-registered users */
  char *z;
  int n;

  login_check_credentials();
  if( !g.perm.Admin ){
    login_needed(0);
    return;
  }
  style_header("Security Audit");
  cgi_printf("<ol>\n");

  /* Step 1:  Determine if the repository is public or private.  "Public"
  ** means that any anonymous user on the internet can access all content.
  ** "Private" repos require (non-anonymous) login to access all content,
  ** though some content may be accessible anonymously.
  */
  zAnonCap = db_text("", "SELECT fullcap(NULL)");
  zPubPages = db_get("public-pages",0);
  if( db_get_boolean("self-register",0) ){
    CapabilityString *pCap;
    pCap = capability_add(0, db_get("default-perms",""));
    capability_expand(pCap);
    zSelfCap = capability_string(pCap);
    capability_free(pCap);
  }else{
    zSelfCap = fossil_strdup("");
  }
  if( hasAnyCap(zAnonCap,"as") ){
    cgi_printf("<li><p>This repository is <big><b>Wildly INSECURE</b></big> because\n"
           "it grants administrator privileges to anonymous users.  You\n"
           "should <a href=\"takeitprivate\">take this repository private</a>\n"
           "immediately!  Or, at least remove the Setup and Admin privileges\n"
           "for users \"anonymous\" and \"login\" on the\n"
           "<a href=\"setup_ulist\">User Configuration</a> page.\n");
  }else if( hasAnyCap(zSelfCap,"as") ){
    cgi_printf("<li><p>This repository is <big><b>Wildly INSECURE</b></big> because\n"
           "it grants administrator privileges to self-registered users.  You\n"
           "should <a href=\"takeitprivate\">take this repository private</a>\n"
           "and/or disable self-registration\n"
           "immediately!  Or, at least remove the Setup and Admin privileges\n"
           "from the default permissions for new users.\n");
  }else if( hasAnyCap(zAnonCap,"y") ){
    cgi_printf("<li><p>This repository is <big><b>INSECURE</b></big> because\n"
           "it allows anonymous users to push unversioned files.\n"
           "<p>Fix this by <a href=\"takeitprivate\">taking the repository private</a>\n"
           "or by removing the \"y\" permission from users \"anonymous\" and\n"
           "\"nobody\" on the <a href=\"setup_ulist\">User Configuration</a> page.\n");
  }else if( hasAnyCap(zSelfCap,"y") ){
    cgi_printf("<li><p>This repository is <big><b>INSECURE</b></big> because\n"
           "it allows self-registered users to push unversioned files.\n"
           "<p>Fix this by <a href=\"takeitprivate\">taking the repository private</a>\n"
           "or by removing the \"y\" permission from the default permissions or\n"
           "by disabling self-registration.\n");
  }else if( hasAnyCap(zAnonCap,"goz") ){
    cgi_printf("<li><p>This repository is <big><b>PUBLIC</b></big>. All\n"
           "checked-in content can be accessed by anonymous users.\n"
           "<a href=\"takeitprivate\">Take it private</a>.<p>\n");
  }else if( hasAnyCap(zSelfCap,"goz") ){
    cgi_printf("<li><p>This repository is <big><b>PUBLIC</b></big> because all\n"
           "checked-in content can be accessed by self-registered users.\n"
           "This repostory would be private if you disabled self-registration.</p>\n");
  }else if( !hasAnyCap(zAnonCap, "jrwy234567")
         && !hasAnyCap(zSelfCap, "jrwy234567")
         && (zPubPages==0 || zPubPages[0]==0) ){
    cgi_printf("<li><p>This repository is <big><b>Completely PRIVATE</b></big>.\n"
           "A valid login and password is required to access any content.\n");
  }else{
    cgi_printf("<li><p>This repository is <big><b>Mostly PRIVATE</b></big>.\n"
           "A valid login and password is usually required, however some\n"
           "content can be accessed either anonymously or by self-registered\n"
           "users:\n"
           "<ul>\n");
    if( hasAnyCap(zAnonCap,"j") || hasAnyCap(zSelfCap,"j") ){
      cgi_printf("<li> Wiki pages\n");
    }
    if( hasAnyCap(zAnonCap,"r") || hasAnyCap(zSelfCap,"r") ){
      cgi_printf("<li> Tickets\n");
    }
    if( hasAnyCap(zAnonCap,"234567") || hasAnyCap(zSelfCap,"234567") ){
      cgi_printf("<li> Forum posts\n");
    }
    if( zPubPages && zPubPages[0] ){
      Glob *pGlob = glob_create(zPubPages);
      int i;
      cgi_printf("<li> URLs that match any of these GLOB patterns:\n"
             "<ul>\n");
      for(i=0; i<pGlob->nPattern; i++){
        cgi_printf("<li> %h\n",(pGlob->azPattern[i]));
      }
      cgi_printf("</ul>\n");
    }
    cgi_printf("</ul>\n");
    if( zPubPages && zPubPages[0] ){
      cgi_printf("<p>Change GLOB patterns exceptions using the \"Public pages\" setting\n"
             "on the <a href=\"setup_access\">Access Settings</a> page.</p>\n");
    }
  }

  /* Make sure the HTTPS is required for login, at least, so that the
  ** password does not go across the Internet in the clear.
  */
  if( db_get_int("redirect-to-https",0)==0 ){
    cgi_printf("<li><p><b>WARNING:</b>\n"
           "Sensitive material such as login passwords can be sent over an\n"
           "unencrypted connection.\n"
           "<p>Fix this by changing the \"Redirect to HTTPS\" setting on the\n"
           "<a href=\"setup_access\">Access Control</a> page. If you were using\n"
           "the old \"Redirect to HTTPS on Login Page\" setting, switch to the\n"
           "new setting: it has a more secure implementation.\n");
  }

  /* Anonymous users should not be able to harvest email addresses
  ** from tickets.
  */
  if( hasAnyCap(zAnonCap, "e") ){
    cgi_printf("<li><p><b>WARNING:</b>\n"
           "Anonymous users can view email addresses and other personally\n"
           "identifiable information on tickets.\n"
           "<p>Fix this by removing the \"Email\" privilege\n"
           "(<a href=\"setup_ucap_list\">capability \"e\"</a>) from users\n"
           "\"anonymous\" and \"nobody\" on the\n"
           "<a href=\"setup_ulist\">User Configuration</a> page.\n");
  }

  /* Anonymous users probably should not be allowed to push content
  ** to the repository.
  */
  if( hasAnyCap(zAnonCap, "i") ){
    cgi_printf("<li><p><b>WARNING:</b>\n"
           "Anonymous users can push new check-ins into the repository.\n"
           "<p>Fix this by removing the \"Check-in\" privilege\n"
           "(<a href=\"setup_ucap_list\">capability</a> \"i\") from users\n"
           "\"anonymous\" and \"nobody\" on the\n"
           "<a href=\"setup_ulist\">User Configuration</a> page.\n");
  }

  /* Anonymous users probably should not be allowed act as moderators
  ** for wiki or tickets.
  */
  if( hasAnyCap(zAnonCap, "lq5") ){
    cgi_printf("<li><p><b>WARNING:</b>\n"
           "Anonymous users can act as moderators for wiki, tickets, or \n"
           "forum posts. This defeats the whole purpose of moderation.\n"
           "<p>Fix this by removing the \"Mod-Wiki\", \"Mod-Tkt\", and \"Mod-Forum\"\n"
           "privileges (<a href=\"%R/setup_ucap_list\">capabilities</a> \"fq5\")\n"
           "from users \"anonymous\" and \"nobody\"\n"
           "on the <a href=\"setup_ulist\">User Configuration</a> page.\n");
  }

  /* Anonymous users probably should not be allowed to delete
  ** wiki or tickets.
  */
  if( hasAnyCap(zAnonCap, "d") ){
    cgi_printf("<li><p><b>WARNING:</b>\n"
           "Anonymous users can delete wiki and tickets.\n"
           "<p>Fix this by removing the \"Delete\"\n"
           "privilege from users \"anonymous\" and \"nobody\" on the\n"
           "<a href=\"setup_ulist\">User Configuration</a> page.\n");
  }

  /* If anonymous users are allowed to create new Wiki, then
  ** wiki moderation should be activated to pervent spam.
  */
  if( hasAnyCap(zAnonCap, "fk") ){
    if( db_get_boolean("modreq-wiki",0)==0 ){
      cgi_printf("<li><p><b>WARNING:</b>\n"
             "Anonymous users can create or edit wiki without moderation.\n"
             "This can result in robots inserting lots of wiki spam into\n"
             "repository.\n"
             "Fix this by removing the \"New-Wiki\" and \"Write-Wiki\"\n"
             "privileges from users \"anonymous\" and \"nobody\" on the\n"
             "<a href=\"setup_ulist\">User Configuration</a> page or\n"
             "by enabling wiki moderation on the\n"
             "<a href=\"setup_modreq\">Moderation Setup</a> page.\n");
    }else{
      cgi_printf("<li><p>\n"
             "Anonymous users can create or edit wiki, but moderator\n"
             "approval is required before the edits become permanent.\n");
    }
  }

  /* Anonymous users should not be able to create trusted forum
  ** posts.
  */
  if( hasAnyCap(zAnonCap, "456") ){
    cgi_printf("<li><p><b>WARNING:</b>\n"
           "Anonymous users can create forum posts that are\n"
           "accepted into the permanent record without moderation.\n"
           "This can result in robots generating spam on forum posts.\n"
           "Fix this by removing the \"WriteTrusted-Forum\" privilege\n"
           "(<a href=\"setup_ucap_list\">capabilities</a> \"456\") from\n"
           "users \"anonymous\" and \"nobody\" on the\n"
           "<a href=\"setup_ulist\">User Configuration</a> page or\n");
  }

  /* Anonymous users should not be able to send announcements.
  */
  if( hasAnyCap(zAnonCap, "A") ){
    cgi_printf("<li><p><b>WARNING:</b>\n"
           "Anonymous users can send announcements to anybody who is signed\n"
           "up to receive announcements.  This can result in spam.\n"
           "Fix this by removing the \"Announce\" privilege\n"
           "(<a href=\"setup_ucap_list\">capability</a> \"A\") from\n"
           "users \"anonymous\" and \"nobody\" on the\n"
           "<a href=\"setup_ulist\">User Configuration</a> page or\n");
  }

  /* Administrative privilege should only be provided to
  ** specific individuals, not to entire classes of people.
  ** And not too many people should have administrator privilege.
  */
  z = db_text(0,
    "SELECT group_concat("
                 "printf('<a href=''setup_uedit?id=%%d''>%%s</a>',uid,login),"
             "' and ')"
    " FROM user"
    " WHERE cap GLOB '*[as]*'"
    "   AND login in ('anonymous','nobody','reader','developer')"
  );
  if( z && z[0] ){
    cgi_printf("<li><p><b>WARNING:</b>\n"
           "Administrative privilege ('a' or 's')\n"
           "is granted to an entire class of users: %s.\n"
           "Administrative privilege should only be\n"
           "granted to specific individuals.\n",(z));
  }
  n = db_int(0,"SELECT count(*) FROM user WHERE fullcap(cap) GLOB '*[as]*'");
  if( n==0 ){
    cgi_printf("<li><p>\n"
           "No users have administrator privilege.\n");
  }else{
    z = db_text(0,
      "SELECT group_concat("
                 "printf('<a href=''setup_uedit?id=%%d''>%%s</a>',uid,login),"
             "', ')"
      " FROM user"
      " WHERE fullcap(cap) GLOB '*[as]*'"
    );
    cgi_printf("<li><p>\n"
           "Users with administrator privilege are: %s\n",(z));
    fossil_free(z);
    if( n>3 ){
      cgi_printf("<li><p><b>WARNING:</b>\n"
             "Administrator privilege is granted to\n"
             "<a href='setup_ulist?with=as'>%d users</a>.\n"
             "Ideally, administator privilege ('s' or 'a') should only\n"
             "be granted to one or two users.\n",(n));
    }
  }

  /* The push-unversioned privilege should only be provided to
  ** specific individuals, not to entire classes of people.
  ** And no too many people should have this privilege.
  */
  z = db_text(0,
    "SELECT group_concat("
                 "printf('<a href=''setup_uedit?id=%%d''>%%s</a>',uid,login),"
             "' and ')"
    " FROM user"
    " WHERE cap GLOB '*y*'"
    "   AND login in ('anonymous','nobody','reader','developer')"
  );
  if( z && z[0] ){
    cgi_printf("<li><p><b>WARNING:</b>\n"
           "The \"Write-Unver\" privilege is granted to an entire class of users: %s.\n"
           "The Write-Unver privilege should only be granted to specific individuals.\n",(z));
    fossil_free(z);
  }
  n = db_int(0,"SELECT count(*) FROM user WHERE cap GLOB '*y*'");
  if( n>0 ){
    z = db_text(0,
       "SELECT group_concat("
          "printf('<a href=''setup_uedit?id=%%d''>%%s</a>',uid,login),', ')"
       " FROM user WHERE fullcap(cap) GLOB '*y*'"
    );
    cgi_printf("<li><p>\n"
           "Users with \"Write-Unver\" privilege: %s\n",(z));
    fossil_free(z);
    if( n>3 ){
      cgi_printf("<p><b>Caution:</b>\n"
             "The \"Write-Unver\" privilege ('y') is granted to an excessive\n"
             "number of users (%d).\n"
             "Ideally, the Write-Unver privilege should only\n"
             "be granted to one or two users.\n",(n));
    }
  }

  /* Notify if REMOTE_USER or HTTP_AUTHENTICATION is used for login.
  */
  if( db_get_boolean("remote_user_ok", 0) ){
    cgi_printf("<li><p>\n"
           "This repository trusts that the REMOTE_USER environment variable set\n"
           "up by the webserver contains the name of an authenticated user.\n"
           "Fossil's built-in authentication mechanism is bypassed.\n"
           "<p>Fix this by deactivating the \"Allow REMOTE_USER authentication\"\n"
           "checkbox on the <a href=\"setup_access\">Access Control</a> page.\n");
  }
  if( db_get_boolean("http_authentication_ok", 0) ){
    cgi_printf("<li><p>\n"
           "This repository trusts that the HTTP_AUTHENITICATION environment\n"
           "variable set up by the webserver contains the name of an\n"
           "authenticated user.\n"
           "Fossil's built-in authentication mechanism is bypassed.\n"
           "<p>Fix this by deactivating the \"Allow HTTP_AUTHENTICATION authentication\"\n"
           "checkbox on the <a href=\"setup_access\">Access Control</a> page.\n");
  }

  /* Logging should be turned on
  */
  if( db_get_boolean("access-log",0)==0 ){
    cgi_printf("<li><p>\n"
           "The <a href=\"access_log\">User Log</a> is disabled.  The user log\n"
           "keeps a record of successful and unsucessful login attempts and is\n"
           "useful for security monitoring.\n");
  }
  if( db_get_boolean("admin-log",0)==0 ){
    cgi_printf("<li><p>\n"
           "The <a href=\"admin_log\">Administrative Log</a> is disabled.\n"
           "The administrative log provides a record of configuration changes\n"
           "and is useful for security monitoring.\n");
  }

#if !defined(_WIN32) && !defined(FOSSIL_OMIT_LOAD_AVERAGE)
  /* Make sure that the load-average limiter is armed and working */
  if( load_average()==0.0 ){
    cgi_printf("<li><p>\n"
           "Unable to get the system load average.  This can prevent Fossil\n"
           "from throttling expensive operations during peak demand.\n"
           "<p>If running in a chroot jail on Linux, verify that the /proc\n"
           "filesystem is mounted within the jail, so that the load average\n"
           "can be obtained from the /proc/loadavg file.\n");
  }else {
    double r = atof(db_get("max-loadavg", "0"));
    if( r<=0.0 ){
      cgi_printf("<li><p>\n"
             "Load average limiting is turned off.  This can cause the server\n"
             "to bog down if many requests for expensive services (such as\n"
             "large diffs or tarballs) arrive at about the same time.\n"
             "<p>To fix this, set the \"Server Load Average Limit\" on the\n"
             "<a href=\"setup_access\">Access Control</a> page to approximately\n"
             "the number of available cores on your server, or maybe just a little\n"
             "less.\n");
    }else if( r>=8.0 ){
      cgi_printf("<li><p>\n"
             "The \"Server Load Average Limit\" on the\n"
             "<a href=\"setup_access\">Access Control</a> page is set to %g,\n"
             "which seems high.  Is this server really a %d-core machine?\n",(r),((int)r));
    }
  }
#endif

  if( g.zErrlog==0 || fossil_strcmp(g.zErrlog,"-")==0 ){
    cgi_printf("<li><p>\n"
           "The server error log is disabled.\n"
           "To set up an error log:\n"
           "<ul>\n"
           "<li>If running from CGI, make an entry \"errorlog: <i>FILENAME</i>\"\n"
           "in the CGI script.\n"
           "<li>If running the \"fossil server\" or \"fossil http\" commands,\n"
           "add the \"--errorlog <i>FILENAME</i>\" command-line option.\n"
           "</ul>\n");
  }else{
    FILE *pTest = fossil_fopen(g.zErrlog,"a");
    if( pTest==0 ){
      cgi_printf("<li><p>\n"
             "<b>Error:</b>\n"
             "There is an error log at \"%h\" but that file is not\n"
             "writable and so no logging will occur.\n",(g.zErrlog));
    }else{
      fclose(pTest);
      cgi_printf("<li><p>\n"
             "The error log at \"<a href='%R/errorlog'>%h</a>\" that is\n"
             "%,lld bytes in size.\n",(g.zErrlog),(file_size(g.zErrlog, ExtFILE)));
    }
  }

  cgi_printf("<li><p> User capability summary:\n");
  capability_summary();

  if( alert_enabled() ){
    cgi_printf("<li><p> Email alert configuration summary:\n"
           "<table class=\"label-value\">\n");
    stats_for_email();
    cgi_printf("</table>\n");
  }else{
    cgi_printf("<li><p> Email alerts are disabled\n");
  }

  cgi_printf("</ol>\n");
  style_footer();
}

/*
** WEBPAGE: takeitprivate
**
** Disable anonymous access to this website
*/
void takeitprivate_page(void){
  login_check_credentials();
  if( !g.perm.Admin ){
    login_needed(0);
    return;
  }
  if( P("cancel") ){
    /* User pressed the cancel button.  Go back */
    cgi_redirect("secaudit0");
  }
  if( P("apply") ){
    db_multi_exec(
      "UPDATE user SET cap=''"
      " WHERE login IN ('nobody','anonymous');"
      "DELETE FROM config WHERE name='public-pages';"
    );
    db_set("self-register","0",0);
    cgi_redirect("secaudit0");
  }
  style_header("Make This Website Private");
  cgi_printf("<p>Click the \"Make It Private\" button below to disable all\n"
         "anonymous access to this repository.  A valid login and password\n"
         "will be required to access this repository after clicking that\n"
         "button.</p>\n"
         "\n"
         "<p>Click the \"Cancel\" button to leave things as they are.</p>\n"
         "\n"
         "<form action=\"%s\" method=\"post\">\n"
         "<input type=\"submit\" name=\"apply\" value=\"Make It Private\">\n"
         "<input type=\"submit\" name=\"cancel\" value=\"Cancel\">\n"
         "</form>\n",(g.zPath));

  style_footer();
}

/*
** The maximum number of bytes of log to show
*/
#define MXSHOWLOG 50000

/*
** WEBPAGE: errorlog
**
** Show the content of the error log.  Only the administrator can view
** this page.
*/
void errorlog_page(void){
  i64 szFile;
  FILE *in;
  char z[10000];
  login_check_credentials();
  if( !g.perm.Admin ){
    login_needed(0);
    return;
  }
  style_header("Server Error Log");
  style_submenu_element("Test", "%R/test-warning");
  style_submenu_element("Refresh", "%R/errorlog");
  if( g.zErrlog==0 || fossil_strcmp(g.zErrlog,"-")==0 ){
    cgi_printf("<p>To create a server error log:\n"
           "<ol>\n"
           "<li><p>\n"
           "If the server is running as CGI, then create a line in the CGI file\n"
           "like this:\n"
           "<blockquote><pre>\n"
           "errorlog: <i>FILENAME</i>\n"
           "</pre></blockquote>\n"
           "<li><p>\n"
           "If the server is running using one of \n"
           "the \"fossil http\" or \"fossil server\" commands then add\n"
           "a command-line option \"--errorlog <i>FILENAME</i>\" to that\n"
           "command.\n"
           "</ol>\n");
    style_footer();
    return;
  }
  if( P("truncate1") && cgi_csrf_safe(1) ){
    fclose(fopen(g.zErrlog,"w"));
  }
  if( P("download") ){
    Blob log;
    blob_read_from_file(&log, g.zErrlog, ExtFILE);
    cgi_set_content_type("text/plain");
    cgi_set_content(&log);
    return;
  }
  szFile = file_size(g.zErrlog, ExtFILE);
  if( P("truncate") ){
    cgi_printf("<form action=\"%R/errorlog\" method=\"POST\">\n"
           "<p>Confirm that you want to truncate the %,lld-byte error log:\n"
           "<input type=\"submit\" name=\"truncate1\" value=\"Confirm\">\n"
           "<input type=\"submit\" name=\"cancel\" value=\"Cancel\">\n"
           "</form>\n",(szFile));
    style_footer();
    return;
  }
  cgi_printf("<p>The server error log at \"%h\" is %,lld bytes in size.\n",(g.zErrlog),(szFile));
  style_submenu_element("Download", "%R/errorlog?download");
  style_submenu_element("Truncate", "%R/errorlog?truncate");
  in = fossil_fopen(g.zErrlog, "rb");
  if( in==0 ){
    cgi_printf("<p class='generalError'>Unable top open that file for reading!</p>\n");
    style_footer();
    return;
  }
  if( szFile>MXSHOWLOG && P("all")==0 ){
    cgi_printf("<form action=\"%R/errorlog\" method=\"POST\">\n"
           "<p>Only the last %,d bytes are shown.\n"
           "<input type=\"submit\" name=\"all\" value=\"Show All\">\n"
           "</form>\n",(MXSHOWLOG));
    fseek(in, -MXSHOWLOG, SEEK_END);
  }
  cgi_printf("<hr>\n"
         "<pre>\n");
  while( fgets(z, sizeof(z), in) ){
    cgi_printf("%h",(z));
  }
  fclose(in);
  cgi_printf("</pre>\n");
  style_footer();
}
