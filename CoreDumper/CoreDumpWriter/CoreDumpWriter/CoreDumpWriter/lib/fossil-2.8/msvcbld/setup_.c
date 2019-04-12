#line 1 "..\\src\\setup.c"
/*
** Copyright (c) 2007 D. Richard Hipp
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the Simplified BSD License (also
** known as the "2-Clause License" or "FreeBSD License".)
**
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
** Implementation of the Setup page
*/
#include "config.h"
#include <assert.h>
#include "setup.h"

/*
** Increment the "cfgcnt" variable, so that ETags will know that
** the configuration has changed.
*/
void setup_incr_cfgcnt(void){
  static int once = 1;
  if( once ){
    once = 0;
    db_multi_exec("UPDATE config SET value=value+1 WHERE name='cfgcnt'");
    if( db_changes()==0 ){
      db_multi_exec("INSERT INTO config(name,value) VALUES('cfgcnt',1)");
    }
  }
}

/*
** Output a single entry for a menu generated using an HTML table.
** If zLink is not NULL or an empty string, then it is the page that
** the menu entry will hyperlink to.  If zLink is NULL or "", then
** the menu entry has no hyperlink - it is disabled.
*/
void setup_menu_entry(
  const char *zTitle,
  const char *zLink,
  const char *zDesc
){
  cgi_printf("<tr><td valign=\"top\" align=\"right\">\n");
  if( zLink && zLink[0] ){
    cgi_printf("<a href=\"%s\">%h</a>\n",(zLink),(zTitle));
  }else{
    cgi_printf("%h\n",(zTitle));
  }
  cgi_printf("</td><td width=\"5\"></td><td valign=\"top\">%h</td></tr>\n",(zDesc));
}



/*
** WEBPAGE: setup
**
** Main menu for the administrative pages.  Requires Admin or Setup
** privileges.  Links to sub-pages only usable by Setup users are
** shown only to Setup users.
*/
void setup_page(void){
  int setup_user = 0;
  login_check_credentials();
  if( !g.perm.Admin ){
    login_needed(0);
  }
  setup_user = g.perm.Setup;

  style_header("Server Administration");

  /* Make sure the header contains <base href="...">.   Issue a warning
  ** if it does not. */
  if( !cgi_header_contains("<base href=") ){
    cgi_printf("<p class=\"generalError\"><b>Configuration Error:</b> Please add\n"
           "<tt>&lt;base href=\"$secureurl/$current_page\"&gt;</tt> after\n"
           "<tt>&lt;head&gt;</tt> in the\n"
           "<a href=\"setup_skinedit?w=2\">HTML header</a>!</p>\n");
  }

#if !defined(_WIN32)
  /* Check for /dev/null and /dev/urandom.  We want both devices to be present,
  ** but they are sometimes omitted (by mistake) from chroot jails. */
  if( access("/dev/null", R_OK|W_OK) ){
    cgi_printf("<p class=\"generalError\">WARNING: Device \"/dev/null\" is not available\n"
           "for reading and writing.</p>\n");
  }
  if( access("/dev/urandom", R_OK) ){
    cgi_printf("<p class=\"generalError\">WARNING: Device \"/dev/urandom\" is not available\n"
           "for reading. This means that the pseudo-random number generator used\n"
           "by SQLite will be poorly seeded.</p>\n");
  }
#endif

  cgi_printf("<table border=\"0\" cellspacing=\"3\">\n");
  setup_menu_entry("Users", "setup_ulist",
    "Grant privileges to individual users.");
  if( setup_user ){
    setup_menu_entry("Access", "setup_access",
      "Control access settings.");
    setup_menu_entry("Configuration", "setup_config",
      "Configure the WWW components of the repository");
  }
  setup_menu_entry("Security-Audit", "secaudit0",
    "Analyze the current configuration for security problems");
  if( setup_user ){
    setup_menu_entry("Settings", "setup_settings",
      "Web interface to the \"fossil settings\" command");
  }
  setup_menu_entry("Timeline", "setup_timeline",
    "Timeline display preferences");
  if( setup_user ){
    setup_menu_entry("Login-Group", "setup_login_group",
      "Manage single sign-on between this repository and others"
      " on the same server");
    setup_menu_entry("Tickets", "tktsetup",
      "Configure the trouble-ticketing system for this repository");
    setup_menu_entry("Wiki", "setup_wiki",
      "Configure the wiki for this repository");
  }
  setup_menu_entry("Search","srchsetup",
    "Configure the built-in search engine");
  setup_menu_entry("URL Aliases", "waliassetup",
    "Configure URL aliases");
  if( setup_user ){
    setup_menu_entry("Notification", "setup_notification",
      "Automatic notifications of changes via outbound email");
    setup_menu_entry("Email-Server", "setup_smtp",
      "Activate and configure the built-in email server");
    setup_menu_entry("Transfers", "xfersetup",
      "Configure the transfer system for this repository");
  }
  setup_menu_entry("Skins", "setup_skin",
    "Select and/or modify the web interface \"skins\"");
  setup_menu_entry("Moderation", "setup_modreq",
    "Enable/Disable requiring moderator approval of Wiki and/or Ticket"
    " changes and attachments.");
  setup_menu_entry("Ad-Unit", "setup_adunit",
    "Edit HTML text for an ad unit inserted after the menu bar");
  setup_menu_entry("URLs & Checkouts", "urllist",
    "Show URLs used to access this repo and known check-outs");
  if( setup_user ){
    setup_menu_entry("Web-Cache", "cachestat",
      "View the status of the expensive-page cache");
  }
  setup_menu_entry("Logo", "setup_logo",
    "Change the logo and background images for the server");
  setup_menu_entry("Shunned", "shun",
    "Show artifacts that are shunned by this repository");
  setup_menu_entry("Artifact Receipts Log", "rcvfromlist",
    "A record of received artifacts and their sources");
  setup_menu_entry("User Log", "access_log",
    "A record of login attempts");
  setup_menu_entry("Administrative Log", "admin_log",
    "View the admin_log entries");
  setup_menu_entry("Error Log", "errorlog",
    "View the Fossil server error log");
  setup_menu_entry("Unversioned Files", "uvlist?byage=1",
    "Show all unversioned files held");
  setup_menu_entry("Stats", "stat",
    "Repository Status Reports");
  setup_menu_entry("Sitemap", "sitemap",
    "Links to miscellaneous pages");
  if( setup_user ){
    setup_menu_entry("SQL", "admin_sql",
      "Enter raw SQL commands");
    setup_menu_entry("TH1", "admin_th1",
      "Enter raw TH1 commands");
  }
  cgi_printf("</table>\n");

  style_footer();
}

/*
** Generate a checkbox for an attribute.
*/
void onoff_attribute(
  const char *zLabel,   /* The text label on the checkbox */
  const char *zVar,     /* The corresponding row in the VAR table */
  const char *zQParm,   /* The query parameter */
  int dfltVal,          /* Default value if VAR table entry does not exist */
  int disabled          /* 1 if disabled */
){
  const char *zQ = P(zQParm);
  int iVal = db_get_boolean(zVar, dfltVal);
  if( zQ==0 && !disabled && P("submit") ){
    zQ = "off";
  }
  if( zQ ){
    int iQ = fossil_strcmp(zQ,"on")==0 || atoi(zQ);
    if( iQ!=iVal ){
      login_verify_csrf_secret();
      db_set(zVar, iQ ? "1" : "0", 0);
      admin_log("Set option [%q] to [%q].",
                zVar, iQ ? "on" : "off");
      iVal = iQ;
    }
  }
  cgi_printf("<label><input type=\"checkbox\" name=\"%s\"\n",(zQParm));
  if( iVal ){
    cgi_printf("checked=\"checked\"\n");
  }
  if( disabled ){
    cgi_printf("disabled=\"disabled\"\n");
  }
  cgi_printf("/> <b>%s</b></label>\n",(zLabel));
}

/*
** Generate an entry box for an attribute.
*/
void entry_attribute(
  const char *zLabel,   /* The text label on the entry box */
  int width,            /* Width of the entry box */
  const char *zVar,     /* The corresponding row in the VAR table */
  const char *zQParm,   /* The query parameter */
  const char *zDflt,    /* Default value if VAR table entry does not exist */
  int disabled          /* 1 if disabled */
){
  const char *zVal = db_get(zVar, zDflt);
  const char *zQ = P(zQParm);
  if( zQ && fossil_strcmp(zQ,zVal)!=0 ){
    const int nZQ = (int)strlen(zQ);
    login_verify_csrf_secret();
    db_set(zVar, zQ, 0);
    admin_log("Set entry_attribute %Q to: %.*s%s",
              zVar, 20, zQ, (nZQ>20 ? "..." : ""));
    zVal = zQ;
  }
  cgi_printf("<input type=\"text\" id=\"%s\" name=\"%s\" value=\"%h\" "
         "size=\"%d\" ",(zQParm),(zQParm),(zVal),(width));
  if( disabled ){
    cgi_printf("disabled=\"disabled\" ");
  }
  cgi_printf("/> <b>%s</b>\n",(zLabel));
}

