#line 1 "..\\src\\forum.c"
/*
** Copyright (c) 2018 D. Richard Hipp
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
** This file contains code used to generate the user forum.
*/
#include "config.h"
#include <assert.h>
#include "forum.h"

/*
** Default to using Markdown markup
*/
#define DEFAULT_FORUM_MIMETYPE  "text/x-markdown"

#if INTERFACE
/*
** Each instance of the following object represents a single message - 
** either the initial post, an edit to a post, a reply, or an edit to
** a reply.
*/
struct ForumEntry {
  int fpid;              /* rid for this entry */
  int fprev;             /* zero if initial entry.  non-zero if an edit */
  int firt;              /* This entry replies to firt */
  int mfirt;             /* Root in-reply-to */
  char *zUuid;           /* Artifact hash */
  ForumEntry *pLeaf;     /* Most recent edit for this entry */
  ForumEntry *pEdit;     /* This entry is an edit of pEditee */
  ForumEntry *pNext;     /* Next in chronological order */
  ForumEntry *pPrev;     /* Previous in chronological order */
  ForumEntry *pDisplay;  /* Next in display order */
  int nIndent;           /* Number of levels of indentation for this entry */
};

/*
** A single instance of the following tracks all entries for a thread.
*/
struct ForumThread {
  ForumEntry *pFirst;    /* First entry in chronological order */
  ForumEntry *pLast;     /* Last entry in chronological order */
  ForumEntry *pDisplay;  /* Entries in display order */
  ForumEntry *pTail;     /* Last on the display list */
};
#endif /* INTERFACE */

/*
** Delete a complete ForumThread and all its entries.
*/
static void forumthread_delete(ForumThread *pThread){
  ForumEntry *pEntry, *pNext;
  for(pEntry=pThread->pFirst; pEntry; pEntry = pNext){
    pNext = pEntry->pNext;
    fossil_free(pEntry->zUuid);
    fossil_free(pEntry);
  }
  fossil_free(pThread);
}

#if 0 /* not used */
/*
** Search a ForumEntry list forwards looking for the entry with fpid
*/
static ForumEntry *forumentry_forward(ForumEntry *p, int fpid){
  while( p && p->fpid!=fpid ) p = p->pNext;
  return p;
}
#endif

/*
** Search backwards for a ForumEntry
*/
static ForumEntry *forumentry_backward(ForumEntry *p, int fpid){
  while( p && p->fpid!=fpid ) p = p->pPrev;
  return p;
}

/*
** Add an entry to the display list
*/
static void forumentry_add_to_display(ForumThread *pThread, ForumEntry *p){
  if( pThread->pDisplay==0 ){
    pThread->pDisplay = p;
  }else{
    pThread->pTail->pDisplay = p;
  }
  pThread->pTail = p;
}

/*
** Extend the display list for pThread by adding all entries that
** reference fpid.  The first such entry will be no earlier then
** entry "p".
*/
static void forumthread_display_order(
  ForumThread *pThread,
  ForumEntry *p,
  int fpid,
  int nIndent
){
  while( p ){
    if( p->fprev==0 && p->mfirt==fpid ){
      p->nIndent = nIndent;
      forumentry_add_to_display(pThread, p);
      forumthread_display_order(pThread, p->pNext, p->fpid, nIndent+1);
    }
    p = p->pNext;
  }
}

/*
** Construct a ForumThread object given the root record id.
*/
static ForumThread *forumthread_create(int froot, int computeHierarchy){
  ForumThread *pThread;
  ForumEntry *pEntry;
  Stmt q;
  pThread = fossil_malloc( sizeof(*pThread) );
  memset(pThread, 0, sizeof(*pThread));
  db_prepare(&q,
     "SELECT fpid, firt, fprev, (SELECT uuid FROM blob WHERE rid=fpid)"
     "  FROM forumpost"
     " WHERE froot=%d ORDER BY fmtime",
     froot
  );
  while( db_step(&q)==SQLITE_ROW ){
    pEntry = fossil_malloc( sizeof(*pEntry) );
    memset(pEntry, 0, sizeof(*pEntry));
    pEntry->fpid = db_column_int(&q, 0);
    pEntry->firt = db_column_int(&q, 1);
    pEntry->fprev = db_column_int(&q, 2);
    pEntry->zUuid = fossil_strdup(db_column_text(&q,3));
    pEntry->mfirt = pEntry->firt;
    pEntry->pPrev = pThread->pLast;
    pEntry->pNext = 0;
    if( pThread->pLast==0 ){
      pThread->pFirst = pEntry;
    }else{
      pThread->pLast->pNext = pEntry;
    }
    pThread->pLast = pEntry;
  }
  db_finalize(&q);

  /* Establish which entries are the latest edit.  After this loop
  ** completes, entries that have non-NULL pLeaf should not be
  ** displayed.
  */
  for(pEntry=pThread->pFirst; pEntry; pEntry=pEntry->pNext){
    if( pEntry->fprev ){
      ForumEntry *pBase = 0, *p;
      p = forumentry_backward(pEntry->pPrev, pEntry->fprev);
      pEntry->pEdit = p;
      while( p ){
        pBase = p;
        p->pLeaf = pEntry;
        p = pBase->pEdit;
      }
      for(p=pEntry->pNext; p; p=p->pNext){
        if( p->mfirt==pEntry->fpid ) p->mfirt = pBase->fpid;
      }
    }
  }

  if( computeHierarchy ){
    /* Compute the hierarchical display order */
    pEntry = pThread->pFirst;
    pEntry->nIndent = 1;
    forumentry_add_to_display(pThread, pEntry);
    forumthread_display_order(pThread, pEntry, pEntry->fpid, 2);
  }

  /* Return the result */
  return pThread;
}

