#line 1 "..\\src\\sitemap.c"
/*
** Copyright (c) 2014 D. Richard Hipp
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
** This file contains code to implement the sitemap webpage.
*/
#include "config.h"
#include "sitemap.h"
#include <assert.h>

/*
** WEBPAGE: sitemap
**
** List some of the web pages offered by the Fossil web engine.  This
** page is intended as a supplement to the menu bar on the main screen.
** That is, this page is designed to hold links that are omitted from
** the main menu due to lack of space.
*/
void sitemap_page(void){
  int srchFlags;
  int inSublist = 0;
  int i;
  int isPopup = 0;         /* This is an XMLHttpRequest() for /sitemap */
  const struct {
    const char *zTitle;
    const char *zProperty;
  } aExtra[] = {
    { "Documentation",  "sitemap-docidx" },
    { "Download",       "sitemap-download" },
    { "License",        "sitemap-license" },
    { "Contact",        "sitemap-contact" },
  };

  login_check_credentials();
  if( P("popup")!=0 && cgi_csrf_safe(0) ){
    /* If this is a POST from the same origin with the popup=1 parameter,
    ** then disable anti-robot defenses */
    isPopup = 1;
    g.perm.Hyperlink = 1;
    g.javascriptHyperlink = 0;
  }
  srchFlags = search_restrict(SRCH_ALL);
  if( !isPopup ){
    style_header("Site Map");
    style_adunit_config(ADUNIT_RIGHT_OK);
  }
  cgi_printf("<ul id=\"sitemap\" class=\"columns\" style=\"column-width:20em\">\n"
         "<li>%zHome Page</a>\n",(href("%R/home")));
  for(i=0; i<sizeof(aExtra)/sizeof(aExtra[0]); i++){
    char *z = db_get(aExtra[i].zProperty,0);
    if( z==0 || z[0]==0 ) continue;
    if( !inSublist ){
      cgi_printf("<ul>\n");
      inSublist = 1;
    }
    if( z[0]=='/' ){
      cgi_printf("<li>%z%s</a></li>\n",(href("%R%s",z)),(aExtra[i].zTitle));
    }else{
      cgi_printf("<li>%z%s</a></li>\n",(href("%s",z)),(aExtra[i].zTitle));
    }
  }
  if( srchFlags & SRCH_DOC ){
    if( !inSublist ){
      cgi_printf("<ul>\n");
      inSublist = 1;
    }
    cgi_printf("<li>%zDocumentation Search</a></li>\n",(href("%R/docsrch")));
  }
  if( inSublist ){
    cgi_printf("</ul>\n");
    inSublist = 0;    
  }
  cgi_printf("</li>\n");
  if( g.perm.Read ){
    cgi_printf("<li>%zFile Browser</a>\n"
           "  <ul>\n"
           "  <li>%zTree-view,\n"
           "       Trunk Check-in</a></li>\n"
           "  <li>%zFlat-view</a></li>\n"
           "  <li>%zFile ages for Trunk</a></li>\n"
           "  <li>%zUnversioned Files</a>\n"
           "</ul>\n",(href("%R/tree")),(href("%R/tree?type=tree&ci=trunk")),(href("%R/tree?type=flat")),(href("%R/fileage?name=trunk")),(href("%R/uvlist")));
  }
  if( g.perm.Read ){
    cgi_printf("<li>%zProject Timeline</a>\n"
           "<ul>\n"
           "  <li>%zActivity Reports</a></li>\n"
           "  <li>%zFile name changes</a></li>\n"
           "  <li>%zForks</a></li>\n"
           "  <li>%zFirst 10\n"
           "      check-ins</a></li>\n"
           "</ul>\n"
           "</li>\n",(href("%R/timeline")),(href("%R/reports")),(href("%R/timeline?n=all&namechng")),(href("%R/timeline?n=all&forks")),(href("%R/timeline?a=1970-01-01&y=ci&n=10")));
  }
  if( g.perm.Read ){
    cgi_printf("<li>%zBranches</a>\n"
           "<ul>\n"
           "  <li>%zTags</a></li>\n"
           "  <li>%zLeaf Check-ins</a></li>\n"
           "</ul>\n"
           "</li>\n",(href("%R/brlist")),(href("%R/taglist")),(href("%R/leaves")));
  }
  if( srchFlags ){
    cgi_printf("<li>%zSearch</a></li>\n",(href("%R/search")));
  }
  if( g.perm.RdForum ){
    cgi_printf("<li>%zForum</a>\n"
           "<ul>\n"
           "  <li>%zRecent activity</a></li>\n"
           "</ul>\n"
           "</li>\n",(href("%R/forum")),(href("%R/timeline?y=f")));
  }
  if( g.perm.RdTkt ){
    cgi_printf("<li>%zTickets</a>\n"
           "  <ul>\n",(href("%R/reportlist")));
    if( srchFlags & SRCH_TKT ){
      cgi_printf("  <li>%zTicket Search</a></li>\n",(href("%R/tktsrch")));
    }
    cgi_printf("  <li>%zRecent activity</a></li>\n"
           "  <li>%zList of Attachments</a></li>\n"
           "  </ul>\n"
           "</li>\n",(href("%R/timeline?y=t")),(href("%R/attachlist")));
  }
  if( g.perm.RdWiki ){
    cgi_printf("<li>%zWiki</a>\n"
           "  <ul>\n",(href("%R/wikihelp")));
    if( srchFlags & SRCH_WIKI ){
      cgi_printf("    <li>%zWiki Search</a></li>\n",(href("%R/wikisrch")));
    }
    cgi_printf("    <li>%zList of Wiki Pages</a></li>\n"
           "    <li>%zRecent activity</a></li>\n"
           "    <li>%zSandbox</a></li>\n"
           "    <li>%zList of Attachments</a></li>\n"
           "  </ul>\n"
           "</li>\n",(href("%R/wcontent")),(href("%R/timeline?y=w")),(href("%R/wiki?name=Sandbox")),(href("%R/attachlist")));
  }

  if( !g.zLogin ){
    cgi_printf("<li>%zLogin</a>\n",(href("%R/login")));
    if( login_self_register_available(0) ){
       cgi_printf("<ul>\n"
              "<li>%zCreate a new account</a></li>\n",(href("%R/register")));
       inSublist = 1;
    }
  }else {
    cgi_printf("<li>%zLogout</a>\n",(href("%R/logout")));
    if( g.perm.Password ){
      cgi_printf("<ul>\n"
             "<li>%zChange Password</a></li>\n",(href("%R/logout")));
      inSublist = 1;
    }
  }
  if( alert_enabled() && g.perm.EmailAlert ){
    if( !inSublist ){
      inSublist = 1;
      cgi_printf("<ul>\n");
    }
    if( login_is_individual() ){
      cgi_printf("<li>%zEmail Alerts</a></li>\n",(href("%R/alerts")));
    }else{
      cgi_printf("<li>%zSubscribe to Email Alerts</a></li>\n",(href("%R/subscribe")));
    }
  }
  if( inSublist ){
    cgi_printf("</ul>\n");
    inSublist = 0;
  }
  cgi_printf("</li>\n");

  if( g.perm.Read ){
    cgi_printf("<li>%zRepository Status</a>\n"
           "  <ul>\n"
           "  <li>%zCollisions on hash prefixes</a></li>\n",(href("%R/stat")),(href("%R/hash-collisions")));
    if( g.perm.Admin ){
      cgi_printf("  <li>%zList of URLs used to access\n"
             "      this repository</a></li>\n",(href("%R/urllist")));
    }
    cgi_printf("  <li>%zList of Artifacts</a></li>\n"
           "  <li>%zList of \"Timewarp\" Check-ins</a></li>\n"
           "  </ul>\n"
           "</li>\n",(href("%R/bloblist")),(href("%R/timewarps")));
  }
  cgi_printf("<li>Help\n"
         "  <ul>\n"
         "  <li>%zWiki Formatting Rules</a></li>\n"
         "  <li>%zMarkdown Formatting Rules</a></li>\n"
         "  <li>%zList of All Commands and Web Pages</a></li>\n"
         "  <li>%zAll \"help\" text on a single page</a></li>\n"
         "  <li>%zFilename suffix to mimetype map</a></li>\n"
         "  </ul></li>\n",(href("%R/wiki_rules")),(href("%R/md_rules")),(href("%R/help")),(href("%R/test-all-help")),(href("%R/mimetype_list")));
  if( g.perm.Admin ){
    cgi_printf("<li>%zAdministration Pages</a>\n"
           "  <ul>\n"
           "  <li>%zPending Moderation Requests</a></li>\n"
           "  <li>%zAdmin log</a></li>\n"
           "  <li>%zStatus of the web-page cache</a></li>\n"
           "  </ul></li>\n",(href("%R/setup")),(href("%R/modreq")),(href("%R/admin_log")),(href("%R/cachestat")));
  }
  cgi_printf("<li>Test Pages\n"
         "  <ul>\n");
  if( g.perm.Admin || db_get_boolean("test_env_enable",0) ){
    cgi_printf("  <li>%zCGI Environment Test</a></li>\n",(href("%R/test_env")));
  }
  if( g.perm.Read ){
    cgi_printf("  <li>%zList of file renames</a></li>\n",(href("%R/test-rename-list")));
  }
  cgi_printf("  <li>%zPage to experiment with the automatic\n"
         "      colors assigned to branch names</a>\n"
         "  <li>%zRandom ASCII-art Captcha image</a></li>\n"
         "  </ul></li>\n"
         "</ul>\n",(href("%R/hash-color-test")),(href("%R/test-captcha")));
  if( !isPopup ){
    style_footer();
  }
}