/*
** Generate a text box for an attribute.
*/
const char *textarea_attribute(
  const char *zLabel,   /* The text label on the textarea */
  int rows,             /* Rows in the textarea */
  int cols,             /* Columns in the textarea */
  const char *zVar,     /* The corresponding row in the VAR table */
  const char *zQP,      /* The query parameter */
  const char *zDflt,    /* Default value if VAR table entry does not exist */
  int disabled          /* 1 if the textarea should  not be editable */
){
  const char *z = db_get(zVar, zDflt);
  const char *zQ = P(zQP);
  if( zQ && !disabled && fossil_strcmp(zQ,z)!=0){
    const int nZQ = (int)strlen(zQ);
    login_verify_csrf_secret();
    db_set(zVar, zQ, 0);
    admin_log("Set textarea_attribute %Q to: %.*s%s",
              zVar, 20, zQ, (nZQ>20 ? "..." : ""));
    z = zQ;
  }
  if( rows>0 && cols>0 ){
    cgi_printf("<textarea id=\"id%s\" name=\"%s\" rows=\"%d\"\n",(zQP),(zQP),(rows));
    if( disabled ){
      cgi_printf("disabled=\"disabled\"\n");
    }
    cgi_printf("cols=\"%d\">%h</textarea>\n",(cols),(z));
    if( zLabel && *zLabel ){
      cgi_printf("<span class=\"textareaLabel\">%s</span>\n",(zLabel));
    }
  }
  return z;
}

/*
** Generate a text box for an attribute.
*/
void multiple_choice_attribute(
  const char *zLabel,   /* The text label on the menu */
  const char *zVar,     /* The corresponding row in the VAR table */
  const char *zQP,      /* The query parameter */
  const char *zDflt,    /* Default value if VAR table entry does not exist */
  int nChoice,          /* Number of choices */
  const char *const *azChoice /* Choices in pairs (VAR value, Display) */
){
  const char *z = db_get(zVar, zDflt);
  const char *zQ = P(zQP);
  int i;
  if( zQ && fossil_strcmp(zQ,z)!=0){
    const int nZQ = (int)strlen(zQ);
    login_verify_csrf_secret();
    db_set(zVar, zQ, 0);
    admin_log("Set multiple_choice_attribute %Q to: %.*s%s",
              zVar, 20, zQ, (nZQ>20 ? "..." : ""));
    z = zQ;
  }
  cgi_printf("<select size=\"1\" name=\"%s\" id=\"id%s\">\n",(zQP),(zQP));
  for(i=0; i<nChoice*2; i+=2){
    const char *zSel = fossil_strcmp(azChoice[i],z)==0 ? " selected" : "";
    cgi_printf("<option value=\"%h\"%s>%h</option>\n",(azChoice[i]),(zSel),(azChoice[i+1]));
  }
  cgi_printf("</select> <b>%h</b>\n",(zLabel));
}


/*
** WEBPAGE: setup_access
**
** The access-control settings page.  Requires Setup privileges.
*/
void setup_access(void){
  static const char * const azRedirectOpts[] = {
    "0", "Off",
    "1", "Login Page Only",
    "2", "All Pages"
  };
  login_check_credentials();
  if( !g.perm.Setup ){
    login_needed(0);
    return;
  }

  style_header("Access Control Settings");
  db_begin_transaction();
  cgi_printf("<form action=\"%s/setup_access\" method=\"post\"><div>\n",(g.zTop));
  login_insert_csrf_secret();
  cgi_printf("<input type=\"submit\"  name=\"submit\" value=\"Apply Changes\" /></p>\n"
         "<hr />\n");
  multiple_choice_attribute("Redirect to HTTPS",
     "redirect-to-https", "redirhttps", "0",
     count(azRedirectOpts)/2, azRedirectOpts);
  cgi_printf("<p>Force the use of HTTPS by redirecting to HTTPS when an \n"
         "unencrypted request is received.  This feature can be enabled\n"
         "for the Login page only, or for all pages.\n"
         "<p>Further details:  When enabled, this option causes the $secureurl TH1\n"
         "variable is set to an \"https:\" variant of $baseurl.  Otherwise,\n"
         "$secureurl is just an alias for $baseurl.\n"
         "(Property: \"redirect-to-https\".  \"0\" for off, \"1\" for Login page only,\n"
         "\"2\" otherwise.)\n"
         "<hr />\n");
  onoff_attribute("Require password for local access",
     "localauth", "localauth", 0, 0);
  cgi_printf("<p>When enabled, the password sign-in is always required for\n"
         "web access.  When disabled, unrestricted web access from 127.0.0.1\n"
         "is allowed for the <a href=\"%R/help/ui\">fossil ui</a> command or\n"
         "from the <a href=\"%R/help/server\">fossil server</a>,\n"
         "<a href=\"%R/help/http\">fossil http</a> commands when the\n"
         "\"--localauth\" command line options is used, or from the\n"
         "<a href=\"%R/help/cgi\">fossil cgi</a> if a line containing\n"
         "the word \"localauth\" appears in the CGI script.\n"
         "\n"
         "<p>A password is always required if any one or more\n"
         "of the following are true:\n"
         "<ol>\n"
         "<li> This button is checked\n"
         "<li> The inbound TCP/IP connection is not from 127.0.0.1\n"
         "<li> The server is started using either of the\n"
         "<a href=\"%R/help/server\">fossil server</a> or\n"
         "<a href=\"%R/help/server\">fossil http</a> commands\n"
         "without the \"--localauth\" option.\n"
         "<li> The server is started from CGI without the \"localauth\" keyword\n"
         "in the CGI script.\n"
         "</ol>\n"
         "(Property: \"localauth\")\n"
         "\n"
         "<hr />\n");
  onoff_attribute("Enable /test_env",
     "test_env_enable", "test_env_enable", 0, 0);
  cgi_printf("<p>When enabled, the %h/test_env URL is available to all\n"
         "users.  When disabled (the default) only users Admin and Setup can visit\n"
         "the /test_env page.\n"
         "(Property: \"test_env_enable\")\n"
         "</p>\n"
         "\n"
         "<hr />\n",(g.zBaseURL));
  onoff_attribute("Allow REMOTE_USER authentication",
     "remote_user_ok", "remote_user_ok", 0, 0);
  cgi_printf("<p>When enabled, if the REMOTE_USER environment variable is set to the\n"
         "login name of a valid user and no other login credentials are available,\n"
         "then the REMOTE_USER is accepted as an authenticated user.\n"
         "(Property: \"remote_user_ok\")\n"
         "</p>\n"
         "\n"
         "<hr />\n");
  onoff_attribute("Allow HTTP_AUTHENTICATION authentication",
     "http_authentication_ok", "http_authentication_ok", 0, 0);
  cgi_printf("<p>When enabled, allow the use of the HTTP_AUTHENTICATION environment\n"
         "variable or the \"Authentication:\" HTTP header to find the username and\n"
         "password. This is another way of supporting Basic Authenitication.\n"
         "(Property: \"http_authentication_ok\")\n"
         "</p>\n"
         "\n"
         "<hr />\n");
  entry_attribute("IP address terms used in login cookie", 3,
                  "ip-prefix-terms", "ipt", "2", 0);
  cgi_printf("<p>The number of octets of of the IP address used in the login cookie.\n"
         "Set to zero to omit the IP address from the login cookie.  A value of\n"
         "2 is recommended.\n"
         "(Property: \"ip-prefix-terms\")\n"
         "</p>\n"
         "\n"
         "<hr />\n");
  entry_attribute("Login expiration time", 6, "cookie-expire", "cex",
                  "8766", 0);
  cgi_printf("<p>The number of hours for which a login is valid.  This must be a\n"
         "positive number.  The default is 8766 hours which is approximately equal\n"
         "to a year.\n"
         "(Property: \"cookie-expire\")</p>\n");

  cgi_printf("<hr />\n");
  entry_attribute("Download packet limit", 10, "max-download", "mxdwn",
                  "5000000", 0);
  cgi_printf("<p>Fossil tries to limit out-bound sync, clone, and pull packets\n"
         "to this many bytes, uncompressed.  If the client requires more data\n"
         "than this, then the client will issue multiple HTTP requests.\n"
         "Values below 1 million are not recommended.  5 million is a\n"
         "reasonable number.  (Property: \"max-download\")</p>\n");

  cgi_printf("<hr />\n");
  entry_attribute("Download time limit", 11, "max-download-time", "mxdwnt",
                  "30", 0);

  cgi_printf("<p>Fossil tries to spend less than this many seconds gathering\n"
         "the out-bound data of sync, clone, and pull packets.\n"
         "If the client request takes longer, a partial reply is given similar\n"
         "to the download packet limit. 30s is a reasonable default.\n"
         "(Property: \"max-download-time\")</p>\n");

  cgi_printf("<hr />\n");
  entry_attribute("Server Load Average Limit", 11, "max-loadavg", "mxldavg",
                  "0.0", 0);
  cgi_printf("<p>Some expensive operations (such as computing tarballs, zip archives,\n"
         "or annotation/blame pages) are prohibited if the load average on the host\n"
         "computer is too large.  Set the threshold for disallowing expensive\n"
         "computations here.  Set this to 0.0 to disable the load average limit.\n"
         "This limit is only enforced on Unix servers.  On Linux systems,\n"
         "access to the /proc virtual filesystem is required, which means this limit\n"
         "might not work inside a chroot() jail.\n"
         "(Property: \"max-loadavg\")</p>\n");

  cgi_printf("<hr />\n");
  onoff_attribute(
      "Enable hyperlinks for \"nobody\" based on User-Agent and Javascript",
      "auto-hyperlink", "autohyperlink", 1, 0);
  cgi_printf("<p>Enable hyperlinks (the equivalent of the \"h\" permission) for all users,\n"
         "including user \"nobody\", as long as\n"
         "<ol><li>the User-Agent string in the\n"
         "HTTP header indicates that the request is coming from an actual human\n"
         "being, and\n"
         "<li>the user agent is able to\n"
         "run Javascript in order to set the href= attribute of hyperlinks, and\n"
         "<li>mouse movement is detected (optional - see the checkbox below), and\n"
         "<li>a number of milliseconds have passed since the page loaded.</ol>\n"
         "\n"
         "<p>This setting is designed to give easy access to humans while\n"
         "keeping out robots and spiders.\n"
         "You do not normally want a robot to walk your entire repository because\n"
         "if it does, your server will end up computing diffs and annotations for\n"
         "every historical version of every file and creating ZIPs and tarballs of\n"
         "every historical check-in, which can use a lot of CPU and bandwidth\n"
         "even for relatively small projects.</p>\n"
         "\n"
         "<p>Additional parameters that control this behavior:</p>\n"
         "<blockquote>\n");
  onoff_attribute("Require mouse movement before enabling hyperlinks",
                  "auto-hyperlink-mouseover", "ahmo", 0, 0);
  cgi_printf("<br />\n");
  entry_attribute("Delay in milliseconds before enabling hyperlinks", 5,
                  "auto-hyperlink-delay", "ah-delay", "50", 0);
  cgi_printf("</blockquote>\n"
         "<p>For maximum robot defense, the \"require mouse movement\" should\n"
         "be turned on and the \"Delay\" should be at least 50 milliseconds.</p>\n"
         "(Properties: \"auto-hyperlink\",\n"
         "\"auto-hyperlink-mouseover\", and \"auto-hyperlink-delay\")</p>\n");

  cgi_printf("<hr />\n");
  onoff_attribute("Require a CAPTCHA if not logged in",
                  "require-captcha", "reqcapt", 1, 0);
  cgi_printf("<p>Require a CAPTCHA for edit operations (appending, creating, or\n"
         "editing wiki or tickets or adding attachments to wiki or tickets)\n"
         "for users who are not logged in. (Property: \"require-captcha\")</p>\n");

  cgi_printf("<hr />\n");
  entry_attribute("Public pages", 30, "public-pages",
                  "pubpage", "", 0);
  cgi_printf("<p>A comma-separated list of glob patterns for pages that are accessible\n"
         "without needing a login and using the privileges given by the\n"
         "\"Default privileges\" setting below.  Example use case: Set this field\n"
         "to \"/doc/trunk/www/*\" to give anonymous users read-only permission to the\n"
         "latest version of the embedded documentation in the www/ folder without\n"
         "allowing them to see the rest of the source code.\n"
         "(Property: \"public-pages\")\n"
         "</p>\n");

  cgi_printf("<hr />\n");
  onoff_attribute("Allow users to register themselves",
                  "self-register", "selfregister", 0, 0);
  cgi_printf("<p>Allow users to register themselves through the HTTP UI.\n"
         "The registration form always requires filling in a CAPTCHA\n"
         "(<em>auto-captcha</em> setting is ignored). Still, bear in mind that anyone\n"
         "can register under any user name. This option is useful for public projects\n"
         "where you do not want everyone in any ticket discussion to be named\n"
         "\"Anonymous\".  (Property: \"self-register\")</p>\n");

  cgi_printf("<hr />\n");
  entry_attribute("Default privileges", 10, "default-perms",
                  "defaultperms", "u", 0);
  cgi_printf("<p>Permissions given to users that... <ul><li>register themselves using\n"
         "the self-registration procedure (if enabled), or <li>access \"public\"\n"
         "pages identified by the public-pages glob pattern above, or <li>\n"
         "are users newly created by the administrator.</ul>\n"
         "<p>Recommended value: \"u\" for Reader.\n"
         "<a href=\"%R/setup_ucap_list\">Capability Key</a>.\n"
         "(Property: \"default-perms\")\n"
         "</p>\n");

  cgi_printf("<hr />\n");
  onoff_attribute("Show javascript button to fill in CAPTCHA",
                  "auto-captcha", "autocaptcha", 0, 0);
  cgi_printf("<p>When enabled, a button appears on the login screen for user\n"
         "\"anonymous\" that will automatically fill in the CAPTCHA password.\n"
         "This is less secure than forcing the user to do it manually, but is\n"
         "probably secure enough and it is certainly more convenient for\n"
         "anonymous users.  (Property: \"auto-captcha\")</p>\n");

  cgi_printf("<hr />\n"
         "<p><input type=\"submit\"  name=\"submit\" value=\"Apply Changes\" /></p>\n"
         "</div></form>\n");
  db_end_transaction(0);
  style_footer();
}