/*
** COMMAND: test-forumthread
**
** Usage: %fossil test-forumthread THREADID
**
** Display a summary of all messages on a thread.
*/
void forumthread_cmd(void){
  int fpid;
  int froot;
  const char *zName;
  ForumThread *pThread;
  ForumEntry *p;

  db_find_and_open_repository(0,0);
  verify_all_options();
  if( g.argc!=3 ) usage("THREADID");
  zName = g.argv[2];
  fpid = symbolic_name_to_rid(zName, "f");
  if( fpid<=0 ){
    fossil_fatal("Unknown or ambiguous forum id: \"%s\"", zName);
  }
  froot = db_int(0, "SELECT froot FROM forumpost WHERE fpid=%d", fpid);
  if( froot==0 ){
    fossil_fatal("Not a forum post: \"%s\"", zName);
  }
  fossil_print("fpid  = %d\n", fpid);
  fossil_print("froot = %d\n", froot);
  pThread = forumthread_create(froot, 1);
  fossil_print("Chronological:\n");
           /*   123456789 123456789 123456789 123456789 123456789  */
  fossil_print("     fpid      firt     fprev     mfirt     pLeaf\n");
  for(p=pThread->pFirst; p; p=p->pNext){
    fossil_print("%9d %9d %9d %9d %9d\n",
       p->fpid, p->firt, p->fprev, p->mfirt, p->pLeaf ? p->pLeaf->fpid : 0);
  }
  fossil_print("\nDisplay\n");
  for(p=pThread->pDisplay; p; p=p->pDisplay){
    fossil_print("%*s", (p->nIndent-1)*3, "");
    if( p->pLeaf ){
      fossil_print("%d->%d\n", p->fpid, p->pLeaf->fpid);
    }else{
      fossil_print("%d\n", p->fpid);
    }
  }
  forumthread_delete(pThread);
}

/*
** Render a forum post for display
*/
void forum_render(
  const char *zTitle,         /* The title.  Might be NULL for no title */
  const char *zMimetype,      /* Mimetype of the message */
  const char *zContent,       /* Content of the message */
  const char *zClass          /* Put in a <div> if not NULL */
){
  if( zClass ){
    cgi_printf("<div class='%s'>\n",(zClass));
  }
  if( zTitle ){
    if( zTitle[0] ){
      cgi_printf("<h1>%h</h1>\n",(zTitle));
    }else{
      cgi_printf("<h1><i>Deleted</i></h1>\n");
    }
  }
  if( zContent && zContent[0] ){
    Blob x;
    blob_init(&x, 0, 0);
    blob_append(&x, zContent, -1);
    wiki_render_by_mimetype(&x, zMimetype);
    blob_reset(&x);
  }else{
    cgi_printf("<i>Deleted</i>\n");
  }
  if( zClass ){
    cgi_printf("</div>\n");
  }
}

/*
** Generate the buttons in the display that allow a forum supervisor to
** mark a user as trusted.  Only do this if:
**
**   (1)  The poster is an individual, not a special user like "anonymous"
**   (2)  The current user has Forum Supervisor privilege
*/
static void generateTrustControls(Manifest *pPost){
  if( !g.perm.AdminForum ) return;
  if( login_is_special(pPost->zUser) ) return;
  cgi_printf("<br>\n"
         "<label><input type=\"checkbox\" name=\"trust\">\n"
         "Trust user \"%h\"\n"
         "so that future posts by \"%h\" do not require moderation.\n"
         "</label>\n"
         "<input type=\"hidden\" name=\"trustuser\" value=\"%h\">\n",(pPost->zUser),(pPost->zUser),(pPost->zUser));
}