/*
** WEBPAGE: setup_login_group
**
** Change how the current repository participates in a login
** group.
*/
void setup_login_group(void){
  const char *zGroup;
  char *zErrMsg = 0;
  Blob fullName;
  char *zSelfRepo;
  const char *zRepo = PD("repo", "");
  const char *zLogin = PD("login", "");
  const char *zPw = PD("pw", "");
  const char *zNewName = PD("newname", "New Login Group");

  login_check_credentials();
  if( !g.perm.Setup ){
    login_needed(0);
    return;
  }
  file_canonical_name(g.zRepositoryName, &fullName, 0);
  zSelfRepo = fossil_strdup(blob_str(&fullName));
  blob_reset(&fullName);
  if( P("join")!=0 ){
    login_group_join(zRepo, zLogin, zPw, zNewName, &zErrMsg);
  }else if( P("leave") ){
    login_group_leave(&zErrMsg);
  }
  style_header("Login Group Configuration");
  if( zErrMsg ){
    cgi_printf("<p class=\"generalError\">%s</p>\n",(zErrMsg));
  }
  zGroup = login_group_name();
  if( zGroup==0 ){
    cgi_printf("<p>This repository (in the file named \"%h\")\n"
           "is not currently part of any login-group.\n"
           "To join a login group, fill out the form below.</p>\n"
           "\n"
           "<form action=\"%s/setup_login_group\" method=\"post\"><div>\n",(zSelfRepo),(g.zTop));
    login_insert_csrf_secret();
    cgi_printf("<blockquote><table border=\"0\">\n"
           "\n"
           "<tr><th align=\"right\">Repository filename in group to join:</th>\n"
           "<td width=\"5\"></td><td>\n"
           "<input type=\"text\" size=\"50\" value=\"%h\" name=\"repo\"></td></tr>\n"
           "\n"
           "<tr><th align=\"right\">Login on the above repo:</th>\n"
           "<td width=\"5\"></td><td>\n"
           "<input type=\"text\" size=\"20\" value=\"%h\" name=\"login\"></td></tr>\n"
           "\n"
           "<tr><th align=\"right\">Password:</th>\n"
           "<td width=\"5\"></td><td>\n"
           "<input type=\"password\" size=\"20\" name=\"pw\"></td></tr>\n"
           "\n"
           "<tr><th align=\"right\">Name of login-group:</th>\n"
           "<td width=\"5\"></td><td>\n"
           "<input type=\"text\" size=\"30\" value=\"%h\" name=\"newname\">\n"
           "(only used if creating a new login-group).</td></tr>\n"
           "\n"
           "<tr><td colspan=\"3\" align=\"center\">\n"
           "<input type=\"submit\" value=\"Join\" name=\"join\"></td></tr>\n"
           "</table></blockquote></div></form>\n",(zRepo),(zLogin),(zNewName));
  }else{
    Stmt q;
    int n = 0;
    cgi_printf("<p>This repository (in the file \"%h\")\n"
           "is currently part of the \"<b>%h</b>\" login group.\n"
           "Other repositories in that group are:</p>\n"
           "<table border=\"0\" cellspacing=\"4\">\n"
           "<tr><td colspan=\"2\"><th align=\"left\">Project Name<td>\n"
           "<th align=\"left\">Repository File</tr>\n",(zSelfRepo),(zGroup));
    db_prepare(&q,
       "SELECT value,"
       "       (SELECT value FROM config"
       "         WHERE name=('peer-name-' || substr(x.name,11)))"
       "  FROM config AS x"
       " WHERE name GLOB 'peer-repo-*'"
       " ORDER BY value"
    );
    while( db_step(&q)==SQLITE_ROW ){
      const char *zRepo = db_column_text(&q, 0);
      const char *zTitle = db_column_text(&q, 1);
      n++;
      cgi_printf("<tr><td align=\"right\">%d.</td><td width=\"4\">\n"
             "<td>%h<td width=\"10\"><td>%h</tr>\n",(n),(zTitle),(zRepo));
    }
    db_finalize(&q);
    cgi_printf("</table>\n"
           "\n"
           "<p><form action=\"%s/setup_login_group\" method=\"post\"><div>\n",(g.zTop));
    login_insert_csrf_secret();
    cgi_printf("To leave this login group press\n"
           "<input type=\"submit\" value=\"Leave Login Group\" name=\"leave\">\n"
           "</form></p>\n"
           "<br />For best results, use the same number of <a href=\"setup_access#ipt\">\n"
           "IP octets</a> in the login cookie across all repositories in the\n"
           "same Login Group.\n"
           "<hr /><h2>Implementation Details</h2>\n"
           "<p>The following are fields from the CONFIG table related to login-groups,\n"
           "provided here for instructional and debugging purposes:</p>\n"
           "<table border='1' class='sortable' data-column-types='ttt' "
           "data-init-sort='1'>\n"
           "<thead><tr>\n"
           "<th>Config.Name<th>Config.Value<th>Config.mtime</tr>\n"
           "</thead><tbody>\n");
    db_prepare(&q, "SELECT name, value, datetime(mtime,'unixepoch') FROM config"
                   " WHERE name GLOB 'peer-*'"
                   "    OR name GLOB 'project-*'"
                   "    OR name GLOB 'login-group-*'"
                   " ORDER BY name");
    while( db_step(&q)==SQLITE_ROW ){
      cgi_printf("<tr><td>%h</td>\n"
             "<td>%h</td>\n"
             "<td>%h</td></tr>\n",(db_column_text(&q,0)),(db_column_text(&q,1)),(db_column_text(&q,2)));
    }
    db_finalize(&q);
    cgi_printf("</tbody></table>\n");
    style_table_sorter();
  }
  style_footer();
}

/*
** WEBPAGE: setup_timeline
**
** Edit administrative settings controlling the display of
** timelines.
*/
void setup_timeline(void){
  double tmDiff;
  char zTmDiff[20];
  static const char *const azTimeFormats[] = {
      "0", "HH:MM",
      "1", "HH:MM:SS",
      "2", "YYYY-MM-DD HH:MM",
      "3", "YYMMDD HH:MM",
      "4", "(off)"
  };
  login_check_credentials();
  if( !g.perm.Admin ){
    login_needed(0);
    return;
  }

  style_header("Timeline Display Preferences");
  db_begin_transaction();
  cgi_printf("<form action=\"%s/setup_timeline\" method=\"post\"><div>\n",(g.zTop));
  login_insert_csrf_secret();
  cgi_printf("<p><input type=\"submit\"  name=\"submit\" value=\"Apply Changes\" /></p>\n");

  cgi_printf("<hr />\n");
  onoff_attribute("Allow block-markup in timeline",
                  "timeline-block-markup", "tbm", 0, 0);
  cgi_printf("<p>In timeline displays, check-in comments can be displayed with or\n"
         "without block markup such as paragraphs, tables, etc.\n"
         "(Property: \"timeline-block-markup\")</p>\n");

  cgi_printf("<hr />\n");
  onoff_attribute("Plaintext comments on timelines",
                  "timeline-plaintext", "tpt", 0, 0);
  cgi_printf("<p>In timeline displays, check-in comments are displayed literally,\n"
         "without any wiki or HTML interpretation.  Use CSS to change\n"
         "display formatting features such as fonts and line-wrapping behavior.\n"
         "(Property: \"timeline-plaintext\")</p>\n");

  cgi_printf("<hr />\n");
  onoff_attribute("Truncate comment at first blank line (Git-style)",
                  "timeline-truncate-at-blank", "ttb", 0, 0);
  cgi_printf("<p>In timeline displays, check-in comments are displayed only through\n"
         "the first blank line.  This is the traditional way to display comments\n"
         "in Git repositories (Property: \"timeline-truncate-at-blank\")</p>\n");

  cgi_printf("<hr />\n");
  onoff_attribute("Break comments at newline characters",
                  "timeline-hard-newlines", "thnl", 0, 0);
  cgi_printf("<p>In timeline displays, newline characters in check-in comments force\n"
         "a line break on the display.\n"
         "(Property: \"timeline-hard-newlines\")</p>\n");

  cgi_printf("<hr />\n");
  onoff_attribute("Use Universal Coordinated Time (UTC)",
                  "timeline-utc", "utc", 1, 0);
  cgi_printf("<p>Show times as UTC (also sometimes called Greenwich Mean Time (GMT) or\n"
         "Zulu) instead of in local time.  On this server, local time is currently\n");
  tmDiff = db_double(0.0, "SELECT julianday('now')");
  tmDiff = db_double(0.0,
        "SELECT (julianday(%.17g,'localtime')-julianday(%.17g))*24.0",
        tmDiff, tmDiff);
  sqlite3_snprintf(sizeof(zTmDiff), zTmDiff, "%.1f", tmDiff);
  if( strcmp(zTmDiff, "0.0")==0 ){
    cgi_printf("the same as UTC and so this setting will make no difference in\n"
           "the display.</p>\n");
  }else if( tmDiff<0.0 ){
    sqlite3_snprintf(sizeof(zTmDiff), zTmDiff, "%.1f", -tmDiff);
    cgi_printf("%s hours behind UTC.</p>\n",(zTmDiff));
  }else{
    cgi_printf("%s hours ahead of UTC.</p>\n",(zTmDiff));
  }
  cgi_printf("<p>(Property: \"timeline-utc\")\n"
         "<hr />\n");
  multiple_choice_attribute("Per-Item Time Format", "timeline-date-format",
            "tdf", "0", count(azTimeFormats)/2, azTimeFormats);
  cgi_printf("<p>If the \"HH:MM\" or \"HH:MM:SS\" format is selected, then the date is shown\n"
         "in a separate box (using CSS class \"timelineDate\") whenever the date\n"
         "changes.  With the \"YYYY-MM-DD&nbsp;HH:MM\" and \"YYMMDD ...\" formats,\n"
         "the complete date and time is shown on every timeline entry using the\n"
         "CSS class \"timelineTime\". (Property: \"timeline-date-format\")</p>\n");

  cgi_printf("<hr />\n");
  entry_attribute("Max timeline comment length", 6,
                  "timeline-max-comment", "tmc", "0", 0);
  cgi_printf("<p>The maximum length of a comment to be displayed in a timeline.\n"
         "\"0\" there is no length limit.\n"
         "(Property: \"timeline-max-comment\")</p>\n");

  cgi_printf("<hr />\n"
         "<p><input type=\"submit\"  name=\"submit\" value=\"Apply Changes\" /></p>\n"
         "</div></form>\n");
  db_end_transaction(0);
  style_footer();
}

/*
** WEBPAGE: setup_settings
**
** Change or view miscellaneous settings.  Part of the
** /setup pages requiring Setup privileges.
*/
void setup_settings(void){
  int nSetting;
  int i;
  Setting const *pSet;
  const Setting *aSetting = setting_info(&nSetting);

  login_check_credentials();
  if( !g.perm.Setup ){
    login_needed(0);
    return;
  }

  style_header("Settings");
  if(!g.repositoryOpen){
    /* Provide read-only access to versioned settings,
       but only if no repo file was explicitly provided. */
    db_open_local(0);
  }
  db_begin_transaction();
  cgi_printf("<p>Settings marked with (v) are \"versionable\" and will be overridden\n"
         "by the contents of managed files named\n"
         "\"<tt>.fossil-settings/</tt><i>SETTING-NAME</i>\".\n"
         "If the file for a versionable setting exists, the value cannot be\n"
         "changed on this screen.</p><hr /><p>\n"
         "\n"
         "<form action=\"%s/setup_settings\" method=\"post\"><div>\n"
         "<table border=\"0\"><tr><td valign=\"top\">\n",(g.zTop));
  login_insert_csrf_secret();
  for(i=0, pSet=aSetting; i<nSetting; i++, pSet++){
    if( pSet->width==0 ){
      int hasVersionableValue = pSet->versionable &&
          (db_get_versioned(pSet->name, NULL)!=0);
      onoff_attribute("", pSet->name,
                      pSet->var!=0 ? pSet->var : pSet->name,
                      is_truth(pSet->def), hasVersionableValue);
      cgi_printf("<a href='%R/help?cmd=%s'>%h</a>\n",(pSet->name),(pSet->name));
      if( pSet->versionable ){
        cgi_printf(" (v)<br />\n");
      } else {
        cgi_printf("<br />\n");
      }
    }
  }
  cgi_printf("<br /><input type=\"submit\"  name=\"submit\" value=\"Apply Changes\" />\n"
         "</td><td style=\"width:50px;\"></td><td valign=\"top\">\n");
  for(i=0, pSet=aSetting; i<nSetting; i++, pSet++){
    if( pSet->width!=0 && !pSet->forceTextArea ){
      int hasVersionableValue = pSet->versionable &&
          (db_get_versioned(pSet->name, NULL)!=0);
      entry_attribute("", /*pSet->width*/ 25, pSet->name,
                      pSet->var!=0 ? pSet->var : pSet->name,
                      (char*)pSet->def, hasVersionableValue);
      cgi_printf("<a href='%R/help?cmd=%s'>%h</a>\n",(pSet->name),(pSet->name));
      if( pSet->versionable ){
        cgi_printf(" (v)<br />\n");
      } else {
        cgi_printf("<br />\n");
      }
    }
  }
  cgi_printf("</td><td style=\"width:50px;\"></td><td valign=\"top\">\n");
  for(i=0, pSet=aSetting; i<nSetting; i++, pSet++){
    if( pSet->width!=0 && pSet->forceTextArea ){
      int hasVersionableValue = db_get_versioned(pSet->name, NULL)!=0;
      cgi_printf("<a href='%R/help?cmd=%s'>%s</a>\n",(pSet->name),(pSet->name));
      if( pSet->versionable ){
        cgi_printf(" (v)<br />\n");
      } else {
        cgi_printf("<br />\n");
      }
      textarea_attribute("", /*rows*/ 2, /*cols*/ 35, pSet->name,
                      pSet->var!=0 ? pSet->var : pSet->name,
                      (char*)pSet->def, hasVersionableValue);
     cgi_printf("<br />\n");
    }
  }
  cgi_printf("</td></tr></table>\n"
         "</div></form>\n");
  db_end_transaction(0);
  style_footer();
}