/*
** Display all posts in a forum thread in chronological order
*/
static void forum_display_chronological(int froot, int target){
  ForumThread *pThread = forumthread_create(froot, 0);
  ForumEntry *p;
  int notAnon = login_is_individual();
  for(p=pThread->pFirst; p; p=p->pNext){
    char *zDate;
    Manifest *pPost;
    int isPrivate;        /* True for posts awaiting moderation */
    int sameUser;         /* True if author is also the reader */

    pPost = manifest_get(p->fpid, CFTYPE_FORUM, 0);
    if( pPost==0 ) continue;
    if( p->fpid==target ){
      cgi_printf("<div id=\"forum%d\" class=\"forumTime forumSel\">\n",(p->fpid));
    }else if( p->pLeaf!=0 ){
      cgi_printf("<div id=\"forum%d\" class=\"forumTime forumObs\">\n",(p->fpid));
    }else{
      cgi_printf("<div id=\"forum%d\" class=\"forumTime\">\n",(p->fpid));
    }
    if( pPost->zThreadTitle ){
      cgi_printf("<h1>%h</h1>\n",(pPost->zThreadTitle));
    }
    zDate = db_text(0, "SELECT datetime(%.17g)", pPost->rDate);
    cgi_printf("<p>By %h on %h (%d)\n",(pPost->zUser),(zDate),(p->fpid));
    fossil_free(zDate);
    if( p->pEdit ){
      cgi_printf("edit of %z%d</a>\n",(href("%R/forumpost/%S?t=c",p->pEdit->zUuid)),(p->fprev));
    }
    if( p->firt ){
      ForumEntry *pIrt = p->pPrev;
      while( pIrt && pIrt->fpid!=p->firt ) pIrt = pIrt->pPrev;
      if( pIrt ){
        cgi_printf("reply to %z%d</a>\n",(href("%R/forumpost/%S?t=c",pIrt->zUuid)),(p->firt));
      }
    }
    if( p->pLeaf ){
      cgi_printf("updated by %z"
             "%d</a>\n",(href("%R/forumpost/%S?t=c",p->pLeaf->zUuid)),(p->pLeaf->fpid));
    }
    if( g.perm.Debug ){
      cgi_printf("<span class=\"debug\">"
             "<a href=\"%R/artifact/%h\">artifact</a></span>\n",(p->zUuid));
    }
    if( p->fpid!=target ){
      cgi_printf("%z[link]</a>\n",(href("%R/forumpost/%S?t=c",p->zUuid)));
    }
    isPrivate = content_is_private(p->fpid);
    sameUser = notAnon && fossil_strcmp(pPost->zUser, g.zLogin)==0;
    if( isPrivate && !g.perm.ModForum && !sameUser ){
      cgi_printf("<p><span class=\"modpending\">Awaiting Moderator Approval</span></p>\n");
    }else{
      forum_render(0, pPost->zMimetype, pPost->zWiki, 0);
    }
    if( g.perm.WrForum && p->pLeaf==0 ){
      int sameUser = login_is_individual()
                     && fossil_strcmp(pPost->zUser, g.zLogin)==0;
      cgi_printf("<p><form action=\"%R/forumedit\" method=\"POST\">\n"
             "<input type=\"hidden\" name=\"fpid\" value=\"%s\">\n",(p->zUuid));
      if( !isPrivate ){
        /* Reply and Edit are only available if the post has already
        ** been approved */
        cgi_printf("<input type=\"submit\" name=\"reply\" value=\"Reply\">\n");
        if( g.perm.Admin || sameUser ){
          cgi_printf("<input type=\"submit\" name=\"edit\" value=\"Edit\">\n"
                 "<input type=\"submit\" name=\"nullout\" value=\"Delete\">\n");
        }
      }else if( g.perm.ModForum ){
        /* Provide moderators with moderation buttons for posts that
        ** are pending moderation */
        cgi_printf("<input type=\"submit\" name=\"approve\" value=\"Approve\">\n"
               "<input type=\"submit\" name=\"reject\" value=\"Reject\">\n");
        generateTrustControls(pPost);
      }else if( sameUser ){
        /* A post that is pending moderation can be deleted by the
        ** person who originally submitted the post */
        cgi_printf("<input type=\"submit\" name=\"reject\" value=\"Delete\">\n");
      }
      cgi_printf("</form></p>\n");
    }
    manifest_destroy(pPost);
    cgi_printf("</div>\n");
  }
  forumthread_delete(pThread);
}