/*
** WEBPAGE: setup_config
**
** The "Admin/Configuration" page.  Requires Setup privilege.
*/
void setup_config(void){
  login_check_credentials();
  if( !g.perm.Setup ){
    login_needed(0);
    return;
  }

  style_header("WWW Configuration");
  db_begin_transaction();
  cgi_printf("<form action=\"%s/setup_config\" method=\"post\"><div>\n",(g.zTop));
  login_insert_csrf_secret();
  cgi_printf("<input type=\"submit\"  name=\"submit\" value=\"Apply Changes\" /></p>\n"
         "<hr />\n");
  entry_attribute("Project Name", 60, "project-name", "pn", "", 0);
  cgi_printf("<p>A brief project name so visitors know what this site is about.\n"
         "The project name will also be used as the RSS feed title.\n"
         "(Property: \"project-name\")\n"
         "</p>\n"
         "<hr />\n");
  textarea_attribute("Project Description", 3, 80,
                     "project-description", "pd", "", 0);
  cgi_printf("<p>Describe your project. This will be used in page headers for search\n"
         "engines as well as a short RSS description.\n"
         "(Property: \"project-description\")</p>\n"
         "<hr />\n");
  entry_attribute("Tarball and ZIP-archive Prefix", 20, "short-project-name",
                  "spn", "", 0);
  cgi_printf("<p>This is used as a prefix on the names of generated tarballs and\n"
         "ZIP archive. For best results, keep this prefix brief and avoid special\n"
         "characters such as \"/\" and \"\\\".\n"
         "If no tarball prefix is specified, then the full Project Name above is used.\n"
         "(Property: \"short-project-name\")\n"
         "</p>\n"
         "<hr />\n");
  entry_attribute("Download Tag", 20, "download-tag", "dlt", "trunk", 0);
  cgi_printf("<p>The <a href='%R/download'>/download</a> page is designed to provide \n"
         "a convenient place for newbies\n"
         "to download a ZIP archive or a tarball of the project.  By default,\n"
         "the latest trunk check-in is downloaded.  Change this tag to something\n"
         "else (ex: release) to alter the behavior of the /download page.\n"
         "(Property: \"download-tag\")\n"
         "</p>\n"
         "<hr />\n");
  entry_attribute("Index Page", 60, "index-page", "idxpg", "/home", 0);
  cgi_printf("<p>Enter the pathname of the page to display when the \"Home\" menu\n"
         "option is selected and when no pathname is\n"
         "specified in the URL.  For example, if you visit the url:</p>\n"
         "\n"
         "<blockquote><p>%h</p></blockquote>\n"
         "\n"
         "<p>And you have specified an index page of \"/home\" the above will\n"
         "automatically redirect to:</p>\n"
         "\n"
         "<blockquote><p>%h/home</p></blockquote>\n"
         "\n"
         "<p>The default \"/home\" page displays a Wiki page with the same name\n"
         "as the Project Name specified above.  Some sites prefer to redirect\n"
         "to a documentation page (ex: \"/doc/tip/index.wiki\") or to \"/timeline\".</p>\n"
         "\n"
         "<p>Note:  To avoid a redirect loop or other problems, this entry must\n"
         "begin with \"/\" and it must specify a valid page.  For example,\n"
         "\"<b>/home</b>\" will work but \"<b>home</b>\" will not, since it omits the\n"
         "leading \"/\".</p>\n"
         "<p>(Property: \"index-page\")\n"
         "<hr>\n"
         "<p>Extra links to appear on the <a href=\"%R/sitemap\">/sitemap</a> page.\n"
         "Often these are filled in with links like \n"
         "\"/doc/trunk/doc/<i>filename</i>.md\" so that they refer to \n"
         "embedded documentation, or like \"/wiki/<i>pagename</i>\" to refer\n"
         "to wiki pages.\n"
         "Leave blank to omit.\n"
         "<p>\n",(g.zBaseURL),(g.zBaseURL));
  entry_attribute("Documentation Index", 40, "sitemap-docidx", "smdocidx",
                  "", 0);
  cgi_printf("(Property: sitemap-docidx)<br>\n");
  entry_attribute("Download", 40, "sitemap-download", "smdownload",
                  "", 0);
  cgi_printf("(Property: sitemap-download)<br>\n");
  entry_attribute("License", 40, "sitemap-license", "smlicense",
                  "", 0);
  cgi_printf("(Property: sitemap-license)<br>\n");
  entry_attribute("Contact", 40, "sitemap-contact", "smcontact",
                  "", 0);
  cgi_printf("(Property: sitemap-contact)\n"
         "<hr />\n"
         "<p><input type=\"submit\"  name=\"submit\" value=\"Apply Changes\" /></p>\n"
         "</div></form>\n");
  db_end_transaction(0);
  style_footer();
}

/*
** WEBPAGE: setup_wiki
**
** The "Admin/Wiki" page.  Requires Setup privilege.
*/
void setup_wiki(void){
  login_check_credentials();
  if( !g.perm.Setup ){
    login_needed(0);
    return;
  }

  style_header("Wiki Configuration");
  db_begin_transaction();
  cgi_printf("<form action=\"%s/setup_wiki\" method=\"post\"><div>\n",(g.zTop));
  login_insert_csrf_secret();
  cgi_printf("<input type=\"submit\"  name=\"submit\" value=\"Apply Changes\" /></p>\n"
         "<hr />\n");
  onoff_attribute("Associate Wiki Pages With Branches, Tags, or Checkins",
                  "wiki-about", "wiki-about", 1, 0);
  cgi_printf("<p>\n"
         "Associate wiki pages with branches, tags, or checkins, based on\n"
         "the wiki page name.  Wiki pages that begin with \"branch/\", \"checkin/\"\n"
         "or \"tag/\" and which continue with the name of an existing branch, checkin\n"
         "or tag are treated specially when this feature is enabled.\n"
         "<ul>\n"
         "<li> <b>branch/</b><i>branch-name</i>\n"
         "<li> <b>checkin/</b><i>full-checkin-hash</i>\n"
         "<li> <b>tag/</b><i>tag-name</i>\n"
         "</ul>\n"
         "(Property: \"wiki-about\")</p>\n"
         "<hr />\n");
  onoff_attribute("Enable WYSIWYG Wiki Editing",
                  "wysiwyg-wiki", "wysiwyg-wiki", 0, 0);
  cgi_printf("<p>Enable what-you-see-is-what-you-get (WYSIWYG) editing of wiki pages.\n"
         "The WYSIWYG editor generates HTML instead of markup, which makes\n"
         "subsequent manual editing more difficult.\n"
         "(Property: \"wysiwyg-wiki\")</p>\n"
         "<hr />\n");
  onoff_attribute("Use HTML as wiki markup language",
    "wiki-use-html", "wiki-use-html", 0, 0);
  cgi_printf("<p>Use HTML as the wiki markup language. Wiki links will still be parsed\n"
         "but all other wiki formatting will be ignored. This option is helpful\n"
         "if you have chosen to use a rich HTML editor for wiki markup such as\n"
         "TinyMCE.</p>\n"
         "<p><strong>CAUTION:</strong> when\n"
         "enabling, <i>all</i> HTML tags and attributes are accepted in the wiki.\n"
         "No sanitization is done. This means that it is very possible for malicious\n"
         "users to inject dangerous HTML, CSS and JavaScript code into your wiki.</p>\n"
         "<p>This should <strong>only</strong> be enabled when wiki editing is limited\n"
         "to trusted users. It should <strong>not</strong> be used on a publicly\n"
         "editable wiki.</p>\n"
         "(Property: \"wiki-use-html\")\n"
         "<hr />\n"
         "<p><input type=\"submit\"  name=\"submit\" value=\"Apply Changes\" /></p>\n"
         "</div></form>\n");
  db_end_transaction(0);
  style_footer();
}

/*
** WEBPAGE: setup_modreq
**
** Admin page for setting up moderation of tickets and wiki.
*/
void setup_modreq(void){
  login_check_credentials();
  if( !g.perm.Admin ){
    login_needed(0);
    return;
  }

  style_header("Moderator For Wiki And Tickets");
  db_begin_transaction();
  cgi_printf("<form action=\"%R/setup_modreq\" method=\"post\"><div>\n");
  login_insert_csrf_secret();
  cgi_printf("<hr />\n");
  onoff_attribute("Moderate ticket changes",
     "modreq-tkt", "modreq-tkt", 0, 0);
  cgi_printf("<p>When enabled, any change to tickets is subject to the approval\n"
         "by a ticket moderator - a user with the \"q\" or Mod-Tkt privilege.\n"
         "Ticket changes enter the system and are shown locally, but are not\n"
         "synced until they are approved.  The moderator has the option to\n"
         "delete the change rather than approve it.  Ticket changes made by\n"
         "a user who has the Mod-Tkt privilege are never subject to\n"
         "moderation. (Property: \"modreq-tkt\")\n"
         "\n"
         "<hr />\n");
  onoff_attribute("Moderate wiki changes",
     "modreq-wiki", "modreq-wiki", 0, 0);
  cgi_printf("<p>When enabled, any change to wiki is subject to the approval\n"
         "by a wiki moderator - a user with the \"l\" or Mod-Wiki privilege.\n"
         "Wiki changes enter the system and are shown locally, but are not\n"
         "synced until they are approved.  The moderator has the option to\n"
         "delete the change rather than approve it.  Wiki changes made by\n"
         "a user who has the Mod-Wiki privilege are never subject to\n"
         "moderation. (Property: \"modreq-wiki\")\n"
         "</p>\n");

  cgi_printf("<hr />\n"
         "<p><input type=\"submit\"  name=\"submit\" value=\"Apply Changes\" /></p>\n"
         "</div></form>\n");
  db_end_transaction(0);
  style_footer();

}

/*
** WEBPAGE: setup_adunit
**
** Administrative page for configuring and controlling ad units
** and how they are displayed.
*/
void setup_adunit(void){
  login_check_credentials();
  if( !g.perm.Admin ){
    login_needed(0);
    return;
  }
  db_begin_transaction();
  if( P("clear")!=0 && cgi_csrf_safe(1) ){
    db_multi_exec("DELETE FROM config WHERE name GLOB 'adunit*'");
    cgi_replace_parameter("adunit","");
  }

  style_header("Edit Ad Unit");
  cgi_printf("<form action=\"%s/setup_adunit\" method=\"post\"><div>\n",(g.zTop));
  login_insert_csrf_secret();
  cgi_printf("<b>Banner Ad-Unit:</b><br />\n");
 textarea_attribute("", 6, 80, "adunit", "adunit", "", 0);
  cgi_printf("<br />\n"
         "<b>Right-Column Ad-Unit:</b><br />\n");
  textarea_attribute("", 6, 80, "adunit-right", "adright", "", 0);
  cgi_printf("<br />\n");
  onoff_attribute("Omit ads to administrator",
     "adunit-omit-if-admin", "oia", 0, 0);
  cgi_printf("<br />\n");
  onoff_attribute("Omit ads to logged-in users",
     "adunit-omit-if-user", "oiu", 0, 0);
  cgi_printf("<br />\n");
  onoff_attribute("Temporarily disable all ads",
     "adunit-disable", "oall", 0, 0);
  cgi_printf("<br />\n"
         "<input type=\"submit\" name=\"submit\" value=\"Apply Changes\" />\n"
         "<input type=\"submit\" name=\"clear\" value=\"Delete Ad-Unit\" />\n"
         "</div></form>\n"
         "<hr />\n"
         "<b>Ad-Unit Notes:</b><ul>\n"
         "<li>Leave both Ad-Units blank to disable all advertising.\n"
         "<li>The \"Banner Ad-Unit\" is used for wide pages.\n"
         "<li>The \"Right-Column Ad-Unit\" is used on pages with tall, narrow content.\n"
         "<li>If the \"Right-Column Ad-Unit\" is blank, the \"Banner Ad-Unit\" is\n"
         "    used on all pages.\n"
         "<li>Properties: \"adunit\", \"adunit-right\", \"adunit-omit-if-admin\", and\n"
         "    \"adunit-omit-if-user\".\n"
         "<li>Suggested <a href=\"setup_skinedit?w=0\">CSS</a> changes:\n"
         "<blockquote><pre>\n"
         "div.adunit_banner {\n"
         "  margin: auto;\n"
         "  width: 100%%;\n"
         "}\n"
         "div.adunit_right {\n"
         "  float: right;\n"
         "}\n"
         "div.adunit_right_container {\n"
         "  min-height: <i>height-of-right-column-ad-unit</i>;\n"
         "}\n"
         "</pre></blockquote>\n"
         "<li>For a place-holder Ad-Unit for testing, Copy/Paste the following\n"
         "with appropriate adjustments to \"width:\" and \"height:\".\n"
         "<blockquote><pre>\n"
         "&lt;div style='\n"
         "  margin: 0 auto;\n"
         "  width: 600px;\n"
         "  height: 90px;\n"
         "  border: 1px solid #f11;\n"
         "  background-color: #fcc;\n"
         "'&gt;Demo Ad&lt;/div&gt;\n"
         "</pre></blockquote>\n"
         "</li>\n");
  style_footer();
  db_end_transaction(0);
}

/*
** WEBPAGE: setup_logo
**
** Administrative page for changing the logo image.
*/
void setup_logo(void){
  const char *zLogoMtime = db_get_mtime("logo-image", 0, 0);
  const char *zLogoMime = db_get("logo-mimetype","image/gif");
  const char *aLogoImg = P("logoim");
  int szLogoImg = atoi(PD("logoim:bytes","0"));
  const char *zBgMtime = db_get_mtime("background-image", 0, 0);
  const char *zBgMime = db_get("background-mimetype","image/gif");
  const char *aBgImg = P("bgim");
  int szBgImg = atoi(PD("bgim:bytes","0"));
  if( szLogoImg>0 ){
    zLogoMime = PD("logoim:mimetype","image/gif");
  }
  if( szBgImg>0 ){
    zBgMime = PD("bgim:mimetype","image/gif");
  }
  login_check_credentials();
  if( !g.perm.Admin ){
    login_needed(0);
    return;
  }
  db_begin_transaction();
  if( !cgi_csrf_safe(1) ){
    /* Allow no state changes if not safe from CSRF */
  }else if( P("setlogo")!=0 && zLogoMime && zLogoMime[0] && szLogoImg>0 ){
    Blob img;
    Stmt ins;
    blob_init(&img, aLogoImg, szLogoImg);
    db_prepare(&ins,
        "REPLACE INTO config(name,value,mtime)"
        " VALUES('logo-image',:bytes,now())"
    );
    db_bind_blob(&ins, ":bytes", &img);
    db_step(&ins);
    db_finalize(&ins);
    db_multi_exec(
       "REPLACE INTO config(name,value,mtime) VALUES('logo-mimetype',%Q,now())",
       zLogoMime
    );
    db_end_transaction(0);
    cgi_redirect("setup_logo");
  }else if( P("clrlogo")!=0 ){
    db_multi_exec(
       "DELETE FROM config WHERE name IN "
           "('logo-image','logo-mimetype')"
    );
    db_end_transaction(0);
    cgi_redirect("setup_logo");
  }else if( P("setbg")!=0 && zBgMime && zBgMime[0] && szBgImg>0 ){
    Blob img;
    Stmt ins;
    blob_init(&img, aBgImg, szBgImg);
    db_prepare(&ins,
        "REPLACE INTO config(name,value,mtime)"
        " VALUES('background-image',:bytes,now())"
    );
    db_bind_blob(&ins, ":bytes", &img);
    db_step(&ins);
    db_finalize(&ins);
    db_multi_exec(
       "REPLACE INTO config(name,value,mtime)"
       " VALUES('background-mimetype',%Q,now())",
       zBgMime
    );
    db_end_transaction(0);
    cgi_redirect("setup_logo");
  }else if( P("clrbg")!=0 ){
    db_multi_exec(
       "DELETE FROM config WHERE name IN "
           "('background-image','background-mimetype')"
    );
    db_end_transaction(0);
    cgi_redirect("setup_logo");
  }
  style_header("Edit Project Logo And Background");
  cgi_printf("<p>The current project logo has a MIME-Type of <b>%h</b>\n"
         "and looks like this:</p>\n"
         "<blockquote><p><img src=\"%s/logo/%z\" "
         "alt=\"logo\" border=\"1\" />\n"
         "</p></blockquote>\n"
         "\n"
         "<form action=\"%s/setup_logo\" method=\"post\"\n"
         " enctype=\"multipart/form-data\"><div>\n"
         "<p>The logo is accessible to all users at this URL:\n"
         "<a href=\"%s/logo\">%s/logo</a>.\n"
         "The logo may or may not appear on each\n"
         "page depending on the <a href=\"setup_skinedit?w=0\">CSS</a> and\n"
         "<a href=\"setup_skinedit?w=2\">header setup</a>.\n"
         "To change the logo image, use the following form:</p>\n",(zLogoMime),(g.zTop),(zLogoMtime),(g.zTop),(g.zBaseURL),(g.zBaseURL));
  login_insert_csrf_secret();
  cgi_printf("Logo Image file:\n"
         "<input type=\"file\" name=\"logoim\" size=\"60\" accept=\"image/*\" />\n"
         "<p align=\"center\">\n"
         "<input type=\"submit\" name=\"setlogo\" value=\"Change Logo\" />\n"
         "<input type=\"submit\" name=\"clrlogo\" value=\"Revert To Default\" /></p>\n"
         "<p>(Properties: \"logo-image\" and \"logo-mimetype\")\n"
         "</div></form>\n"
         "<hr />\n"
         "\n"
         "<p>The current background image has a MIME-Type of <b>%h</b>\n"
         "and looks like this:</p>\n"
         "<blockquote><p><img src=\"%s/background/%z\" "
         "alt=\"background\" border=1 />\n"
         "</p></blockquote>\n"
         "\n"
         "<form action=\"%s/setup_logo\" method=\"post\"\n"
         " enctype=\"multipart/form-data\"><div>\n"
         "<p>The background image is accessible to all users at this URL:\n"
         "<a href=\"%s/background\">%s/background</a>.\n"
         "The background image may or may not appear on each\n"
         "page depending on the <a href=\"setup_skinedit?w=0\">CSS</a> and\n"
         "<a href=\"setup_skinedit?w=2\">header setup</a>.\n"
         "To change the background image, use the following form:</p>\n",(zBgMime),(g.zTop),(zBgMtime),(g.zTop),(g.zBaseURL),(g.zBaseURL));
  login_insert_csrf_secret();
  cgi_printf("Background image file:\n"
         "<input type=\"file\" name=\"bgim\" size=\"60\" accept=\"image/*\" />\n"
         "<p align=\"center\">\n"
         "<input type=\"submit\" name=\"setbg\" value=\"Change Background\" />\n"
         "<input type=\"submit\" name=\"clrbg\" value=\"Revert To Default\" /></p>\n"
         "</div></form>\n"
         "<p>(Properties: \"background-image\" and \"background-mimetype\")\n"
         "<hr />\n"
         "\n"
         "<p><span class=\"note\">Note:</span>  Your browser has probably cached these\n"
         "images, so you may need to press the Reload button before changes will\n"
         "take effect. </p>\n");
  style_footer();
  db_end_transaction(0);
}

/*
** Prevent the RAW SQL feature from being used to ATTACH a different
** database and query it.
**
** Actually, the RAW SQL feature only does a single statement per request.
** So it is not possible to ATTACH and then do a separate query.  This
** routine is not strictly necessary, therefore.  But it does not hurt
** to be paranoid.
*/
int raw_sql_query_authorizer(
  void *pError,
  int code,
  const char *zArg1,
  const char *zArg2,
  const char *zArg3,
  const char *zArg4
){
  if( code==SQLITE_ATTACH ){
    return SQLITE_DENY;
  }
  return SQLITE_OK;
}