/*
** Display all messages in a forumthread with indentation.
*/
static int forum_display_hierarchical(int froot, int target){
  ForumThread *pThread;
  ForumEntry *p;
  Manifest *pPost, *pOPost;
  int fpid;
  const char *zUuid;
  char *zDate;
  const char *zSel;
  int notAnon = login_is_individual();

  pThread = forumthread_create(froot, 1);
  for(p=pThread->pFirst; p; p=p->pNext){
    if( p->fpid==target ){
      while( p->pEdit ) p = p->pEdit;
      target = p->fpid;
      break;
    }
  }
  for(p=pThread->pDisplay; p; p=p->pDisplay){
    int isPrivate;         /* True for posts awaiting moderation */
    int sameUser;          /* True if reader is also the poster */
    pOPost = manifest_get(p->fpid, CFTYPE_FORUM, 0);
    if( p->pLeaf ){
      fpid = p->pLeaf->fpid;
      zUuid = p->pLeaf->zUuid;
      pPost = manifest_get(fpid, CFTYPE_FORUM, 0);
    }else{
      fpid = p->fpid;
      zUuid = p->zUuid;
      pPost = pOPost;
    }
    zSel = p->fpid==target ? " forumSel" : "";
    if( p->nIndent==1 ){
      cgi_printf("<div id='forum%d' class='forumHierRoot%s'>\n",(fpid),(zSel));
    }else{
      cgi_printf("<div id='forum%d' class='forumHier%s' "
             "style='margin-left: %dex;'>\n",(fpid),(zSel),((p->nIndent-1)*3));
    }
    pPost = manifest_get(fpid, CFTYPE_FORUM, 0);
    if( pPost==0 ) continue;
    if( pPost->zThreadTitle ){
      cgi_printf("<h1>%h</h1>\n",(pPost->zThreadTitle));
    }
    zDate = db_text(0, "SELECT datetime(%.17g)", pOPost->rDate);
    cgi_printf("<p>By %h on %h\n",(pOPost->zUser),(zDate));
    fossil_free(zDate);
    if( g.perm.Debug ){
      cgi_printf("<span class=\"debug\">"
             "<a href=\"%R/artifact/%h\">(%d)</a></span>\n",(p->zUuid),(p->fpid));
    }
    if( p->pLeaf ){
      zDate = db_text(0, "SELECT datetime(%.17g)", pPost->rDate);
      if( fossil_strcmp(pOPost->zUser,pPost->zUser)==0 ){
        cgi_printf("and edited on %h\n",(zDate));
      }else{
        cgi_printf("as edited by %h on %h\n",(pPost->zUser),(zDate));
      }
      fossil_free(zDate);
      if( g.perm.Debug ){
        cgi_printf("<span class=\"debug\">"
               "<a href=\"%R/artifact/%h\">(%d)</a></span>\n",(p->pLeaf->zUuid),(fpid));
      }
      manifest_destroy(pOPost);
    }
    if( fpid!=target ){
      cgi_printf("%z[link]</a>\n",(href("%R/forumpost/%S",zUuid)));
    }
    isPrivate = content_is_private(fpid);
    sameUser = notAnon && fossil_strcmp(pPost->zUser, g.zLogin)==0;
    if( isPrivate && !g.perm.ModForum && !sameUser ){
      cgi_printf("<p><span class=\"modpending\">Awaiting Moderator Approval</span></p>\n");
    }else{
      forum_render(0, pPost->zMimetype, pPost->zWiki, 0);
    }
    if( g.perm.WrForum ){
      cgi_printf("<p><form action=\"%R/forumedit\" method=\"POST\">\n"
             "<input type=\"hidden\" name=\"fpid\" value=\"%s\">\n",(zUuid));
      if( !isPrivate ){
        /* Reply and Edit are only available if the post has already
        ** been approved */
        cgi_printf("<input type=\"submit\" name=\"reply\" value=\"Reply\">\n");
        if( g.perm.Admin || sameUser ){
          cgi_printf("<input type=\"submit\" name=\"edit\" value=\"Edit\">\n"
                 "<input type=\"submit\" name=\"nullout\" value=\"Delete\">\n");
        }
      }else if( g.perm.ModForum ){
        /* Provide moderators with moderation buttons for posts that
        ** are pending moderation */
        cgi_printf("<input type=\"submit\" name=\"approve\" value=\"Approve\">\n"
               "<input type=\"submit\" name=\"reject\" value=\"Reject\">\n");
        generateTrustControls(pPost);
      }else if( sameUser ){
        /* A post that is pending moderation can be deleted by the
        ** person who originally submitted the post */
        cgi_printf("<input type=\"submit\" name=\"reject\" value=\"Delete\">\n");
      }
      cgi_printf("</form></p>\n");
    }
    manifest_destroy(pPost);
    cgi_printf("</div>\n");
  }
  forumthread_delete(pThread);
  return target;
}

/*
** WEBPAGE: forumpost
**
** Show a single forum posting. The posting is shown in context with
** it's entire thread.  The selected posting is enclosed within
** <div class='forumSel'>...</div>.  Javascript is used to move the
** selected posting into view after the page loads.
**
** Query parameters:
**
**   name=X        REQUIRED.  The hash of the post to display
**   t=MODE        Display mode. MODE is 'c' for chronological or
**                   'h' for hierarchical, or 'a' for automatic.
*/
void forumpost_page(void){
  forumthread_page();
}

/*
** WEBPAGE: forumthread
**
** Show all forum messages associated with a particular message thread.
** The result is basically the same as /forumpost except that none of
** the postings in the thread are selected.
**
** Query parameters:
**
**   name=X        REQUIRED.  The hash of any post of the thread.
**   t=MODE        Display mode. MODE is 'c' for chronological or
**                   'h' for hierarchical, or 'a' for automatic.
*/
void forumthread_page(void){
  int fpid;
  int froot;
  const char *zName = P("name");
  const char *zMode = PD("t","a");
  login_check_credentials();
  if( !g.perm.RdForum ){
    login_needed(g.anon.RdForum);
    return;
  }
  if( zName==0 ){
    webpage_error("Missing \"name=\" query parameter");
  }
  fpid = symbolic_name_to_rid(zName, "f");
  if( fpid<=0 ){
    webpage_error("Unknown or ambiguous forum id: \"%s\"", zName);
  }
  style_header("Forum");
  froot = db_int(0, "SELECT froot FROM forumpost WHERE fpid=%d", fpid);
  if( froot==0 ){
    webpage_error("Not a forum post: \"%s\"", zName);
  }
  if( fossil_strcmp(g.zPath,"forumthread")==0 ) fpid = 0;
  if( zMode[0]=='a' ){
    if( cgi_from_mobile() ){
      zMode = "c";  /* Default to chronological on mobile */
    }else{
      zMode = "h";
    }
  }
  if( zMode[0]=='c' ){
    style_submenu_element("Hierarchical", "%R/%s/%s?t=h", g.zPath, zName);
    forum_display_chronological(froot, fpid);
  }else{
    style_submenu_element("Chronological", "%R/%s/%s?t=c", g.zPath, zName);
    forum_display_hierarchical(froot, fpid);
  }
  style_load_js("forum.js");
  style_footer();
}