/*
** WEBPAGE: admin_sql
**
** Run raw SQL commands against the database file using the web interface.
** Requires Setup privileges.
*/
void sql_page(void){
  const char *zQ;
  int go = P("go")!=0;
  login_check_credentials();
  if( !g.perm.Setup ){
    login_needed(0);
    return;
  }
  add_content_sql_commands(g.db);
  zQ = cgi_csrf_safe(1) ? P("q") : 0;
  style_header("Raw SQL Commands");
  cgi_printf("<p><b>Caution:</b> There are no restrictions on the SQL that can be\n"
         "run by this page.  You can do serious and irrepairable damage to the\n"
         "repository.  Proceed with extreme caution.</p>\n"
         "\n");
#if 0
  cgi_printf("<p>Only the first statement in the entry box will be run.\n"
         "Any subsequent statements will be silently ignored.</p>\n"
         "\n"
         "<p>Database names:<ul><li>repository\n");
  if( g.zConfigDbName ){
    cgi_printf("<li>configdb\n");
  }
  if( g.localOpen ){
    cgi_printf("<li>localdb\n");
  }
  cgi_printf("</ul></p>\n");
#endif

  if( P("configtab") ){
    /* If the user presses the "CONFIG Table Query" button, populate the
    ** query text with a pre-packaged query against the CONFIG table */
    zQ = "SELECT\n"
         " CASE WHEN length(name)<50 THEN name ELSE printf('%.50s...',name)"
         "  END AS name,\n"
         " CASE WHEN typeof(value)<>'blob' AND length(value)<80 THEN value\n"
         "           ELSE '...' END AS value,\n"
         " datetime(mtime, 'unixepoch') AS mtime\n"
         "FROM config\n"
         "-- ORDER BY mtime DESC; -- optional";
     go = 1;
  }
  cgi_printf("\n"
         "<form method=\"post\" action=\"%s/admin_sql\">\n",(g.zTop));
  login_insert_csrf_secret();
  cgi_printf("SQL:<br />\n"
         "<textarea name=\"q\" rows=\"8\" cols=\"80\">%h</textarea><br />\n"
         "<input type=\"submit\" name=\"go\" value=\"Run SQL\">\n"
         "<input type=\"submit\" name=\"schema\" value=\"Show Schema\">\n"
         "<input type=\"submit\" name=\"tablelist\" value=\"List Tables\">\n"
         "<input type=\"submit\" name=\"configtab\" value=\"CONFIG Table Query\">\n"
         "</form>\n",(zQ));
  if( P("schema") ){
    zQ = sqlite3_mprintf(
            "SELECT sql FROM repository.sqlite_master"
            " WHERE sql IS NOT NULL ORDER BY name");
    go = 1;
  }else if( P("tablelist") ){
    zQ = sqlite3_mprintf(
            "SELECT name FROM repository.sqlite_master WHERE type='table'"
            " ORDER BY name");
    go = 1;
  }
  if( go ){
    sqlite3_stmt *pStmt;
    int rc;
    const char *zTail;
    int nCol;
    int nRow = 0;
    int i;
    cgi_printf("<hr />\n");
    login_verify_csrf_secret();
    sqlite3_set_authorizer(g.db, raw_sql_query_authorizer, 0);
    rc = sqlite3_prepare_v2(g.db, zQ, -1, &pStmt, &zTail);
    if( rc!=SQLITE_OK ){
      cgi_printf("<div class=\"generalError\">%h</div>\n",(sqlite3_errmsg(g.db)));
      sqlite3_finalize(pStmt);
    }else if( pStmt==0 ){
      /* No-op */
    }else if( (nCol = sqlite3_column_count(pStmt))==0 ){
      sqlite3_step(pStmt);
      rc = sqlite3_finalize(pStmt);
      if( rc ){
        cgi_printf("<div class=\"generalError\">%h</div>\n",(sqlite3_errmsg(g.db)));
      }
    }else{
      cgi_printf("<table border=1>\n");
      while( sqlite3_step(pStmt)==SQLITE_ROW ){
        if( nRow==0 ){
          cgi_printf("<tr>\n");
          for(i=0; i<nCol; i++){
            cgi_printf("<th>%h</th>\n",(sqlite3_column_name(pStmt, i)));
          }
          cgi_printf("</tr>\n");
        }
        nRow++;
        cgi_printf("<tr>\n");
        for(i=0; i<nCol; i++){
          switch( sqlite3_column_type(pStmt, i) ){
            case SQLITE_INTEGER:
            case SQLITE_FLOAT: {
               cgi_printf("<td align=\"right\" valign=\"top\">\n"
                      "%s</td>\n",(sqlite3_column_text(pStmt, i)));
               break;
            }
            case SQLITE_NULL: {
               cgi_printf("<td valign=\"top\" align=\"center\"><i>NULL</i></td>\n");
               break;
            }
            case SQLITE_TEXT: {
               const char *zText = (const char*)sqlite3_column_text(pStmt, i);
               cgi_printf("<td align=\"left\" valign=\"top\"\n"
                      "style=\"white-space:pre;\">%h</td>\n",(zText));
               break;
            }
            case SQLITE_BLOB: {
               cgi_printf("<td valign=\"top\" align=\"center\">\n"
                      "<i>%d-byte BLOB</i></td>\n",(sqlite3_column_bytes(pStmt, i)));
               break;
            }
          }
        }
        cgi_printf("</tr>\n");
      }
      sqlite3_finalize(pStmt);
      cgi_printf("</table>\n");
    }
  }
  style_footer();
}


/*
** WEBPAGE: admin_th1
**
** Run raw TH1 commands using the web interface.  If Tcl integration was
** enabled at compile-time and the "tcl" setting is enabled, Tcl commands
** may be run as well.  Requires Admin privilege.
*/
void th1_page(void){
  const char *zQ = P("q");
  int go = P("go")!=0;
  login_check_credentials();
  if( !g.perm.Setup ){
    login_needed(0);
    return;
  }
  style_header("Raw TH1 Commands");
  cgi_printf("<p><b>Caution:</b> There are no restrictions on the TH1 that can be\n"
         "run by this page.  If Tcl integration was enabled at compile-time and\n"
         "the \"tcl\" setting is enabled, Tcl commands may be run as well.</p>\n"
         "\n"
         "<form method=\"post\" action=\"%s/admin_th1\">\n",(g.zTop));
  login_insert_csrf_secret();
  cgi_printf("TH1:<br />\n"
         "<textarea name=\"q\" rows=\"5\" cols=\"80\">%h</textarea><br />\n"
         "<input type=\"submit\" name=\"go\" value=\"Run TH1\">\n"
         "</form>\n",(zQ));
  if( go ){
    const char *zR;
    int rc;
    int n;
    cgi_printf("<hr />\n");
    login_verify_csrf_secret();
    rc = Th_Eval(g.interp, 0, zQ, -1);
    zR = Th_GetResult(g.interp, &n);
    if( rc==TH_OK ){
      cgi_printf("<pre class=\"th1result\">%h</pre>\n",(zR));
    }else{
      cgi_printf("<pre class=\"th1error\">%h</pre>\n",(zR));
    }
  }
  style_footer();
}

/*
** WEBPAGE: admin_log
**
** Shows the contents of the admin_log table, which is only created if
** the admin-log setting is enabled. Requires Admin or Setup ('a' or
** 's') permissions.
*/
void page_admin_log(){
  Stmt stLog;
  int limit;                 /* How many entries to show */
  int ofst;                  /* Offset to the first entry */
  int fLogEnabled;
  int counter = 0;
  login_check_credentials();
  if( !g.perm.Admin ){
    login_needed(0);
    return;
  }
  style_header("Admin Log");
  create_admin_log_table();
  limit = atoi(PD("n","200"));
  ofst = atoi(PD("x","0"));
  fLogEnabled = db_get_boolean("admin-log", 0);
  cgi_printf("<div>Admin logging is %s.\n"
         "(Change this on the <a href=\"setup_settings\">settings</a> page.)</div>\n",(fLogEnabled?"on":"off"));

  if( ofst>0 ){
    int prevx = ofst - limit;
    if( prevx<0 ) prevx = 0;
    cgi_printf("<p><a href=\"admin_log?n=%d&x=%d\">[Newer]</a></p>\n",(limit),(prevx));
  }
  db_prepare(&stLog,
    "SELECT datetime(time,'unixepoch'), who, page, what "
    "FROM admin_log "
    "ORDER BY time DESC");
  style_table_sorter();
  cgi_printf("<table class=\"sortable adminLogTable\" width=\"100%%\" "
         " data-column-types='Tttx' data-init-sort='1'>\n"
         "<thead>\n"
         "<th>Time</th>\n"
         "<th>User</th>\n"
         "<th>Page</th>\n"
         "<th width=\"60%%\">Message</th>\n"
         "</thead><tbody>\n");
  while( SQLITE_ROW == db_step(&stLog) ){
    const char *zTime = db_column_text(&stLog, 0);
    const char *zUser = db_column_text(&stLog, 1);
    const char *zPage = db_column_text(&stLog, 2);
    const char *zMessage = db_column_text(&stLog, 3);
    counter++;
    if( counter<ofst ) continue;
    if( counter>ofst+limit ) break;
    cgi_printf("<tr class=\"row%d\">\n"
           "<td class=\"adminTime\">%s</td>\n"
           "<td>%s</td>\n"
           "<td>%s</td>\n"
           "<td>%h</td>\n"
           "</tr>\n",(counter%2),(zTime),(zUser),(zPage),(zMessage));
  }
  db_finalize(&stLog);
  cgi_printf("</tbody></table>\n");
  if( counter>ofst+limit ){
    cgi_printf("<p><a href=\"admin_log?n=%d&x=%d\">[Older]</a></p>\n",(limit),(limit+ofst));
  }
  style_footer();
}