/*
** Return true if a forum post should be moderated.
*/
static int forum_need_moderation(void){
  if( P("domod") ) return 1;
  if( g.perm.WrTForum ) return 0;
  if( g.perm.ModForum ) return 0;
  return 1;
}

/*
** Add a new Forum Post artifact to the repository.
**
** Return true if a redirect occurs.
*/
static int forum_post(
  const char *zTitle,          /* Title.  NULL for replies */
  int iInReplyTo,              /* Post replying to.  0 for new threads */
  int iEdit,                   /* Post being edited, or zero for a new post */
  const char *zUser,           /* Username.  NULL means use login name */
  const char *zMimetype,       /* Mimetype of content. */
  const char *zContent         /* Content */
){
  char *zDate;
  char *zI;
  char *zG;
  int iBasis;
  Blob x, cksum, formatCheck, errMsg;
  Manifest *pPost;

  schema_forum();
  if( iInReplyTo==0 && iEdit>0 ){
    iBasis = iEdit;
    iInReplyTo = db_int(0, "SELECT firt FROM forumpost WHERE fpid=%d", iEdit);
  }else{
    iBasis = iInReplyTo;
  }
  webpage_assert( (zTitle==0)+(iInReplyTo==0)==1 );
  blob_init(&x, 0, 0);
  zDate = date_in_standard_format("now");
  blob_appendf(&x, "D %s\n", zDate);
  fossil_free(zDate);
  zG = db_text(0, 
     "SELECT uuid FROM blob, forumpost"
     " WHERE blob.rid==forumpost.froot"
     "   AND forumpost.fpid=%d", iBasis);
  if( zG ){
    blob_appendf(&x, "G %s\n", zG);
    fossil_free(zG);
  }
  if( zTitle ){
    blob_appendf(&x, "H %F\n", zTitle);
  }
  zI = db_text(0, "SELECT uuid FROM blob WHERE rid=%d", iInReplyTo);
  if( zI ){
    blob_appendf(&x, "I %s\n", zI);
    fossil_free(zI);
  }
  if( fossil_strcmp(zMimetype,"text/x-fossil-wiki")!=0 ){
    blob_appendf(&x, "N %s\n", zMimetype);
  }
  if( iEdit>0 ){
    char *zP = db_text(0, "SELECT uuid FROM blob WHERE rid=%d", iEdit);
    if( zP==0 ) webpage_error("missing edit artifact %d", iEdit);
    blob_appendf(&x, "P %s\n", zP);
    fossil_free(zP);
  }
  if( zUser==0 ){
    if( login_is_nobody() ){
      zUser = "anonymous";
    }else{
      zUser = login_name();
    }
  }
  blob_appendf(&x, "U %F\n", zUser);
  blob_appendf(&x, "W %d\n%s\n", strlen(zContent), zContent);
  md5sum_blob(&x, &cksum);
  blob_appendf(&x, "Z %b\n", &cksum);
  blob_reset(&cksum);

  /* Verify that the artifact we are creating is well-formed */
  blob_init(&formatCheck, 0, 0);
  blob_init(&errMsg, 0, 0);
  blob_copy(&formatCheck, &x);
  pPost = manifest_parse(&formatCheck, 0, &errMsg);
  if( pPost==0 ){
    webpage_error("malformed forum post artifact - %s", blob_str(&errMsg));
  }
  webpage_assert( pPost->type==CFTYPE_FORUM );
  manifest_destroy(pPost);

  if( P("dryrun") ){
    cgi_printf("<div class='debug'>\n"
           "This is the artifact that would have been generated:\n"
           "<pre>%h</pre>\n"
           "</div>\n",(blob_str(&x)));
    blob_reset(&x);
    return 0;
  }else{
    int nrid = wiki_put(&x, 0, forum_need_moderation());
    cgi_redirectf("%R/forumpost/%S", rid_to_uuid(nrid));
    return 1;
  }
}

/*
** Paint the form elements for entering a Forum post
*/
static void forum_entry_widget(
  const char *zTitle,
  const char *zMimetype,
  const char *zContent
){
  if( zTitle ){
    cgi_printf("Title: <input type=\"input\" name=\"title\" value=\"%h\" size=\"50\"><br>\n",(zTitle));
  }
  cgi_printf("Markup style:\n");
  mimetype_option_menu(zMimetype);
  cgi_printf("<br><textarea name=\"content\" class=\"wikiedit\" cols=\"80\" "
         "rows=\"25\" wrap=\"virtual\">%h</textarea><br>\n",(zContent));
}

/*
** WEBPAGE: forumnew
** WEBPAGE: forumedit
**
** Start a new thread on the forum or reply to an existing thread.
** But first prompt to see if the user would like to log in.
*/
void forum_page_init(void){
  int isEdit;
  char *zGoto;
  login_check_credentials();
  if( !g.perm.WrForum ){
    login_needed(g.anon.WrForum);
    return;
  }
  if( sqlite3_strglob("*edit*", g.zPath)==0 ){
    zGoto = mprintf("%R/forume2?fpid=%S",PD("fpid",""));
    isEdit = 1;
  }else{
    zGoto = mprintf("%R/forume1");
    isEdit = 0;
  }
  if( login_is_individual() ){
    if( isEdit ){
      forumedit_page();
    }else{
      forumnew_page();
    }
    return;
  }
  style_header("%h As Anonymous?", isEdit ? "Reply" : "Post");
  cgi_printf("<p>You are not logged in.\n"
         "<p><table border=\"0\" cellpadding=\"10\">\n"
         "<tr><td>\n"
         "<form action=\"%s\" method=\"POST\">\n"
         "<input type=\"submit\" value=\"Remain Anonymous\">\n"
         "</form>\n"
         "<td>Post to the forum anonymously\n",(zGoto));
  if( login_self_register_available(0) ){
    cgi_printf("<tr><td>\n"
           "<form action=\"%R/register\" method=\"POST\">\n"
           "<input type=\"hidden\" name=\"g\" value=\"%s\">\n"
           "<input type=\"submit\" value=\"Create An Account\">\n"
           "</form>\n"
           "<td>Create a new account and post using that new account\n",(zGoto));
  }
  cgi_printf("<tr><td>\n"
         "<form action=\"%R/login\" method=\"POST\">\n"
         "<input type=\"hidden\" name=\"g\" value=\"%s\">\n"
         "<input type=\"hidden\" name=\"noanon\" value=\"1\">\n"
         "<input type=\"submit\" value=\"Login\">\n"
         "</form>\n"
         "<td>Log into an existing account\n"
         "</table>\n",(zGoto));
  style_footer();
  fossil_free(zGoto);
}

/*
** Write the "From: USER" line on the webpage.
*/
static void forum_from_line(void){
  if( login_is_nobody() ){
    cgi_printf("From: anonymous<br>\n");
  }else{
    cgi_printf("From: %h<br>\n",(login_name()));
  }
}

/*
** WEBPAGE: forume1
**
** Start a new forum thread.
*/
void forumnew_page(void){
  const char *zTitle = PDT("title","");
  const char *zMimetype = PD("mimetype",DEFAULT_FORUM_MIMETYPE);
  const char *zContent = PDT("content","");
  login_check_credentials();
  if( !g.perm.WrForum ){
    login_needed(g.anon.WrForum);
    return;
  }
  if( P("submit") ){
    if( forum_post(zTitle, 0, 0, 0, zMimetype, zContent) ) return;
  }
  if( P("preview") ){
    cgi_printf("<h1>Preview:</h1>\n");
    forum_render(zTitle, zMimetype, zContent, "forumEdit");
  }
  style_header("New Forum Thread");
  cgi_printf("<form action=\"%R/forume1\" method=\"POST\">\n"
         "<h1>New Thread:</h1>\n");
  forum_from_line();
  forum_entry_widget(zTitle, zMimetype, zContent);
  cgi_printf("<input type=\"submit\" name=\"preview\" value=\"Preview\">\n");
  if( P("preview") ){
    cgi_printf("<input type=\"submit\" name=\"submit\" value=\"Submit\">\n");
  }else{
    cgi_printf("<input type=\"submit\" name=\"submit\" value=\"Submit\" disabled>\n");
  }
  if( g.perm.Debug ){
    /* For the test-forumnew page add these extra debugging controls */
    cgi_printf("<div class=\"debug\">\n"
           "<label><input type=\"checkbox\" name=\"dryrun\" %s> "
           "Dry run</label>\n"
           "<br><label><input type=\"checkbox\" name=\"domod\" %s> "
           "Require moderator approval</label>\n"
           "<br><label><input type=\"checkbox\" name=\"showqp\" %s> "
           "Show query parameters</label>\n"
           "</div>\n",(PCK("dryrun")),(PCK("domod")),(PCK("showqp")));
  }
  cgi_printf("</form>\n");
  style_footer();
}