/*
** WEBPAGE: srchsetup
**
** Configure the search engine.  Requires Admin privilege.
*/
void page_srchsetup(){
  login_check_credentials();
  if( !g.perm.Admin ){
    login_needed(0);
    return;
  }
  style_header("Search Configuration");
  cgi_printf("<form action=\"%s/srchsetup\" method=\"post\"><div>\n",(g.zTop));
  login_insert_csrf_secret();
  cgi_printf("<div style=\"text-align:center;font-weight:bold;\">\n"
         "Server-specific settings that affect the\n"
         "<a href=\"%R/search\">/search</a> webpage.\n"
         "</div>\n"
         "<hr />\n");
  textarea_attribute("Document Glob List", 3, 35, "doc-glob", "dg", "", 0);
  cgi_printf("<p>The \"Document Glob List\" is a comma- or newline-separated list\n"
         "of GLOB expressions that identify all documents within the source\n"
         "tree that are to be searched when \"Document Search\" is enabled.\n"
         "Some examples:\n"
         "<table border=0 cellpadding=2 align=center>\n"
         "<tr><td>*.wiki,*.html,*.md,*.txt<td style=\"width: 4x;\">\n"
         "<td>Search all wiki, HTML, Markdown, and Text files</tr>\n"
         "<tr><td>doc/*.md,*/README.txt,README.txt<td>\n"
         "<td>Search all Markdown files in the doc/ subfolder and all README.txt\n"
         "files.</tr>\n"
         "<tr><td>*<td><td>Search all checked-in files</tr>\n"
         "<tr><td><i>(blank)</i><td>\n"
         "<td>Search nothing. (Disables document search).</tr>\n"
         "</table>\n"
         "<hr />\n");
  entry_attribute("Document Branch", 20, "doc-branch", "db", "trunk", 0);
  cgi_printf("<p>When searching documents, use the versions of the files found at the\n"
         "type of the \"Document Branch\" branch.  Recommended value: \"trunk\".\n"
         "Document search is disabled if blank.\n"
         "<hr />\n");
  onoff_attribute("Search Check-in Comments", "search-ci", "sc", 0, 0);
  cgi_printf("<br />\n");
  onoff_attribute("Search Documents", "search-doc", "sd", 0, 0);
  cgi_printf("<br />\n");
  onoff_attribute("Search Tickets", "search-tkt", "st", 0, 0);
  cgi_printf("<br />\n");
  onoff_attribute("Search Wiki", "search-wiki", "sw", 0, 0);
  cgi_printf("<br />\n");
  onoff_attribute("Search Tech Notes", "search-technote", "se", 0, 0);
  cgi_printf("<br />\n");
  onoff_attribute("Search Forum", "search-forum", "sf", 0, 0);
  cgi_printf("<hr />\n"
         "<p><input type=\"submit\"  name=\"submit\" value=\"Apply Changes\" /></p>\n"
         "<hr />\n");
  if( P("fts0") ){
    search_drop_index();
  }else if( P("fts1") ){
    search_drop_index();
    search_create_index();
    search_fill_index();
    search_update_index(search_restrict(SRCH_ALL));
  }
  if( search_index_exists() ){
    cgi_printf("<p>Currently using an SQLite FTS4 search index. This makes search\n"
           "run faster, especially on large repositories, but takes up space.</p>\n");
    onoff_attribute("Use Porter Stemmer","search-stemmer","ss",0,0);
    cgi_printf("<p><input type=\"submit\" name=\"fts0\" value=\"Delete The Full-Text Index\">\n"
           "<input type=\"submit\" name=\"fts1\" value=\"Rebuild The Full-Text Index\">\n");
  }else{
    cgi_printf("<p>The SQLite FTS4 search index is disabled.  All searching will be\n"
           "a full-text scan.  This usually works fine, but can be slow for\n"
           "larger repositories.</p>\n");
    onoff_attribute("Use Porter Stemmer","search-stemmer","ss",0,0);
    cgi_printf("<p><input type=\"submit\" name=\"fts1\" value=\"Create A Full-Text Index\">\n");
  }
  cgi_printf("</div></form>\n");
  style_footer();
}

/*
** A URL Alias originally called zOldName is now zNewName/zValue.
** Write SQL to make this change into pSql.
**
** If zNewName or zValue is an empty string, then delete the entry.
**
** If zOldName is an empty string, create a new entry.
*/
static void setup_update_url_alias(
  Blob *pSql,
  const char *zOldName,
  const char *zNewName,
  const char *zValue
){
  if( !cgi_csrf_safe(1) ) return;
  if( zNewName[0]==0 || zValue[0]==0 ){
    if( zOldName[0] ){
      blob_append_sql(pSql,
        "DELETE FROM config WHERE name='walias:%q';\n",
        zOldName);
    }
    return;
  }
  if( zOldName[0]==0 ){
    blob_append_sql(pSql,
      "INSERT INTO config(name,value,mtime) VALUES('walias:%q',%Q,now());\n",
      zNewName, zValue);
    return;
  }
  if( strcmp(zOldName, zNewName)!=0 ){
    blob_append_sql(pSql,
       "UPDATE config SET name='walias:%q', value=%Q, mtime=now()"
       " WHERE name='walias:%q';\n",
       zNewName, zValue, zOldName);
  }else{
    blob_append_sql(pSql,
       "UPDATE config SET value=%Q, mtime=now()"
       " WHERE name='walias:%q' AND value<>%Q;\n",
       zValue, zOldName, zValue);
  }
}

/*
** WEBPAGE: waliassetup
**
** Configure the URL aliases
*/
void page_waliassetup(){
  Stmt q;
  int cnt = 0;
  Blob namelist;
  login_check_credentials();
  if( !g.perm.Admin ){
    login_needed(0);
    return;
  }
  style_header("URL Alias Configuration");
  if( P("submit")!=0 ){
    Blob token;
    Blob sql;
    const char *zNewName;
    const char *zValue;
    char zCnt[10];
    login_verify_csrf_secret();
    blob_init(&namelist, PD("namelist",""), -1);
    blob_init(&sql, 0, 0);
    while( blob_token(&namelist, &token) ){
      const char *zOldName = blob_str(&token);
      sqlite3_snprintf(sizeof(zCnt), zCnt, "n%d", cnt);
      zNewName = PD(zCnt, "");
      sqlite3_snprintf(sizeof(zCnt), zCnt, "v%d", cnt);
      zValue = PD(zCnt, "");
      setup_update_url_alias(&sql, zOldName, zNewName, zValue);
      cnt++;
      blob_reset(&token);
    }
    sqlite3_snprintf(sizeof(zCnt), zCnt, "n%d", cnt);
    zNewName = PD(zCnt,"");
    sqlite3_snprintf(sizeof(zCnt), zCnt, "v%d", cnt);
    zValue = PD(zCnt,"");
    setup_update_url_alias(&sql, "", zNewName, zValue);
    db_multi_exec("%s", blob_sql_text(&sql));
    blob_reset(&sql);
    blob_reset(&namelist);
    cnt = 0;
  }
  db_prepare(&q,
      "SELECT substr(name,8), value FROM config WHERE name GLOB 'walias:/*'"
      " UNION ALL SELECT '', ''"
  );
  cgi_printf("<form action=\"%s/waliassetup\" method=\"post\"><div>\n",(g.zTop));
  login_insert_csrf_secret();
  cgi_printf("<table border=0 cellpadding=5>\n"
         "<tr><th>Alias<th>URI That The Alias Maps Into\n");
  blob_init(&namelist, 0, 0);
  while( db_step(&q)==SQLITE_ROW ){
    const char *zName = db_column_text(&q, 0);
    const char *zValue = db_column_text(&q, 1);
    cgi_printf("<tr><td>\n"
           "<input type='text' size='20' value='%h' name='n%d'>\n"
           "</td><td>\n"
           "<input type='text' size='80' value='%h' name='v%d'>\n"
           "</td></tr>\n",(zName),(cnt),(zValue),(cnt));
    cnt++;
    if( blob_size(&namelist)>0 ) blob_append(&namelist, " ", 1);
    blob_append(&namelist, zName, -1);
  }
  db_finalize(&q);
  cgi_printf("<tr><td>\n"
         "<input type='hidden' name='namelist' value='%h'>\n"
         "<input type='submit' name='submit' value=\"Apply Changes\">\n"
         "</td><td></td></tr>\n"
         "</table></form>\n"
         "<hr>\n"
         "<p>When the first term of an incoming URL exactly matches one of\n"
         "the \"Aliases\" on the left-hand side (LHS) above, the URL is\n"
         "converted into the corresponding form on the right-hand side (RHS).\n"
         "<ul>\n"
         "<li><p>\n"
         "The LHS is compared against only the first term of the incoming URL.\n"
         "All LHS entries in the alias table should therefore begin with a\n"
         "single \"/\" followed by a single path element.\n"
         "<li><p>\n"
         "The RHS entries in the alias table should begin with a single \"/\"\n"
         "followed by a path element, and optionally followed by \"?\" and a\n"
         "list of query parameters.\n"
         "<li><p>\n"
         "Query parameters on the RHS are added to the set of query parameters\n"
         "in the incoming URL.\n"
         "<li><p>\n"
         "If the same query parameter appears in both the incoming URL and\n"
         "on the RHS of the alias, the RHS query parameter value overwrites\n"
         "the value on the incoming URL.\n"
         "<li><p>\n"
         "If a query parameter on the RHS of the alias is of the form \"X!\"\n"
         "(a name followed by \"!\") then the X query parameter is removed\n"
         "from the incoming URL if\n"
         "it exists.\n"
         "<li><p>\n"
         "Only a single alias operation occurs.  It is not possible to nest aliases.\n"
         "The RHS entries must be built-in webpage names.\n"
         "<li><p>\n"
         "The alias table is only checked if no built-in webpage matches\n"
         "the incoming URL.\n"
         "Hence, it is not possible to override a built-in webpage using aliases.\n"
         "This is by design.\n"
         "</ul>\n"
         "\n"
         "<p>To delete an entry from the alias table, change its name or value to an\n"
         "empty string and press \"Apply Changes\".\n"
         "\n"
         "<p>To add a new alias, fill in the name and value in the bottom row\n"
         "of the table above and press \"Apply Changes\".\n",(blob_str(&namelist)));
  style_footer();
}