/*
** WEBPAGE: forume2
**
** Edit an existing forum message.
** Query parameters:
**
**   fpid=X        Hash of the post to be editted.  REQUIRED
*/
void forumedit_page(void){
  int fpid;
  Manifest *pPost = 0;
  const char *zMimetype = 0;
  const char *zContent = 0;
  const char *zTitle = 0;
  int isCsrfSafe;
  int isDelete = 0;

  login_check_credentials();
  if( !g.perm.WrForum ){
    login_needed(g.anon.WrForum);
    return;
  }
  fpid = symbolic_name_to_rid(PD("fpid",""), "f");
  if( fpid<=0 || (pPost = manifest_get(fpid, CFTYPE_FORUM, 0))==0 ){
    webpage_error("Missing or invalid fpid query parameter");
  }
  if( P("cancel") ){
    cgi_redirectf("%R/forumpost/%S",P("fpid"));
    return;
  }
  isCsrfSafe = cgi_csrf_safe(1);
  if( g.perm.ModForum && isCsrfSafe ){
    if( P("approve") ){
      const char *zUserToTrust;
      moderation_approve(fpid);
      if( g.perm.AdminForum
       && PB("trust")
       && (zUserToTrust = P("trustuser"))!=0
      ){
        db_multi_exec("UPDATE user SET cap=cap||'4' "
                      "WHERE login=%Q AND cap NOT GLOB '*4*'",
                      zUserToTrust);
      }
      cgi_redirectf("%R/forumpost/%S",P("fpid"));
      return;
    }
    if( P("reject") ){
      char *zParent = 
        db_text(0,
          "SELECT uuid FROM forumpost, blob"
          " WHERE forumpost.fpid=%d AND blob.rid=forumpost.firt",
          fpid
        );
      moderation_disapprove(fpid);
      if( zParent ){
        cgi_redirectf("%R/forumpost/%S",zParent);
      }else{
        cgi_redirectf("%R/forum");
      }
      return;
    }
  }
  isDelete = P("nullout")!=0;
  if( P("submit") && isCsrfSafe ){
    int done = 1;
    const char *zMimetype = PD("mimetype",DEFAULT_FORUM_MIMETYPE);
    const char *zContent = PDT("content","");
    if( P("reply") ){
      done = forum_post(0, fpid, 0, 0, zMimetype, zContent);
    }else if( P("edit") || isDelete ){
      done = forum_post(P("title"), 0, fpid, 0, zMimetype, zContent);
    }else{
      webpage_error("Missing 'reply' query parameter");
    }
    if( done ) return;
  }
  if( isDelete ){
    zMimetype = "text/x-fossil-wiki";
    zContent = "";
    if( pPost->zThreadTitle ) zTitle = "";
    style_header("Delete %s", zTitle ? "Post" : "Reply");
    cgi_printf("<h1>Original Post:</h1>\n");
    forum_render(pPost->zThreadTitle, pPost->zMimetype, pPost->zWiki,
                 "forumEdit");
    cgi_printf("<h1>Change Into:</h1>\n");
    forum_render(zTitle, zMimetype, zContent,"forumEdit");
    cgi_printf("<form action=\"%R/forume2\" method=\"POST\">\n"
           "<input type=\"hidden\" name=\"fpid\" value=\"%h\">\n"
           "<input type=\"hidden\" name=\"nullout\" value=\"1\">\n"
           "<input type=\"hidden\" name=\"mimetype\" value=\"%h\">\n"
           "<input type=\"hidden\" name=\"content\" value=\"%h\">\n",(P("fpid")),(zMimetype),(zContent));
    if( zTitle ){
      cgi_printf("<input type=\"hidden\" name=\"title\" value=\"%h\">\n",(zTitle));
    }
  }else if( P("edit") ){
    /* Provide an edit to the fpid post */
    zMimetype = P("mimetype");
    zContent = PT("content");
    zTitle = P("title");
    if( zContent==0 ) zContent = fossil_strdup(pPost->zWiki);
    if( zMimetype==0 ) zMimetype = fossil_strdup(pPost->zMimetype);
    if( zTitle==0 && pPost->zThreadTitle!=0 ){
      zTitle = fossil_strdup(pPost->zThreadTitle);
    }
    style_header("Edit %s", zTitle ? "Post" : "Reply");
    cgi_printf("<h1>Original Post:</h1>\n");
    forum_render(pPost->zThreadTitle, pPost->zMimetype, pPost->zWiki,
                 "forumEdit");
    if( P("preview") ){
      cgi_printf("<h1>Preview of Edited Post:</h1>\n");
      forum_render(zTitle, zMimetype, zContent,"forumEdit");
    }
    cgi_printf("<h1>Revised Message:</h1>\n"
           "<form action=\"%R/forume2\" method=\"POST\">\n"
           "<input type=\"hidden\" name=\"fpid\" value=\"%h\">\n"
           "<input type=\"hidden\" name=\"edit\" value=\"1\">\n",(P("fpid")));
    forum_from_line();
    forum_entry_widget(zTitle, zMimetype, zContent);
  }else{
    /* Reply */
    zMimetype = PD("mimetype",DEFAULT_FORUM_MIMETYPE);
    zContent = PDT("content","");
    style_header("Reply");
    cgi_printf("<h1>Replying To:</h1>\n");
    forum_render(0, pPost->zMimetype, pPost->zWiki, "forumEdit");
    if( P("preview") ){
      cgi_printf("<h1>Preview:</h1>\n");
      forum_render(0, zMimetype,zContent, "forumEdit");
    }
    cgi_printf("<h1>Enter Reply:</h1>\n"
           "<form action=\"%R/forume2\" method=\"POST\">\n"
           "<input type=\"hidden\" name=\"fpid\" value=\"%h\">\n"
           "<input type=\"hidden\" name=\"reply\" value=\"1\">\n",(P("fpid")));
    forum_from_line();
    forum_entry_widget(0, zMimetype, zContent);
  }
  if( !isDelete ){
    cgi_printf("<input type=\"submit\" name=\"preview\" value=\"Preview\">\n");
  }
  cgi_printf("<input type=\"submit\" name=\"cancel\" value=\"Cancel\">\n");
  if( P("preview") || isDelete ){
    cgi_printf("<input type=\"submit\" name=\"submit\" value=\"Submit\">\n");
  }
  if( g.perm.Debug ){
    /* For the test-forumnew page add these extra debugging controls */
    cgi_printf("<div class=\"debug\">\n"
           "<label><input type=\"checkbox\" name=\"dryrun\" %s> "
           "Dry run</label>\n"
           "<br><label><input type=\"checkbox\" name=\"domod\" %s> "
           "Require moderator approval</label>\n"
           "<br><label><input type=\"checkbox\" name=\"showqp\" %s> "
           "Show query parameters</label>\n"
           "</div>\n",(PCK("dryrun")),(PCK("domod")),(PCK("showqp")));
  }
  cgi_printf("</form>\n");
  style_footer();
}

/*
** WEBPAGE: forum
**
** The main page for the forum feature.  Show a list of recent forum
** threads.  Also show a search box at the top if search is enabled,
** and a button for creating a new thread, if enabled.
**
** Query parameters:
**
**    n=N             The number of threads to show on each page
**    x=X             Skip the first X threads
*/
void forum_main_page(void){
  Stmt q;
  int iLimit, iOfst, iCnt;
  int srchFlags;
  login_check_credentials();
  srchFlags = search_restrict(SRCH_FORUM);
  if( !g.perm.RdForum ){
    login_needed(g.anon.RdForum);
    return;
  }
  style_header("Forum");
  if( g.perm.WrForum ){
    style_submenu_element("New Thread","%R/forumnew");
  }else{
    /* Can't combine this with previous case using the ternary operator
     * because that causes an error yelling about "non-constant format"
     * with some compilers.  I can't see it, since both expressions have
     * the same format, but I'm no C spec lawyer. */
    style_submenu_element("New Thread","%R/login");
  }
  if( g.perm.ModForum && moderation_needed() ){
    style_submenu_element("Moderation Requests", "%R/modreq");
  }
  if( (srchFlags & SRCH_FORUM)!=0 ){
    if( search_screen(SRCH_FORUM, 0) ){
      style_submenu_element("Recent Threads","%R/forum");
      style_footer();
      return;
    }
  }
  iLimit = atoi(PD("n","25"));
  iOfst = atoi(PD("x","0"));
  iCnt = 0;
  if( db_table_exists("repository","forumpost") ){
    db_prepare(&q,
      "WITH thread(age,duration,cnt,root,last) AS ("
      "  SELECT"
      "    julianday('now') - max(fmtime),"
      "    max(fmtime) - min(fmtime),"
      "    sum(fprev IS NULL),"
      "    froot,"
      "    (SELECT fpid FROM forumpost AS y"
      "      WHERE y.froot=x.froot %s"
      "      ORDER BY y.fmtime DESC LIMIT 1)"
      "  FROM forumpost AS x"
      "  WHERE %s"
      "  GROUP BY froot"
      "  ORDER BY 1 LIMIT %d OFFSET %d"
      ")"
      "SELECT"
      "  thread.age,"                                         /* 0 */
      "  thread.duration,"                                    /* 1 */
      "  thread.cnt,"                                         /* 2 */
      "  blob.uuid,"                                          /* 3 */
      "  substr(event.comment,instr(event.comment,':')+1),"   /* 4 */
      "  thread.last"                                         /* 5 */
      " FROM thread, blob, event"
      " WHERE blob.rid=thread.last"
      "  AND event.objid=thread.last"
      " ORDER BY 1;",
      g.perm.ModForum ? "" : "AND y.fpid NOT IN private" /*safe-for-%s*/,
      g.perm.ModForum ? "true" : "fpid NOT IN private" /*safe-for-%s*/,
      iLimit+1, iOfst
    );
    while( db_step(&q)==SQLITE_ROW ){
      char *zAge = human_readable_age(db_column_double(&q,0));
      int nMsg = db_column_int(&q, 2);
      const char *zUuid = db_column_text(&q, 3);
      const char *zTitle = db_column_text(&q, 4);
      if( iCnt==0 ){
        if( iOfst>0 ){
          cgi_printf("<h1>Threads at least %s old</h1>\n",(zAge));
        }else{
          cgi_printf("<h1>Most recent threads</h1>\n");
        }
        cgi_printf("<div class='forumPosts fileage'><table width=\"100%%\">\n");
        if( iOfst>0 ){
          if( iOfst>iLimit ){
            cgi_printf("<tr><td colspan=\"3\">"
                   "%z"
                   "&uarr; Newer...</a></td></tr>\n",(href("%R/forum?x=%d&n=%d",iOfst-iLimit,iLimit)));
          }else{
            cgi_printf("<tr><td colspan=\"3\">%z"
                   "&uarr; Newer...</a></td></tr>\n",(href("%R/forum?n=%d",iLimit)));
          }
        }
      }
      iCnt++;
      if( iCnt>iLimit ){
        cgi_printf("<tr><td colspan=\"3\">"
               "%z"
               "&darr; Older...</a></td></tr>\n",(href("%R/forum?x=%d&n=%d",iOfst+iLimit,iLimit)));
        fossil_free(zAge);
        break;
      }
      cgi_printf("<tr><td>%h ago</td>\n"
             "<td>%z%h</a></td>\n"
             "<td>",(zAge),(href("%R/forumpost/%S",zUuid)),(zTitle));
      if( g.perm.ModForum && moderation_pending(db_column_int(&q,5)) ){
        cgi_printf("<span class=\"modpending\">"
               "Awaiting Moderator Approval</span><br>\n");
      }
      if( nMsg<2 ){
        cgi_printf("no replies</td>\n");
      }else{
        char *zDuration = human_readable_age(db_column_double(&q,1));
        cgi_printf("%d posts spanning %h</td>\n",(nMsg),(zDuration));
        fossil_free(zDuration);
      }
      cgi_printf("</tr>\n");
      fossil_free(zAge);
    }
    db_finalize(&q);
  }
  if( iCnt>0 ){
    cgi_printf("</table></div>\n");
  }else{
    cgi_printf("<h1>No forum posts found</h1>\n");
  }
  style_footer();
}
