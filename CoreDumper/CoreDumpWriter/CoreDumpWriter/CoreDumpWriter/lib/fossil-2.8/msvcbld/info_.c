#line 1 "..\\src\\info.c"
/*
** Copyright (c) 2007 D. Richard Hipp
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
** This file contains code to implement the "info" command.  The
** "info" command gives command-line access to information about
** the current tree, or a particular artifact or check-in.
*/
#include "config.h"
#include "info.h"
#include <assert.h>

/*
** Return a string (in memory obtained from malloc) holding a
** comma-separated list of tags that apply to check-in with
** record-id rid.  If the "propagatingOnly" flag is true, then only
** show branch tags (tags that propagate to children).
**
** Return NULL if there are no such tags.
*/
char *info_tags_of_checkin(int rid, int propagatingOnly){
  char *zTags;
  zTags = db_text(0, "SELECT group_concat(substr(tagname, 5), ', ')"
                     "  FROM tagxref, tag"
                     " WHERE tagxref.rid=%d AND tagxref.tagtype>%d"
                     "   AND tag.tagid=tagxref.tagid"
                     "   AND tag.tagname GLOB 'sym-*'",
                     rid, propagatingOnly!=0);
  return zTags;
}


/*
** Print common information about a particular record.
**
**     *  The UUID
**     *  The record ID
**     *  mtime and ctime
**     *  who signed it
**
*/
void show_common_info(
  int rid,                   /* The rid for the check-in to display info for */
  const char *zUuidName,     /* Name of the UUID */
  int showComment,           /* True to show the check-in comment */
  int showFamily             /* True to show parents and children */
){
  Stmt q;
  char *zComment = 0;
  char *zTags;
  char *zDate;
  char *zUuid;
  zUuid = db_text(0, "SELECT uuid FROM blob WHERE rid=%d", rid);
  if( zUuid ){
    zDate = db_text(0,
      "SELECT datetime(mtime) || ' UTC' FROM event WHERE objid=%d",
      rid
    );
         /* 01234567890123 */
    fossil_print("%-13s %.40s %s\n", zUuidName, zUuid, zDate ? zDate : "");
    free(zDate);
    if( showComment ){
      zComment = db_text(0,
        "SELECT coalesce(ecomment,comment) || "
        "       ' (user: ' || coalesce(euser,user,'?') || ')' "
        "  FROM event WHERE objid=%d",
        rid
      );
    }
    free(zUuid);
  }
  if( showFamily ){
    db_prepare(&q, "SELECT uuid, pid, isprim FROM plink JOIN blob ON pid=rid "
                   " WHERE cid=%d"
                   " ORDER BY isprim DESC, mtime DESC /*sort*/", rid);
    while( db_step(&q)==SQLITE_ROW ){
      const char *zUuid = db_column_text(&q, 0);
      const char *zType = db_column_int(&q, 2) ? "parent:" : "merged-from:";
      zDate = db_text("",
        "SELECT datetime(mtime) || ' UTC' FROM event WHERE objid=%d",
        db_column_int(&q, 1)
      );
      fossil_print("%-13s %.40s %s\n", zType, zUuid, zDate);
      free(zDate);
    }
    db_finalize(&q);
    db_prepare(&q, "SELECT uuid, cid, isprim FROM plink JOIN blob ON cid=rid "
                   " WHERE pid=%d"
                   " ORDER BY isprim DESC, mtime DESC /*sort*/", rid);
    while( db_step(&q)==SQLITE_ROW ){
      const char *zUuid = db_column_text(&q, 0);
      const char *zType = db_column_int(&q, 2) ? "child:" : "merged-into:";
      zDate = db_text("",
        "SELECT datetime(mtime) || ' UTC' FROM event WHERE objid=%d",
        db_column_int(&q, 1)
      );
      fossil_print("%-13s %.40s %s\n", zType, zUuid, zDate);
      free(zDate);
    }
    db_finalize(&q);
  }
  zTags = info_tags_of_checkin(rid, 0);
  if( zTags && zTags[0] ){
    fossil_print("tags:         %s\n", zTags);
  }
  free(zTags);
  if( zComment ){
    fossil_print("comment:      ");
    comment_print(zComment, 0, 14, -1, get_comment_format());
    free(zComment);
  }
}

/*
** Print information about the URLs used to access a repository and
** checkouts in a repository.
*/
static void extraRepoInfo(void){
  Stmt s;
  db_prepare(&s, "SELECT substr(name,7), date(mtime,'unixepoch')"
                 "  FROM config"
                 " WHERE name GLOB 'ckout:*' ORDER BY mtime DESC");
  while( db_step(&s)==SQLITE_ROW ){
    const char *zName;
    const char *zCkout = db_column_text(&s, 0);
    if( !vfile_top_of_checkout(zCkout) ) continue;
    if( g.localOpen ){
      if( fossil_strcmp(zCkout, g.zLocalRoot)==0 ) continue;
      zName = "alt-root:";
    }else{
      zName = "check-out:";
    }
    fossil_print("%-11s   %-54s %s\n", zName, zCkout,
                 db_column_text(&s, 1));
  }
  db_finalize(&s);
  db_prepare(&s, "SELECT substr(name,9), date(mtime,'unixepoch')"
                 "  FROM config"
                 " WHERE name GLOB 'baseurl:*' ORDER BY mtime DESC");
  while( db_step(&s)==SQLITE_ROW ){
    fossil_print("access-url:   %-54s %s\n", db_column_text(&s, 0),
                 db_column_text(&s, 1));
  }
  db_finalize(&s);
}

/*
** Show the parent project, if any
*/
static void showParentProject(void){
  const char *zParentCode;
  zParentCode = db_get("parent-project-code",0);
  if( zParentCode ){
    fossil_print("derived-from: %s %s\n", zParentCode,
                 db_get("parent-project-name",""));
  }
}


/*
** COMMAND: info
**
** Usage: %fossil info ?VERSION | REPOSITORY_FILENAME? ?OPTIONS?
**
** With no arguments, provide information about the current tree.
** If an argument is specified, provide information about the object
** in the repository of the current tree that the argument refers
** to.  Or if the argument is the name of a repository, show
** information about that repository.
**
** If the argument is a repository name, then the --verbose option shows
** known the check-out locations for that repository and all URLs used
** to access the repository.  The --verbose is (currently) a no-op if
** the argument is the name of a object within the repository.
**
** Use the "finfo" command to get information about a specific
** file in a checkout.
**
** Options:
**
**    -R|--repository FILE       Extract info from repository FILE
**    -v|--verbose               Show extra information about repositories
**
** See also: annotate, artifact, finfo, timeline
*/
void info_cmd(void){
  i64 fsize;
  int verboseFlag = find_option("verbose","v",0)!=0;
  if( !verboseFlag ){
    verboseFlag = find_option("detail","l",0)!=0; /* deprecated */
  }

  if( g.argc==3
   && file_isfile(g.argv[2], ExtFILE)
   && (fsize = file_size(g.argv[2], ExtFILE))>0
   && (fsize&0x1ff)==0
  ){
    db_open_config(0, 0);
    db_open_repository(g.argv[2]);
    db_record_repository_filename(g.argv[2]);
    fossil_print("project-name: %s\n", db_get("project-name", "<unnamed>"));
    fossil_print("project-code: %s\n", db_get("project-code", "<none>"));
    showParentProject();
    extraRepoInfo();
    return;
  }
  db_find_and_open_repository(0,0);
  verify_all_options();
  if( g.argc==2 ){
    int vid;
         /* 012345678901234 */
    db_record_repository_filename(0);
    fossil_print("project-name: %s\n", db_get("project-name", "<unnamed>"));
    if( g.localOpen ){
      fossil_print("repository:   %s\n", db_repository_filename());
      fossil_print("local-root:   %s\n", g.zLocalRoot);
    }
    if( verboseFlag ) extraRepoInfo();
    if( g.zConfigDbName ){
      fossil_print("config-db:    %s\n", g.zConfigDbName);
    }
    fossil_print("project-code: %s\n", db_get("project-code", ""));
    showParentProject();
    vid = g.localOpen ? db_lget_int("checkout", 0) : 0;
    if( vid ){
      show_common_info(vid, "checkout:", 1, 1);
    }
    fossil_print("check-ins:    %d\n",
             db_int(-1, "SELECT count(*) FROM event WHERE type='ci' /*scan*/"));
  }else{
    int rid;
    rid = name_to_rid(g.argv[2]);
    if( rid==0 ){
      fossil_fatal("no such object: %s", g.argv[2]);
    }
    show_common_info(rid, "uuid:", 1, 1);
  }
}

/*
** Show the context graph (immediate parents and children) for
** check-in rid.
*/
void render_checkin_context(int rid, int parentsOnly){
  Blob sql;
  Stmt q;
  blob_zero(&sql);
  blob_append(&sql, timeline_query_for_www(), -1);
  db_multi_exec(
     "CREATE TEMP TABLE IF NOT EXISTS ok(rid INTEGER PRIMARY KEY);"
     "DELETE FROM ok;"
     "INSERT INTO ok VALUES(%d);"
     "INSERT OR IGNORE INTO ok SELECT pid FROM plink WHERE cid=%d;",
     rid, rid
  );
  if( !parentsOnly ){
    db_multi_exec(
      "INSERT OR IGNORE INTO ok SELECT cid FROM plink WHERE pid=%d;", rid
    );
    if( db_table_exists("repository","cherrypick") ){
      db_multi_exec(
        "INSERT OR IGNORE INTO ok "
        "  SELECT parentid FROM cherrypick WHERE childid=%d;"
        "INSERT OR IGNORE INTO ok "
        "  SELECT childid FROM cherrypick WHERE parentid=%d;",
        rid, rid
      );
    }
  }
  blob_append_sql(&sql, " AND event.objid IN ok ORDER BY mtime DESC");
  db_prepare(&q, "%s", blob_sql_text(&sql));
  www_print_timeline(&q,
       TIMELINE_DISJOINT
         |TIMELINE_GRAPH
         |TIMELINE_NOSCROLL
         |TIMELINE_CHPICK,
       0, 0, rid, 0);
  db_finalize(&q);
}

/*
** Show a graph all wiki, tickets, and check-ins that refer to object zUuid.
**
** If zLabel is not NULL and the graph is not empty, then output zLabel as
** a prefix to the graph.
*/
void render_backlink_graph(const char *zUuid, const char *zLabel){
  Blob sql;
  Stmt q;
  char *zGlob;
  zGlob = mprintf("%.5s*", zUuid);
  db_multi_exec(
     "CREATE TEMP TABLE IF NOT EXISTS ok(rid INTEGER PRIMARY KEY);"
     "DELETE FROM ok;"
     "INSERT OR IGNORE INTO ok"
     " SELECT srcid FROM backlink"
     "  WHERE target GLOB %Q"
     "    AND %Q GLOB (target || '*');",
     zGlob, zUuid
  );
  if( !db_exists("SELECT 1 FROM ok") ) return;
  if( zLabel ) cgi_printf("%s", zLabel);
  blob_zero(&sql);
  blob_append(&sql, timeline_query_for_www(), -1);
  blob_append_sql(&sql, " AND event.objid IN ok ORDER BY mtime DESC");
  db_prepare(&q, "%s", blob_sql_text(&sql));
  www_print_timeline(&q, TIMELINE_DISJOINT|TIMELINE_GRAPH|TIMELINE_NOSCROLL,
                     0, 0, 0, 0);
  db_finalize(&q);
}

/*
** WEBPAGE: test-backlinks
**
** Show a timeline of all check-ins and other events that have entries
** in the backlink table.  This is used for testing the rendering
** of the "References" section of the /info page.
*/
void backlink_timeline_page(void){
  Blob sql;
  Stmt q;

  login_check_credentials();
  if( !g.perm.Read || !g.perm.RdTkt || !g.perm.RdWiki ){
    login_needed(g.anon.Read && g.anon.RdTkt && g.anon.RdWiki);
    return;
  }
  style_header("Backlink Timeline (Internal Testing Use)");
  db_multi_exec(
     "CREATE TEMP TABLE IF NOT EXISTS ok(rid INTEGER PRIMARY KEY);"
     "DELETE FROM ok;"
     "INSERT OR IGNORE INTO ok"
     " SELECT blob.rid FROM backlink, blob"
     "  WHERE blob.uuid BETWEEN backlink.target AND (backlink.target||'x')"
  );
  blob_zero(&sql);
  blob_append(&sql, timeline_query_for_www(), -1);
  blob_append_sql(&sql, " AND event.objid IN ok ORDER BY mtime DESC");
  db_prepare(&q, "%s", blob_sql_text(&sql));
  www_print_timeline(&q, TIMELINE_DISJOINT|TIMELINE_GRAPH|TIMELINE_NOSCROLL,
                     0, 0, 0, 0);
  db_finalize(&q);
  style_footer();
}


/*
** Append the difference between artifacts to the output
*/
static void append_diff(
  const char *zFrom,    /* Diff from this artifact */
  const char *zTo,      /*  ... to this artifact */
  u64 diffFlags,        /* Diff formatting flags */
  ReCompiled *pRe       /* Only show change matching this regex */
){
  int fromid;
  int toid;
  Blob from, to, out;
  if( zFrom ){
    fromid = uuid_to_rid(zFrom, 0);
    content_get(fromid, &from);
  }else{
    blob_zero(&from);
  }
  if( zTo ){
    toid = uuid_to_rid(zTo, 0);
    content_get(toid, &to);
  }else{
    blob_zero(&to);
  }
  blob_zero(&out);
  if( diffFlags & DIFF_SIDEBYSIDE ){
    text_diff(&from, &to, &out, pRe, diffFlags | DIFF_HTML | DIFF_NOTTOOBIG);
    cgi_printf("%s\n",(blob_str(&out)));
  }else{
    text_diff(&from, &to, &out, pRe,
           diffFlags | DIFF_LINENO | DIFF_HTML | DIFF_NOTTOOBIG);
    cgi_printf("<pre class=\"udiff\">\n"
           "%s\n"
           "</pre>\n",(blob_str(&out)));
  }
  blob_reset(&from);
  blob_reset(&to);
  blob_reset(&out);
}

/*
** Write a line of web-page output that shows changes that have occurred
** to a file between two check-ins.
*/
static void append_file_change_line(
  const char *zName,    /* Name of the file that has changed */
  const char *zOld,     /* blob.uuid before change.  NULL for added files */
  const char *zNew,     /* blob.uuid after change.  NULL for deletes */
  const char *zOldName, /* Prior name.  NULL if no name change. */
  u64 diffFlags,        /* Flags for text_diff().  Zero to omit diffs */
  ReCompiled *pRe,      /* Only show diffs that match this regex, if not NULL */
  int mperm             /* executable or symlink permission for zNew */
){
  cgi_printf("<p>\n");
  if( !g.perm.Hyperlink ){
    if( zNew==0 ){
      cgi_printf("Deleted %h.\n",(zName));
    }else if( zOld==0 ){
      cgi_printf("Added %h.\n",(zName));
    }else if( zOldName!=0 && fossil_strcmp(zName,zOldName)!=0 ){
      cgi_printf("Name change from %h to %h.\n",(zOldName),(zName));
    }else if( fossil_strcmp(zNew, zOld)==0 ){
      if( mperm==PERM_EXE ){
        cgi_printf("%h became executable.\n",(zName));
      }else if( mperm==PERM_LNK ){
        cgi_printf("%h became a symlink.\n",(zName));
      }else{
        cgi_printf("%h became a regular file.\n",(zName));
      }
    }else{
      cgi_printf("Changes to %h.\n",(zName));
    }
    if( diffFlags ){
      append_diff(zOld, zNew, diffFlags, pRe);
    }
  }else{
    if( zOld && zNew ){
      if( fossil_strcmp(zOld, zNew)!=0 ){
        cgi_printf("Modified %z%h</a>\n"
               "from %z[%S]</a>\n"
               "to %z[%S]</a>.\n",(href("%R/finfo?name=%T&m=%!S",zName,zNew)),(zName),(href("%R/artifact/%!S",zOld)),(zOld),(href("%R/artifact/%!S",zNew)),(zNew));
      }else if( zOldName!=0 && fossil_strcmp(zName,zOldName)!=0 ){
        cgi_printf("Name change\n"
               "from %z%h</a>\n"
               "to %z%h</a>.\n",(href("%R/finfo?name=%T&m=%!S",zOldName,zOld)),(zOldName),(href("%R/finfo?name=%T&m=%!S",zName,zNew)),(zName));
      }else{
        cgi_printf("%z%h</a> became\n",(href("%R/finfo?name=%T&m=%!S",zName,zNew)),(zName));
        if( mperm==PERM_EXE ){
          cgi_printf("executable with contents\n");
        }else if( mperm==PERM_LNK ){
          cgi_printf("a symlink with target\n");
        }else{
          cgi_printf("a regular file with contents\n");
        }
        cgi_printf("%z[%S]</a>.\n",(href("%R/artifact/%!S",zNew)),(zNew));
      }
    }else if( zOld ){
      cgi_printf("Deleted %z%h</a>\n"
             "version %z[%S]</a>.\n",(href("%R/finfo?name=%T&m=%!S",zName,zOld)),(zName),(href("%R/artifact/%!S",zOld)),(zOld));
    }else{
      cgi_printf("Added %z%h</a>\n"
             "version %z[%S]</a>.\n",(href("%R/finfo?name=%T&m=%!S",zName,zNew)),(zName),(href("%R/artifact/%!S",zNew)),(zNew));
    }
    if( diffFlags ){
      append_diff(zOld, zNew, diffFlags, pRe);
    }else if( zOld && zNew && fossil_strcmp(zOld,zNew)!=0 ){
      cgi_printf("&nbsp;&nbsp;\n"
             "%z[diff]</a>\n",(href("%R/fdiff?v1=%!S&v2=%!S",zOld,zNew)));
    }
  }
  cgi_printf("</p>\n");
}

/*
** Generate javascript to enhance HTML diffs.
*/
void append_diff_javascript(int sideBySide){
  if( !sideBySide ) return;
  style_load_one_js_file("sbsdiff.js");
}

/*
** Construct an appropriate diffFlag for text_diff() based on query
** parameters and the to boolean arguments.
*/
u64 construct_diff_flags(int diffType){
  u64 diffFlags = 0;  /* Zero means do not show any diff */
  if( diffType>0 ){
    int x;
    if( diffType==2 ){
      diffFlags = DIFF_SIDEBYSIDE;

      /* "dw" query parameter determines width of each column */
      x = atoi(PD("dw","80"))*(DIFF_CONTEXT_MASK+1);
      if( x<0 || x>DIFF_WIDTH_MASK ) x = DIFF_WIDTH_MASK;
      diffFlags += x;
    }

    if( P("w") ){
      diffFlags |= DIFF_IGNORE_ALLWS;
    }
    /* "dc" query parameter determines lines of context */
    x = atoi(PD("dc","7"));
    if( x<0 || x>DIFF_CONTEXT_MASK ) x = DIFF_CONTEXT_MASK;
    diffFlags += x;

    /* The "noopt" parameter disables diff optimization */
    if( PD("noopt",0)!=0 ) diffFlags |= DIFF_NOOPT;
    diffFlags |= DIFF_STRIP_EOLCR;
  }
  return diffFlags;
}

/*
** WEBPAGE: ci_tags
** URL:    /ci_tags?name=ARTIFACTID
**
** Show all tags and properties for a given check-in.
**
** This information used to be part of the main /ci page, but it is of
** marginal usefulness.  Better to factor it out into a sub-screen.
*/
void ci_tags_page(void){
  const char *zHash;
  int rid;
  Stmt q;
  int cnt = 0;
  Blob sql;

  login_check_credentials();
  if( !g.perm.Read ){ login_needed(g.anon.Read); return; }
  rid = name_to_rid_www("name");
  if( rid==0 ){
    style_header("Check-in Information Error");
    cgi_printf("No such object: %h\n",(g.argv[2]));
    style_footer();
    return;
  }
  zHash = db_text(0, "SELECT uuid FROM blob WHERE rid=%d", rid);
  style_header("Tags and Properties");
  cgi_printf("<h1>Tags and Properties for Check-In "
         "%z%S</a></h1>\n",(href("%R/ci/%!S",zHash)),(zHash));
  db_prepare(&q,
    "SELECT tag.tagid, tagname, "
    "       (SELECT uuid FROM blob WHERE rid=tagxref.srcid AND rid!=%d),"
    "       value, datetime(tagxref.mtime,toLocal()), tagtype,"
    "       (SELECT uuid FROM blob WHERE rid=tagxref.origid AND rid!=%d)"
    "  FROM tagxref JOIN tag ON tagxref.tagid=tag.tagid"
    " WHERE tagxref.rid=%d"
    " ORDER BY tagname /*sort*/", rid, rid, rid
  );
  while( db_step(&q)==SQLITE_ROW ){
    const char *zTagname = db_column_text(&q, 1);
    const char *zSrcUuid = db_column_text(&q, 2);
    const char *zValue = db_column_text(&q, 3);
    const char *zDate = db_column_text(&q, 4);
    int tagtype = db_column_int(&q, 5);
    const char *zOrigUuid = db_column_text(&q, 6);
    cnt++;
    if( cnt==1 ){
      cgi_printf("<ul>\n");
    }
    cgi_printf("<li>\n");
    if( tagtype==0 ){
      cgi_printf("<span class=\"infoTagCancelled\">%h</span> cancelled\n",(zTagname));
    }else if( zValue ){
      cgi_printf("<span class=\"infoTag\">%h=%h</span>\n",(zTagname),(zValue));
    }else {
      cgi_printf("<span class=\"infoTag\">%h</span>\n",(zTagname));
    }
    if( tagtype==2 ){
      if( zOrigUuid && zOrigUuid[0] ){
        cgi_printf("inherited from\n");
        hyperlink_to_uuid(zOrigUuid);
      }else{
        cgi_printf("propagates to descendants\n");
      }
    }
    if( zSrcUuid && zSrcUuid[0] ){
      if( tagtype==0 ){
        cgi_printf("by\n");
      }else{
        cgi_printf("added by\n");
      }
      hyperlink_to_uuid(zSrcUuid);
      cgi_printf("on\n");
      hyperlink_to_date(zDate,0);
    }
    cgi_printf("</li>\n");
  }
  db_finalize(&q);
  if( cnt ){
    cgi_printf("</ul>\n");
  }
  cgi_printf("<div class=\"section\">Context</div>\n");
  db_multi_exec(
     "CREATE TEMP TABLE IF NOT EXISTS ok(rid INTEGER PRIMARY KEY);"
     "DELETE FROM ok;"
     "INSERT INTO ok VALUES(%d);"
     "INSERT OR IGNORE INTO ok "
     " SELECT tagxref.srcid"
     "   FROM tagxref JOIN tag ON tagxref.tagid=tag.tagid"
     "  WHERE tagxref.rid=%d;"
     "INSERT OR IGNORE INTO ok "
     " SELECT tagxref.origid"
     "   FROM tagxref JOIN tag ON tagxref.tagid=tag.tagid"
     "  WHERE tagxref.rid=%d;",
     rid, rid, rid
  );
  db_multi_exec(
    "SELECT tag.tagid, tagname, "
    "       (SELECT uuid FROM blob WHERE rid=tagxref.srcid AND rid!=%d),"
    "       value, datetime(tagxref.mtime,toLocal()), tagtype,"
    "       (SELECT uuid FROM blob WHERE rid=tagxref.origid AND rid!=%d)"
    "  FROM tagxref JOIN tag ON tagxref.tagid=tag.tagid"
    " WHERE tagxref.rid=%d"
    " ORDER BY tagname /*sort*/", rid, rid, rid
  );
  blob_zero(&sql);
  blob_append(&sql, timeline_query_for_www(), -1);
  blob_append_sql(&sql, " AND event.objid IN ok ORDER BY mtime DESC");
  db_prepare(&q, "%s", blob_sql_text(&sql));
  www_print_timeline(&q, TIMELINE_DISJOINT|TIMELINE_GRAPH|TIMELINE_NOSCROLL,
                     0, 0, rid, 0);
  db_finalize(&q);
  style_footer();
}

/*
** WEBPAGE: vinfo
** WEBPAGE: ci
** URL:  /ci/ARTIFACTID
**  OR:  /ci?name=ARTIFACTID
**
** Display information about a particular check-in.  The exact
** same information is shown on the /info page if the name query
** parameter to /info describes a check-in.
**
** The ARTIFACTID can be a unique prefix for the HASH of the check-in,
** or a tag or branch name that identifies the check-in.
*/
void ci_page(void){
  Stmt q1, q2, q3;
  int rid;
  int isLeaf;
  int diffType;        /* 0: no diff,  1: unified,  2: side-by-side */
  u64 diffFlags;       /* Flag parameter for text_diff() */
  const char *zName;   /* Name of the check-in to be displayed */
  const char *zUuid;   /* UUID of zName */
  const char *zParent; /* UUID of the parent check-in (if any) */
  const char *zRe;     /* regex parameter */
  ReCompiled *pRe = 0; /* regex */
  const char *zW;      /* URL param for ignoring whitespace */
  const char *zPage = "vinfo";  /* Page that shows diffs */
  const char *zPageHide = "ci"; /* Page that hides diffs */

  login_check_credentials();
  if( !g.perm.Read ){ login_needed(g.anon.Read); return; }
  zName = P("name");
  rid = name_to_rid_www("name");
  if( rid==0 ){
    style_header("Check-in Information Error");
    cgi_printf("No such object: %h\n",(g.argv[2]));
    style_footer();
    return;
  }
  zRe = P("regex");
  if( zRe ) re_compile(&pRe, zRe, 0);
  zUuid = db_text(0, "SELECT uuid FROM blob WHERE rid=%d", rid);
  zParent = db_text(0,
    "SELECT uuid FROM plink, blob"
    " WHERE plink.cid=%d AND blob.rid=plink.pid AND plink.isprim",
    rid
  );
  isLeaf = !db_exists("SELECT 1 FROM plink WHERE pid=%d", rid);
  db_prepare(&q1,
     "SELECT uuid, datetime(mtime,toLocal()), user, comment,"
     "       datetime(omtime,toLocal()), mtime"
     "  FROM blob, event"
     " WHERE blob.rid=%d"
     "   AND event.objid=%d",
     rid, rid
  );
  
  cookie_link_parameter("diff","diff","2");
  diffType = atoi(PD("diff","2"));
  if( db_step(&q1)==SQLITE_ROW ){
    const char *zUuid = db_column_text(&q1, 0);
    int nUuid = db_column_bytes(&q1, 0);
    char *zEUser, *zEComment;
    const char *zUser;
    const char *zOrigUser;
    const char *zComment;
    const char *zDate;
    const char *zOrigDate;
    const char *zBrName;
    int okWiki = 0;
    Blob wiki_read_links = BLOB_INITIALIZER;
    Blob wiki_add_links = BLOB_INITIALIZER;

    style_header("Check-in [%S]", zUuid);
    login_anonymous_available();
    zEUser = db_text(0,
                   "SELECT value FROM tagxref"
                   " WHERE tagid=%d AND rid=%d AND tagtype>0",
                    TAG_USER, rid);
    zEComment = db_text(0,
                   "SELECT value FROM tagxref WHERE tagid=%d AND rid=%d",
                   TAG_COMMENT, rid);
    zBrName = db_text(0,
                   "SELECT value FROM tagxref WHERE tagid=%d AND rid=%d",
                   TAG_BRANCH, rid);
    zOrigUser = db_column_text(&q1, 2);
    zUser = zEUser ? zEUser : zOrigUser;
    zComment = db_column_text(&q1, 3);
    zDate = db_column_text(&q1,1);
    zOrigDate = db_column_text(&q1, 4);
    if( zOrigDate==0 ) zOrigDate = zDate;
    cgi_printf("<div class=\"section\">Overview</div>\n"
           "<table class=\"label-value\">\n"
           "<tr><th>Comment:</th><td class=\"infoComment\">"
           "%!W</td></tr>\n",(zEComment?zEComment:zComment));

    /* The Download: line */
    if( g.perm.Zip  ){
      char *zPJ = db_get("short-project-name", 0);
      char *zUrl;
      Blob projName;
      int jj;
      if( zPJ==0 ) zPJ = db_get("project-name", "unnamed");
      blob_zero(&projName);
      blob_append(&projName, zPJ, -1);
      blob_trim(&projName);
      zPJ = blob_str(&projName);
      for(jj=0; zPJ[jj]; jj++){
        if( (zPJ[jj]>0 && zPJ[jj]<' ') || strchr("\"*/:<>?\\|", zPJ[jj]) ){
          zPJ[jj] = '_';
        }
      }
      zUrl = mprintf("%R/tarball/%S/%t-%S.tar.gz", zUuid, zPJ, zUuid);
      cgi_printf("<tr><th>Downloads:</th><td>\n"
             "%zTarball</a>\n"
             "| %zZIP archive</a>\n"
             "| %z"
             "SQL archive</a></td></tr>\n",(href("%s",zUrl)),(href("%R/zip/%S/%t-%S.zip",zUuid, zPJ,zUuid)),(href("%R/sqlar/%S/%t-%S.sqlar",zUuid,zPJ,zUuid)));
      fossil_free(zUrl);
      blob_reset(&projName);
    }

    cgi_printf("<tr><th>Timelines:</th><td>\n"
           "  %zfamily</a>\n",(href("%R/timeline?f=%!S&unhide",zUuid)));
    if( zParent ){
      cgi_printf("| %zancestors</a>\n",(href("%R/timeline?p=%!S&unhide",zUuid)));
    }
    if( !isLeaf ){
      cgi_printf("| %zdescendants</a>\n",(href("%R/timeline?d=%!S&unhide",zUuid)));
    }
    if( zParent && !isLeaf ){
      cgi_printf("| %zboth</a>\n",(href("%R/timeline?dp=%!S&unhide",zUuid)));
    }
    db_prepare(&q2,"SELECT substr(tag.tagname,5) FROM tagxref, tag "
                   " WHERE rid=%d AND tagtype>0 "
                   "   AND tag.tagid=tagxref.tagid "
                   "   AND +tag.tagname GLOB 'sym-*'", rid);
    while( db_step(&q2)==SQLITE_ROW ){
      const char *zTagName = db_column_text(&q2, 0);
      if( fossil_strcmp(zTagName,zBrName)==0 ){
        cgi_printf(" | %z%h</a>\n",(href("%R/timeline?r=%T&unhide",zTagName)),(zTagName));
        if( wiki_tagid2("branch",zTagName)!=0 ){
          blob_appendf(&wiki_read_links, " | %z%h</a>",
              href("%R/wiki?name=branch/%h",zTagName), zTagName);
        }else if( g.perm.Write && g.perm.WrWiki ){
          blob_appendf(&wiki_add_links, " | %z%h</a>",
              href("%R/wikiedit?name=branch/%h",zTagName), zTagName);
        }
      }else{
        cgi_printf(" | %z%h</a>\n",(href("%R/timeline?t=%T&unhide",zTagName)),(zTagName));
        if( wiki_tagid2("tag",zTagName)!=0 ){
          blob_appendf(&wiki_read_links, " | %z%h</a>",
              href("%R/wiki?name=tag/%h",zTagName), zTagName);
        }else if( g.perm.Write && g.perm.WrWiki ){
          blob_appendf(&wiki_add_links, " | %z%h</a>",
              href("%R/wikiedit?name=tag/%h",zTagName), zTagName);
        }
      }
    }
    db_finalize(&q2);
    cgi_printf("</td></tr>\n");

    cgi_printf("<tr><th>Files:</th>\n"
           "  <td>\n"
           "    %zfiles</a>\n"
           "  | %zfile ages</a>\n"
           "  | %zfolders</a>\n"
           "  </td>\n"
           "</tr>\n",(href("%R/tree?ci=%!S",zUuid)),(href("%R/fileage?name=%!S",zUuid)),(href("%R/tree?nofiles&type=tree&ci=%!S",zUuid)));

    cgi_printf("<tr><th>%s:</th><td>%.32s<wbr>%s\n",(hname_alg(nUuid)),(zUuid),(zUuid+32));
    if( g.perm.Setup ){
      cgi_printf("(Record ID: %d)\n",(rid));
    }
    cgi_printf("</td></tr>\n"
           "<tr><th>User&nbsp;&amp;&nbsp;Date:</th><td>\n");
    hyperlink_to_user(zUser,zDate," on ");
    hyperlink_to_date(zDate, "</td></tr>");
    if( zEComment ){
      cgi_printf("<tr><th>Original&nbsp;Comment:</th>\n"
             "    <td class=\"infoComment\">%!W</td></tr>\n",(zComment));
    }
    if( fossil_strcmp(zDate, zOrigDate)!=0
     || fossil_strcmp(zOrigUser, zUser)!=0
    ){
      cgi_printf("<tr><th>Original&nbsp;User&nbsp;&amp;&nbsp;Date:</th><td>\n");
      hyperlink_to_user(zOrigUser,zOrigDate," on ");
      hyperlink_to_date(zOrigDate, "</td></tr>");
    }
    if( g.perm.Admin ){
      db_prepare(&q2,
         "SELECT rcvfrom.ipaddr, user.login, datetime(rcvfrom.mtime)"
         "  FROM blob JOIN rcvfrom USING(rcvid) LEFT JOIN user USING(uid)"
         " WHERE blob.rid=%d",
         rid
      );
      if( db_step(&q2)==SQLITE_ROW ){
        const char *zIpAddr = db_column_text(&q2, 0);
        const char *zUser = db_column_text(&q2, 1);
        const char *zDate = db_column_text(&q2, 2);
        if( zUser==0 || zUser[0]==0 ) zUser = "unknown";
        cgi_printf("<tr><th>Received&nbsp;From:</th>\n"
               "<td>%h @ %h on %s</td></tr>\n",(zUser),(zIpAddr),(zDate));
      }
      db_finalize(&q2);
    }

    /* Only show links to read wiki pages if the users can read wiki
    ** and if the wiki pages already exist */
    if( g.perm.RdWiki
     && ((okWiki = wiki_tagid2("checkin",zUuid))!=0 ||
                 blob_size(&wiki_read_links)>0)
     && db_get_boolean("wiki-about",1)
    ){
      const char *zLinks = blob_str(&wiki_read_links);
      cgi_printf("<tr><th>Wiki:</th><td>");
      if( okWiki ){
        cgi_printf("%zthis checkin</a>",(href("%R/wiki?name=checkin/%s",zUuid)));
      }else if( zLinks[0] ){
        zLinks += 3;
      }
      cgi_printf("%s</td></tr>\n",(zLinks));
    }

    /* Only show links to create new wiki pages if the users can write wiki
    ** and if the wiki pages do not already exist */
    if( g.perm.WrWiki
     && g.perm.RdWiki
     && g.perm.Write
     && (blob_size(&wiki_add_links)>0 || !okWiki)
     && db_get_boolean("wiki-about",1)
    ){
      const char *zLinks = blob_str(&wiki_add_links);
      cgi_printf("<tr><th>Add&nbsp;Wiki:</th><td>");
      if( !okWiki ){
        cgi_printf("%zthis checkin</a>",(href("%R/wikiedit?name=checkin/%s",zUuid)));
      }else if( zLinks[0] ){
        zLinks += 3;
      }
      cgi_printf("%s</td></tr>\n",(zLinks));
    }

    if( g.perm.Hyperlink ){
      cgi_printf("<tr><th>Other&nbsp;Links:</th>\n"
             "  <td>\n"
             "  %zmanifest</a>\n"
             "| %ztags</a>\n",(href("%R/artifact/%!S",zUuid)),(href("%R/ci_tags/%!S",zUuid)));
      if( g.perm.Admin ){
        cgi_printf("  | %zmlink table</a>\n",(href("%R/mlink?ci=%!S",zUuid)));
      }
      if( g.anon.Write ){
        cgi_printf("  | %zedit</a>\n",(href("%R/ci_edit?r=%!S",zUuid)));
      }
      cgi_printf("  </td>\n"
             "</tr>\n");
    }
    cgi_printf("</table>\n");
    blob_reset(&wiki_read_links);
    blob_reset(&wiki_add_links);
  }else{
    style_header("Check-in Information");
    login_anonymous_available();
  }
  db_finalize(&q1);
  if( !PB("nowiki") ){
    wiki_render_associated("checkin", zUuid, 0);
  }
  render_backlink_graph(zUuid, "<div class=\"section\">References</div>\n");
  cgi_printf("<div class=\"section\">Context</div>\n");
  render_checkin_context(rid, 0);
  cgi_printf("<div class=\"section\">Changes</div>\n"
         "<div class=\"sectionmenu\">\n");
  diffFlags = construct_diff_flags(diffType);
  zW = (diffFlags&DIFF_IGNORE_ALLWS)?"&w":"";
  if( diffType!=0 ){
    cgi_printf("%z"
           "Hide&nbsp;Diffs</a>\n",(chref("button","%R/%s/%T?diff=0",zPageHide,zName)));
  }
  if( diffType!=1 ){
    cgi_printf("%z"
           "Unified&nbsp;Diffs</a>\n",(chref("button","%R/%s/%T?diff=1%s",zPage,zName,zW)));
  }
  if( diffType!=2 ){
    cgi_printf("%z"
           "Side-by-Side&nbsp;Diffs</a>\n",(chref("button","%R/%s/%T?diff=2%s",zPage,zName,zW)));
  }
  if( diffType!=0 ){
    if( *zW ){
      cgi_printf("%z\n"
             "Show&nbsp;Whitespace&nbsp;Changes</a>\n",(chref("button","%R/%s/%T",zPage,zName)));
    }else{
      cgi_printf("%z\n"
             "Ignore&nbsp;Whitespace</a>\n",(chref("button","%R/%s/%T?w",zPage,zName)));
    }
  }
  if( zParent ){
    cgi_printf("%z\n"
           "Patch</a>\n",(chref("button","%R/vpatch?from=%!S&to=%!S",zParent,zUuid)));
  }
  if( g.perm.Admin ){
    cgi_printf("%zMLink Table</a>\n",(chref("button","%R/mlink?ci=%!S",zUuid)));
  }
 cgi_printf("</div>\n");
  if( pRe ){
    cgi_printf("<p><b>Only differences that match regular expression \"%h\"\n"
           "are shown.</b></p>\n",(zRe));
  }
  db_prepare(&q3,
    "SELECT name,"
    "       mperm,"
    "       (SELECT uuid FROM blob WHERE rid=mlink.pid),"
    "       (SELECT uuid FROM blob WHERE rid=mlink.fid),"
    "       (SELECT name FROM filename WHERE filename.fnid=mlink.pfnid)"
    "  FROM mlink JOIN filename ON filename.fnid=mlink.fnid"
    " WHERE mlink.mid=%d AND NOT mlink.isaux"
    "   AND (mlink.fid>0"
           " OR mlink.fnid NOT IN (SELECT pfnid FROM mlink WHERE mid=%d))"
    " ORDER BY name /*sort*/",
    rid, rid
  );
  while( db_step(&q3)==SQLITE_ROW ){
    const char *zName = db_column_text(&q3,0);
    int mperm = db_column_int(&q3, 1);
    const char *zOld = db_column_text(&q3,2);
    const char *zNew = db_column_text(&q3,3);
    const char *zOldName = db_column_text(&q3, 4);
    append_file_change_line(zName, zOld, zNew, zOldName, diffFlags,pRe,mperm);
  }
  db_finalize(&q3);
  append_diff_javascript(diffType==2);
  cookie_render();
  style_footer();
}

/*
** WEBPAGE: winfo
** URL:  /winfo?name=UUID
**
** Display information about a wiki page.
*/
void winfo_page(void){
  int rid;
  Manifest *pWiki;
  char *zUuid;
  char *zDate;
  Blob wiki;
  int modPending;
  const char *zModAction;
  int tagid;
  int ridNext;

  login_check_credentials();
  if( !g.perm.RdWiki ){ login_needed(g.anon.RdWiki); return; }
  rid = name_to_rid_www("name");
  if( rid==0 || (pWiki = manifest_get(rid, CFTYPE_WIKI, 0))==0 ){
    style_header("Wiki Page Information Error");
    cgi_printf("No such object: %h\n",(P("name")));
    style_footer();
    return;
  }
  if( g.perm.ModWiki && (zModAction = P("modaction"))!=0 ){
    if( strcmp(zModAction,"delete")==0 ){
      moderation_disapprove(rid);
      /*
      ** Next, check if the wiki page still exists; if not, we cannot
      ** redirect to it.
      */
      if( db_exists("SELECT 1 FROM tagxref JOIN tag USING(tagid)"
                    " WHERE rid=%d AND tagname LIKE 'wiki-%%'", rid) ){
        cgi_redirectf("%R/wiki?name=%T", pWiki->zWikiTitle);
        /*NOTREACHED*/
      }else{
        cgi_redirectf("%R/modreq");
        /*NOTREACHED*/
      }
    }
    if( strcmp(zModAction,"approve")==0 ){
      moderation_approve(rid);
    }
  }
  style_header("Update of \"%h\"", pWiki->zWikiTitle);
  zUuid = db_text(0, "SELECT uuid FROM blob WHERE rid=%d", rid);
  zDate = db_text(0, "SELECT datetime(%.17g)", pWiki->rDate);
  style_submenu_element("Raw", "artifact/%s", zUuid);
  style_submenu_element("History", "whistory?name=%t", pWiki->zWikiTitle);
  style_submenu_element("Page", "wiki?name=%t", pWiki->zWikiTitle);
  login_anonymous_available();
  cgi_printf("<div class=\"section\">Overview</div>\n"
         "<p><table class=\"label-value\">\n"
         "<tr><th>Artifact&nbsp;ID:</th>\n"
         "<td>%z%s</a>\n",(href("%R/artifact/%!S",zUuid)),(zUuid));
  if( g.perm.Setup ){
    cgi_printf("(%d)\n",(rid));
  }
  modPending = moderation_pending_www(rid);
  cgi_printf("</td></tr>\n"
         "<tr><th>Page&nbsp;Name:</th>"
         "<td>%z"
         "%h</a></td></tr>\n"
         "<tr><th>Date:</th><td>\n",(href("%R/whistory?name=%h",pWiki->zWikiTitle)),(pWiki->zWikiTitle));
  hyperlink_to_date(zDate, "</td></tr>");
  cgi_printf("<tr><th>Original&nbsp;User:</th><td>\n");
  hyperlink_to_user(pWiki->zUser, zDate, "</td></tr>");
  if( pWiki->zMimetype ){
    cgi_printf("<tr><th>Mimetype:</th><td>%h</td></tr>\n",(pWiki->zMimetype));
  }
  if( pWiki->nParent>0 ){
    int i;
    cgi_printf("<tr><th>Parent%s:</th><td>\n",(pWiki->nParent==1?"":"s"));
    for(i=0; i<pWiki->nParent; i++){
      char *zParent = pWiki->azParent[i];
      cgi_printf("%z%s</a>\n"
             "%z(diff)</a>\n",(href("info/%!S",zParent)),(zParent),(href("%R/wdiff?id=%!S&pid=%!S",zUuid,zParent)));
    }
    cgi_printf("</td></tr>\n");
  }
  tagid = wiki_tagid(pWiki->zWikiTitle);
  if( tagid>0 && (ridNext = wiki_next(tagid, pWiki->rDate))>0 ){
    char *zId = db_text(0, "SELECT uuid FROM blob WHERE rid=%d", ridNext);
    cgi_printf("<tr><th>Next</th>\n"
           "<td>%z%s</a></td>\n",(href("%R/info/%!S",zId)),(zId));
  }
  cgi_printf("</table>\n");

  if( g.perm.ModWiki && modPending ){
    cgi_printf("<div class=\"section\">Moderation</div>\n"
           "<blockquote>\n"
           "<form method=\"POST\" action=\"%R/winfo/%s\">\n"
           "<label><input type=\"radio\" name=\"modaction\" value=\"delete\">\n"
           "Delete this change</label><br />\n"
           "<label><input type=\"radio\" name=\"modaction\" value=\"approve\">\n"
           "Approve this change</label><br />\n"
           "<input type=\"submit\" value=\"Submit\">\n"
           "</form>\n"
           "</blockquote>\n",(zUuid));
  }


  cgi_printf("<div class=\"section\">Content</div>\n");
  blob_init(&wiki, pWiki->zWiki, -1);
  wiki_render_by_mimetype(&wiki, pWiki->zMimetype);
  blob_reset(&wiki);
  manifest_destroy(pWiki);
  style_footer();
}

/*
** Find an check-in based on query parameter zParam and parse its
** manifest.  Return the number of errors.
*/
static Manifest *vdiff_parse_manifest(const char *zParam, int *pRid){
  int rid;

  *pRid = rid = name_to_rid_www(zParam);
  if( rid==0 ){
    const char *z = P(zParam);
    if( z==0 || z[0]==0 ){
      webpage_error("Missing \"%s\" query parameter.", zParam);
    }else{
      webpage_error("No such artifact: \"%s\"", z);
    }
    return 0;
  }
  if( !is_a_version(rid) ){
    webpage_error("Artifact %s is not a check-in.", P(zParam));
    return 0;
  }
  return manifest_get(rid, CFTYPE_MANIFEST, 0);
}

/*
** Output a description of a check-in
*/
static void checkin_description(int rid){
  Stmt q;
  db_prepare(&q,
    "SELECT datetime(mtime), coalesce(euser,user),"
    "       coalesce(ecomment,comment), uuid,"
    "      (SELECT group_concat(substr(tagname,5), ', ') FROM tag, tagxref"
    "        WHERE tagname GLOB 'sym-*' AND tag.tagid=tagxref.tagid"
    "          AND tagxref.rid=blob.rid AND tagxref.tagtype>0)"
    "  FROM event, blob"
    " WHERE event.objid=%d AND type='ci'"
    "   AND blob.rid=%d",
    rid, rid
  );
  while( db_step(&q)==SQLITE_ROW ){
    const char *zDate = db_column_text(&q, 0);
    const char *zUser = db_column_text(&q, 1);
    const char *zUuid = db_column_text(&q, 3);
    const char *zTagList = db_column_text(&q, 4);
    Blob comment;
    int wikiFlags = WIKI_INLINE|WIKI_NOBADLINKS;
    if( db_get_boolean("timeline-block-markup", 0)==0 ){
      wikiFlags |= WIKI_NOBLOCK;
    }
    hyperlink_to_uuid(zUuid);
    blob_zero(&comment);
    db_column_blob(&q, 2, &comment);
    wiki_convert(&comment, 0, wikiFlags);
    blob_reset(&comment);
    cgi_printf("(user:\n");
    hyperlink_to_user(zUser,zDate,",");
    if( zTagList && zTagList[0] && g.perm.Hyperlink ){
      int i;
      const char *z = zTagList;
      Blob links;
      blob_zero(&links);
      while( z && z[0] ){
        for(i=0; z[i] && (z[i]!=',' || z[i+1]!=' '); i++){}
        blob_appendf(&links,
              "%z%#h</a>%.2s",
              href("%R/timeline?r=%#t&nd&c=%t",i,z,zDate), i,z, &z[i]
        );
        if( z[i]==0 ) break;
        z += i+2;
      }
      cgi_printf("tags: %s,\n",(blob_str(&links)));
      blob_reset(&links);
    }else{
      cgi_printf("tags: %h,\n",(zTagList));
    }
    cgi_printf("date:\n");
    hyperlink_to_date(zDate, ")");
    tag_private_status(rid);
  }
  db_finalize(&q);
}


/*
** WEBPAGE: vdiff
** URL: /vdiff?from=TAG&to=TAG
**
** Show the difference between two check-ins identified by the from= and
** to= query parameters.
**
** Query parameters:
**
**   from=TAG        Left side of the comparison
**   to=TAG          Right side of the comparison
**   branch=TAG      Show all changes on a particular branch
**   diff=INTEGER    0: none, 1: unified, 2: side-by-side
**   glob=STRING     only diff files matching this glob
**   dc=N            show N lines of context around each diff
**   w=BOOLEAN       ignore whitespace when computing diffs
**   nohdr           omit the description at the top of the page
**
**
** Show all differences between two check-ins.
*/
void vdiff_page(void){
  int ridFrom, ridTo;
  int diffType = 0;        /* 0: none, 1: unified, 2: side-by-side */
  u64 diffFlags = 0;
  Manifest *pFrom, *pTo;
  ManifestFile *pFileFrom, *pFileTo;
  const char *zBranch;
  const char *zFrom;
  const char *zTo;
  const char *zRe;
  const char *zW;
  const char *zGlob;
  ReCompiled *pRe = 0;
  login_check_credentials();
  if( !g.perm.Read ){ login_needed(g.anon.Read); return; }
  login_anonymous_available();
  cookie_link_parameter("diff","diff","2");
  diffType = atoi(PD("diff","2"));
  cookie_render();
  zRe = P("regex");
  if( zRe ) re_compile(&pRe, zRe, 0);
  zBranch = P("branch");
  if( zBranch && zBranch[0] ){
    cgi_replace_parameter("from", mprintf("root:%s", zBranch));
    cgi_replace_parameter("to", zBranch);
  }
  pTo = vdiff_parse_manifest("to", &ridTo);
  if( pTo==0 ) return;
  pFrom = vdiff_parse_manifest("from", &ridFrom);
  if( pFrom==0 ) return;
  zGlob = P("glob");
  zFrom = P("from");
  zTo = P("to");
  if(zGlob && !*zGlob){
    zGlob = NULL;
  }
  diffFlags = construct_diff_flags(diffType);
  zW = (diffFlags&DIFF_IGNORE_ALLWS)?"&w":"";
  style_submenu_element("Path", "%R/timeline?me=%T&you=%T", zFrom, zTo);
  if( diffType!=0 ){
    style_submenu_element("Hide Diff", "%R/vdiff?from=%T&to=%T&diff=0%s%T%s",
                          zFrom, zTo,
                          zGlob ? "&glob=" : "", zGlob ? zGlob : "", zW);
  }
  if( diffType!=2 ){
    style_submenu_element("Side-by-Side Diff",
                          "%R/vdiff?from=%T&to=%T&diff=2%s%T%s",
                          zFrom, zTo,
                          zGlob ? "&glob=" : "", zGlob ? zGlob : "", zW);
  }
  if( diffType!=1 ) {
    style_submenu_element("Unified Diff",
                          "%R/vdiff?from=%T&to=%T&diff=1%s%T%s",
                          zFrom, zTo,
                          zGlob ? "&glob=" : "", zGlob ? zGlob : "", zW);
  }
  style_submenu_element("Invert",
                        "%R/vdiff?from=%T&to=%T&%s%T%s", zTo, zFrom,
                        zGlob ? "&glob=" : "", zGlob ? zGlob : "", zW);
  if( zGlob ){
    style_submenu_element("Clear glob",
                          "%R/vdiff?from=%T&to=%T&%s", zFrom, zTo, zW);
  }else{
    style_submenu_element("Patch", "%R/vpatch?from=%T&to=%T%s", zFrom, zTo, zW);
  }
  if( diffType!=0 ){
    style_submenu_checkbox("w", "Ignore Whitespace", 0, 0);
  }
  style_header("Check-in Differences");
  if( P("nohdr")==0 ){
    cgi_printf("<h2>Difference From:</h2><blockquote>\n");
    checkin_description(ridFrom);
    cgi_printf("</blockquote><h2>To:</h2><blockquote>\n");
    checkin_description(ridTo);
    cgi_printf("</blockquote>\n");
    if( pRe ){
      cgi_printf("<p><b>Only differences that match regular expression \"%h\"\n"
             "are shown.</b></p>\n",(zRe));
    }
    if( zGlob ){
      cgi_printf("<p><b>Only files matching the glob \"%h\" are shown.</b></p>\n",(zGlob));
    }
   cgi_printf("<hr /><p>\n");
  }

  manifest_file_rewind(pFrom);
  pFileFrom = manifest_file_next(pFrom, 0);
  manifest_file_rewind(pTo);
  pFileTo = manifest_file_next(pTo, 0);
  while( pFileFrom || pFileTo ){
    int cmp;
    if( pFileFrom==0 ){
      cmp = +1;
    }else if( pFileTo==0 ){
      cmp = -1;
    }else{
      cmp = fossil_strcmp(pFileFrom->zName, pFileTo->zName);
    }
    if( cmp<0 ){
      if( !zGlob || sqlite3_strglob(zGlob, pFileFrom->zName)==0 ){
        append_file_change_line(pFileFrom->zName,
                                pFileFrom->zUuid, 0, 0, diffFlags, pRe, 0);
      }
      pFileFrom = manifest_file_next(pFrom, 0);
    }else if( cmp>0 ){
      if( !zGlob || sqlite3_strglob(zGlob, pFileTo->zName)==0 ){
        append_file_change_line(pFileTo->zName,
                                0, pFileTo->zUuid, 0, diffFlags, pRe,
                                manifest_file_mperm(pFileTo));
      }
      pFileTo = manifest_file_next(pTo, 0);
    }else if( fossil_strcmp(pFileFrom->zUuid, pFileTo->zUuid)==0 ){
      pFileFrom = manifest_file_next(pFrom, 0);
      pFileTo = manifest_file_next(pTo, 0);
    }else{
      if(!zGlob || (sqlite3_strglob(zGlob, pFileFrom->zName)==0
                || sqlite3_strglob(zGlob, pFileTo->zName)==0) ){
        append_file_change_line(pFileFrom->zName,
                                pFileFrom->zUuid,
                                pFileTo->zUuid, 0, diffFlags, pRe,
                                manifest_file_mperm(pFileTo));
      }
      pFileFrom = manifest_file_next(pFrom, 0);
      pFileTo = manifest_file_next(pTo, 0);
    }
  }
  manifest_destroy(pFrom);
  manifest_destroy(pTo);
  append_diff_javascript(diffType==2);
  style_footer();
}

#if INTERFACE
/*
** Possible return values from object_description()
*/
#define OBJTYPE_CHECKIN    0x0001
#define OBJTYPE_CONTENT    0x0002
#define OBJTYPE_WIKI       0x0004
#define OBJTYPE_TICKET     0x0008
#define OBJTYPE_ATTACHMENT 0x0010
#define OBJTYPE_EVENT      0x0020
#define OBJTYPE_TAG        0x0040
#define OBJTYPE_SYMLINK    0x0080
#define OBJTYPE_EXE        0x0100
#define OBJTYPE_FORUM      0x0200

/*
** Possible flags for the second parameter to
** object_description()
*/
#define OBJDESC_DETAIL      0x0001   /* more detail */
#endif

/*
** Write a description of an object to the www reply.
**
** If the object is a file then mention:
**
**     * It's artifact ID
**     * All its filenames
**     * The check-in it was part of, with times and users
**
** If the object is a manifest, then mention:
**
**     * It's artifact ID
**     * date of check-in
**     * Comment & user
*/
int object_description(
  int rid,                 /* The artifact ID */
  u32 objdescFlags,        /* Flags to control display */
  Blob *pDownloadName      /* Fill with an appropriate download name */
){
  Stmt q;
  int cnt = 0;
  int nWiki = 0;
  int objType = 0;
  char *zUuid = db_text(0, "SELECT uuid FROM blob WHERE rid=%d", rid);
  int showDetail = (objdescFlags & OBJDESC_DETAIL)!=0;
  char *prevName = 0;

  db_prepare(&q,
    "SELECT filename.name, datetime(event.mtime,toLocal()),"
    "       coalesce(event.ecomment,event.comment),"
    "       coalesce(event.euser,event.user),"
    "       b.uuid, mlink.mperm,"
    "       coalesce((SELECT value FROM tagxref"
                  "  WHERE tagid=%d AND tagtype>0 AND rid=mlink.mid),'trunk'),"
    "       a.size"
    "  FROM mlink, filename, event, blob a, blob b"
    " WHERE filename.fnid=mlink.fnid"
    "   AND event.objid=mlink.mid"
    "   AND a.rid=mlink.fid"
    "   AND b.rid=mlink.mid"
    "   AND mlink.fid=%d"
    "   ORDER BY filename.name, event.mtime /*sort*/",
    TAG_BRANCH, rid
  );
  cgi_printf("<ul>\n");
  while( db_step(&q)==SQLITE_ROW ){
    const char *zName = db_column_text(&q, 0);
    const char *zDate = db_column_text(&q, 1);
    const char *zCom = db_column_text(&q, 2);
    const char *zUser = db_column_text(&q, 3);
    const char *zVers = db_column_text(&q, 4);
    int mPerm = db_column_int(&q, 5);
    const char *zBr = db_column_text(&q, 6);
    int szFile = db_column_int(&q,7);
    int sameFilename = prevName!=0 && fossil_strcmp(zName,prevName)==0;
    if( sameFilename && !showDetail ){
      if( cnt==1 ){
        cgi_printf("%z[more...]</a>\n",(href("%R/whatis/%!S",zUuid)));
      }
      cnt++;
      continue;
    }
    if( !sameFilename ){
      if( prevName && showDetail ) {
        cgi_printf("</ul>\n");
      }
      if( mPerm==PERM_LNK ){
        cgi_printf("<li>Symbolic link\n");
        objType |= OBJTYPE_SYMLINK;
      }else if( mPerm==PERM_EXE ){
        cgi_printf("<li>Executable file\n");
        objType |= OBJTYPE_EXE;
      }else{
        cgi_printf("<li>File\n");
      }
      objType |= OBJTYPE_CONTENT;
      cgi_printf("%z%h</a>\n",(href("%R/finfo?name=%T&m=%!S",zName,zUuid)),(zName));
      tag_private_status(rid);
      if( showDetail ){
        cgi_printf("<ul>\n");
      }
      prevName = fossil_strdup(zName);
    }
    if( showDetail ){
      cgi_printf("<li>\n");
      hyperlink_to_date(zDate,"");
      cgi_printf("&mdash; part of check-in\n");
      hyperlink_to_uuid(zVers);
    }else{
      cgi_printf("&mdash; part of check-in\n");
      hyperlink_to_uuid(zVers);
      cgi_printf("at\n");
      hyperlink_to_date(zDate,"");
    }
    if( zBr && zBr[0] ){
      cgi_printf("on branch %z%h</a>\n",(href("%R/timeline?r=%T",zBr)),(zBr));
    }
    cgi_printf("&mdash; %!W (user:\n",(zCom));
    hyperlink_to_user(zUser,zDate,",");
    cgi_printf("size: %d)\n",(szFile));
    if( g.perm.Hyperlink ){
      cgi_printf("%z\n"
             "[annotate]</a>\n"
             "%z\n"
             "[blame]</a>\n"
             "%z[check-ins&nbsp;using]</a>\n",(href("%R/annotate?filename=%T&checkin=%!S",zName,zVers)),(href("%R/blame?filename=%T&checkin=%!S",zName,zVers)),(href("%R/timeline?n=all&uf=%!S",zUuid)));
    }
    cnt++;
    if( pDownloadName && blob_size(pDownloadName)==0 ){
      blob_append(pDownloadName, zName, -1);
    }
  }
  if( prevName && showDetail ){
    cgi_printf("</ul>\n");
  }
  cgi_printf("</ul>\n");
  free(prevName);
  db_finalize(&q);
  db_prepare(&q,
    "SELECT substr(tagname, 6, 10000), datetime(event.mtime, toLocal()),"
    "       coalesce(event.euser, event.user)"
    "  FROM tagxref, tag, event"
    " WHERE tagxref.rid=%d"
    "   AND tag.tagid=tagxref.tagid"
    "   AND tag.tagname LIKE 'wiki-%%'"
    "   AND event.objid=tagxref.rid",
    rid
  );
  while( db_step(&q)==SQLITE_ROW ){
    const char *zPagename = db_column_text(&q, 0);
    const char *zDate = db_column_text(&q, 1);
    const char *zUser = db_column_text(&q, 2);
    if( cnt>0 ){
      cgi_printf("Also wiki page\n");
    }else{
      cgi_printf("Wiki page\n");
    }
    objType |= OBJTYPE_WIKI;
    cgi_printf("[%z%h</a>] by\n",(href("%R/wiki?name=%t",zPagename)),(zPagename));
    hyperlink_to_user(zUser,zDate," on");
    hyperlink_to_date(zDate,".");
    nWiki++;
    cnt++;
    if( pDownloadName && blob_size(pDownloadName)==0 ){
      blob_appendf(pDownloadName, "%s.txt", zPagename);
    }
  }
  db_finalize(&q);
  if( nWiki==0 ){
    db_prepare(&q,
      "SELECT datetime(mtime, toLocal()), user, comment, type, uuid, tagid"
      "  FROM event, blob"
      " WHERE event.objid=%d"
      "   AND blob.rid=%d",
      rid, rid
    );
    while( db_step(&q)==SQLITE_ROW ){
      const char *zDate = db_column_text(&q, 0);
      const char *zUser = db_column_text(&q, 1);
      const char *zCom = db_column_text(&q, 2);
      const char *zType = db_column_text(&q, 3);
      const char *zUuid = db_column_text(&q, 4);
      int eventTagId = db_column_int(&q, 5);
      if( cnt>0 ){
        cgi_printf("Also\n");
      }
      if( zType[0]=='w' ){
        cgi_printf("Wiki edit\n");
        objType |= OBJTYPE_WIKI;
      }else if( zType[0]=='t' ){
        cgi_printf("Ticket change\n");
        objType |= OBJTYPE_TICKET;
      }else if( zType[0]=='c' ){
        cgi_printf("Manifest of check-in\n");
        objType |= OBJTYPE_CHECKIN;
      }else if( zType[0]=='e' ){
        if( eventTagId != 0) {
          cgi_printf("Instance of technote\n");
          objType |= OBJTYPE_EVENT;
          hyperlink_to_event_tagid(db_column_int(&q, 5));
        }else{
          cgi_printf("Attachment to technote\n");
        }
      }else if( zType[0]=='f' ){
        objType |= OBJTYPE_FORUM;
        cgi_printf("Forum post\n");
      }else{
        cgi_printf("Tag referencing\n");
      }
      if( zType[0]!='e' || eventTagId == 0){
        hyperlink_to_uuid(zUuid);
      }
      cgi_printf("- %!W by\n",(zCom));
      hyperlink_to_user(zUser,zDate," on");
      hyperlink_to_date(zDate, ".");
      if( pDownloadName && blob_size(pDownloadName)==0 ){
        blob_appendf(pDownloadName, "%S.txt", zUuid);
      }
      tag_private_status(rid);
      cnt++;
    }
    db_finalize(&q);
  }
  db_prepare(&q,
    "SELECT target, filename, datetime(mtime, toLocal()), user, src"
    "  FROM attachment"
    " WHERE src=(SELECT uuid FROM blob WHERE rid=%d)"
    " ORDER BY mtime DESC /*sort*/",
    rid
  );
  while( db_step(&q)==SQLITE_ROW ){
    const char *zTarget = db_column_text(&q, 0);
    const char *zFilename = db_column_text(&q, 1);
    const char *zDate = db_column_text(&q, 2);
    const char *zUser = db_column_text(&q, 3);
    /* const char *zSrc = db_column_text(&q, 4); */
    if( cnt>0 ){
      cgi_printf("Also attachment \"%h\" to\n",(zFilename));
    }else{
      cgi_printf("Attachment \"%h\" to\n",(zFilename));
    }
    objType |= OBJTYPE_ATTACHMENT;
    if( fossil_is_uuid(zTarget) ){
      if ( db_exists("SELECT 1 FROM tag WHERE tagname='tkt-%q'",
            zTarget)
      ){
        if( g.perm.Hyperlink && g.anon.RdTkt ){
          cgi_printf("ticket [%z%S</a>]\n",(href("%R/tktview?name=%!S",zTarget)),(zTarget));
        }else{
          cgi_printf("ticket [%S]\n",(zTarget));
        }
      }else if( db_exists("SELECT 1 FROM tag WHERE tagname='event-%q'",
            zTarget)
      ){
        if( g.perm.Hyperlink && g.anon.RdWiki ){
          cgi_printf("tech note [%z%S</a>]\n",(href("%R/technote/%h",zTarget)),(zTarget));
        }else{
          cgi_printf("tech note [%S]\n",(zTarget));
        }
      }else{
        if( g.perm.Hyperlink && g.anon.RdWiki ){
          cgi_printf("wiki page [%z%h</a>]\n",(href("%R/wiki?name=%t",zTarget)),(zTarget));
        }else{
          cgi_printf("wiki page [%h]\n",(zTarget));
        }
      }
    }else{
      if( g.perm.Hyperlink && g.anon.RdWiki ){
        cgi_printf("wiki page [%z%h</a>]\n",(href("%R/wiki?name=%t",zTarget)),(zTarget));
      }else{
        cgi_printf("wiki page [%h]\n",(zTarget));
      }
    }
    cgi_printf("added by\n");
    hyperlink_to_user(zUser,zDate," on");
    hyperlink_to_date(zDate,".");
    cnt++;
    if( pDownloadName && blob_size(pDownloadName)==0 ){
      blob_append(pDownloadName, zFilename, -1);
    }
    tag_private_status(rid);
  }
  db_finalize(&q);
  if( db_exists("SELECT 1 FROM tagxref WHERE rid=%d AND tagid=%d",
                rid, TAG_CLUSTER) ){
    cgi_printf("Cluster\n");
    cnt++;
  }
  if( cnt==0 ){
    cgi_printf("Unrecognized artifact\n");
    if( pDownloadName && blob_size(pDownloadName)==0 ){
      blob_appendf(pDownloadName, "%S.txt", zUuid);
    }
    tag_private_status(rid);
  }
  return objType;
}


/*
** WEBPAGE: fdiff
** URL: fdiff?v1=UUID&v2=UUID
**
** Two arguments, v1 and v2, identify the artifacts to be diffed.
** Show diff side by side unless sbs is 0.  Generate plain text if
** "patch" is present, otherwise generate "pretty" HTML.
**
** Alternative URL:  fdiff?from=filename1&to=filename2&ci=checkin
**
** If the "from" and "to" query parameters are both present, then they are
** the names of two files within the check-in "ci" that are diffed.  If the
** "ci" parameter is omitted, then the most recent check-in ("tip") is
** used.
**
** Additional parameters:
**
**      dc=N             Show N lines of context around each diff
**      patch            Use the patch diff format
**      regex=REGEX      Only show differences that match REGEX
**      sbs=BOOLEAN      Turn side-by-side diffs on and off (default: on)
**      verbose=BOOLEAN  Show more detail when describing artifacts
**      w=BOOLEAN        Ignore whitespace
*/
void diff_page(void){
  int v1, v2;
  int isPatch = P("patch")!=0;
  int diffType;          /* 0: none, 1: unified,  2: side-by-side */
  char *zV1;
  char *zV2;
  const char *zRe;
  ReCompiled *pRe = 0;
  u64 diffFlags;
  u32 objdescFlags = 0;
  int verbose = PB("verbose");

  login_check_credentials();
  if( !g.perm.Read ){ login_needed(g.anon.Read); return; }
  cookie_link_parameter("diff","diff","2");
  diffType = atoi(PD("diff","2"));
  cookie_render();
  if( P("from") && P("to") ){
    v1 = artifact_from_ci_and_filename(0, "from");
    v2 = artifact_from_ci_and_filename(0, "to");
  }else{
    Stmt q;
    v1 = name_to_rid_www("v1");
    v2 = name_to_rid_www("v2");

    /* If the two file versions being compared both have the same
    ** filename, then offer an "Annotate" link that constructs an
    ** annotation between those version. */
    db_prepare(&q,
      "SELECT (SELECT substr(uuid,1,20) FROM blob WHERE rid=a.mid),"
      "       (SELECT substr(uuid,1,20) FROM blob WHERE rid=b.mid),"
      "       (SELECT name FROM filename WHERE filename.fnid=a.fnid)"
      "  FROM mlink a, event ea, mlink b, event eb"
      " WHERE a.fid=%d"
      "   AND b.fid=%d"
      "   AND a.fnid=b.fnid"
      "   AND a.fid!=a.pid"
      "   AND b.fid!=b.pid"
      "   AND ea.objid=a.mid"
      "   AND eb.objid=b.mid"
      " ORDER BY ea.mtime ASC, eb.mtime ASC",
      v1, v2
    );
    if( db_step(&q)==SQLITE_ROW ){
      const char *zCkin = db_column_text(&q, 0);
      const char *zOrig = db_column_text(&q, 1);
      const char *zFN = db_column_text(&q, 2);
      style_submenu_element("Annotate",
        "%R/annotate?origin=%s&checkin=%s&filename=%T",
        zOrig, zCkin, zFN);
    }
    db_finalize(&q);
  }
  if( v1==0 || v2==0 ) fossil_redirect_home();
  zRe = P("regex");
  if( zRe ) re_compile(&pRe, zRe, 0);
  if( verbose ) objdescFlags |= OBJDESC_DETAIL;
  if( isPatch ){
    Blob c1, c2, *pOut;
    pOut = cgi_output_blob();
    cgi_set_content_type("text/plain");
    diffFlags = 4;
    content_get(v1, &c1);
    content_get(v2, &c2);
    text_diff(&c1, &c2, pOut, pRe, diffFlags);
    blob_reset(&c1);
    blob_reset(&c2);
    return;
  }

  zV1 = db_text(0, "SELECT uuid FROM blob WHERE rid=%d", v1);
  zV2 = db_text(0, "SELECT uuid FROM blob WHERE rid=%d", v2);
  diffFlags = construct_diff_flags(diffType) | DIFF_HTML;

  style_header("Diff");
  style_submenu_checkbox("w", "Ignore Whitespace", 0, 0);
  if( diffType==2 ){
    style_submenu_element("Unified Diff", "%R/fdiff?v1=%T&v2=%T&diff=1",
                           P("v1"), P("v2"));
  }else{
    style_submenu_element("Side-by-side Diff", "%R/fdiff?v1=%T&v2=%T&diff=2",
                           P("v1"), P("v2"));
  }
  style_submenu_checkbox("verbose", "Verbose", 0, 0);
  style_submenu_element("Patch", "%R/fdiff?v1=%T&v2=%T&patch",
                        P("v1"), P("v2"));

  if( P("smhdr")!=0 ){
    cgi_printf("<h2>Differences From Artifact\n"
           "%z[%S]</a> To\n"
           "%z[%S]</a>.</h2>\n",(href("%R/artifact/%!S",zV1)),(zV1),(href("%R/artifact/%!S",zV2)),(zV2));
  }else{
    cgi_printf("<h2>Differences From\n"
           "Artifact %z[%S]</a>:</h2>\n",(href("%R/artifact/%!S",zV1)),(zV1));
    object_description(v1, objdescFlags, 0);
    cgi_printf("<h2>To Artifact %z[%S]</a>:</h2>\n",(href("%R/artifact/%!S",zV2)),(zV2));
    object_description(v2, objdescFlags, 0);
  }
  if( pRe ){
    cgi_printf("<b>Only differences that match regular expression \"%h\"\n"
           "are shown.</b>\n",(zRe));
  }
  cgi_printf("<hr />\n");
  append_diff(zV1, zV2, diffFlags, pRe);
  append_diff_javascript(diffType);
  style_footer();
}

/*
** WEBPAGE: raw
** URL: /raw?name=ARTIFACTID&m=TYPE
** URL: /raw?ci=BRANCH&filename=NAME
**
** Return the uninterpreted content of an artifact.  Used primarily
** to view artifacts that are images.
*/
void rawartifact_page(void){
  int rid = 0;
  char *zUuid;
  const char *zMime;
  Blob content;

  if( P("ci") && P("filename") ){
    rid = artifact_from_ci_and_filename(0, 0);
  }
  if( rid==0 ){
    rid = name_to_rid_www("name");
  }
  login_check_credentials();
  if( !g.perm.Read ){ login_needed(g.anon.Read); return; }
  if( rid==0 ) fossil_redirect_home();
  zUuid = db_text(0, "SELECT uuid FROM blob WHERE rid=%d", rid);
  if( fossil_strcmp(P("name"), zUuid)==0 && login_is_nobody() ){
    g.isConst = 1;
  }
  free(zUuid);
  zMime = P("m");
  if( zMime==0 ){
    char *zFName = db_text(0, "SELECT filename.name FROM mlink, filename"
                              " WHERE mlink.fid=%d"
                              "   AND filename.fnid=mlink.fnid", rid);
    if( !zFName ){
      /* Look also at the attachment table */
      zFName = db_text(0, "SELECT attachment.filename FROM attachment, blob"
                          " WHERE blob.rid=%d"
                          "   AND attachment.src=blob.uuid", rid);
    }
    if( zFName ) zMime = mimetype_from_name(zFName);
    if( zMime==0 ) zMime = "application/x-fossil-artifact";
  }
  content_get(rid, &content);
  cgi_set_content_type(zMime);
  cgi_set_content(&content);
}

/*
** Render a hex dump of a file.
*/
static void hexdump(Blob *pBlob){
  const unsigned char *x;
  int n, i, j, k;
  char zLine[100];
  static const char zHex[] = "0123456789abcdef";

  x = (const unsigned char*)blob_buffer(pBlob);
  n = blob_size(pBlob);
  for(i=0; i<n; i+=16){
    j = 0;
    zLine[0] = zHex[(i>>24)&0xf];
    zLine[1] = zHex[(i>>16)&0xf];
    zLine[2] = zHex[(i>>8)&0xf];
    zLine[3] = zHex[i&0xf];
    zLine[4] = ':';
    sqlite3_snprintf(sizeof(zLine), zLine, "%04x: ", i);
    for(j=0; j<16; j++){
      k = 5+j*3;
      zLine[k] = ' ';
      if( i+j<n ){
        unsigned char c = x[i+j];
        zLine[k+1] = zHex[c>>4];
        zLine[k+2] = zHex[c&0xf];
      }else{
        zLine[k+1] = ' ';
        zLine[k+2] = ' ';
      }
    }
    zLine[53] = ' ';
    zLine[54] = ' ';
    for(j=0; j<16; j++){
      k = j+55;
      if( i+j<n ){
        unsigned char c = x[i+j];
        if( c>=0x20 && c<=0x7e ){
          zLine[k] = c;
        }else{
          zLine[k] = '.';
        }
      }else{
        zLine[k] = 0;
      }
    }
    zLine[71] = 0;
    cgi_printf("%h\n",(zLine));
  }
}

/*
** WEBPAGE: hexdump
** URL: /hexdump?name=ARTIFACTID
**
** Show the complete content of a file identified by ARTIFACTID
** as preformatted text.
**
** Other parameters:
**
**     verbose              Show more detail when describing the object
*/
void hexdump_page(void){
  int rid;
  Blob content;
  Blob downloadName;
  char *zUuid;
  u32 objdescFlags = 0;

  rid = name_to_rid_www("name");
  login_check_credentials();
  if( !g.perm.Read ){ login_needed(g.anon.Read); return; }
  if( rid==0 ) fossil_redirect_home();
  if( g.perm.Admin ){
    const char *zUuid = db_text("", "SELECT uuid FROM blob WHERE rid=%d", rid);
    if( db_exists("SELECT 1 FROM shun WHERE uuid=%Q", zUuid) ){
      style_submenu_element("Unshun", "%s/shun?accept=%s&sub=1#delshun",
            g.zTop, zUuid);
    }else{
      style_submenu_element("Shun", "%s/shun?shun=%s#addshun", g.zTop, zUuid);
    }
  }
  style_header("Hex Artifact Content");
  zUuid = db_text("?","SELECT uuid FROM blob WHERE rid=%d", rid);
  if( g.perm.Setup ){
    cgi_printf("<h2>Artifact %s (%d):</h2>\n",(zUuid),(rid));
  }else{
    cgi_printf("<h2>Artifact %s:</h2>\n",(zUuid));
  }
  blob_zero(&downloadName);
  if( P("verbose")!=0 ) objdescFlags |= OBJDESC_DETAIL;
  object_description(rid, objdescFlags, &downloadName);
  style_submenu_element("Download", "%s/raw/%T?name=%s",
        g.zTop, blob_str(&downloadName), zUuid);
  cgi_printf("<hr />\n");
  content_get(rid, &content);
  cgi_printf("<blockquote><pre>\n");
  hexdump(&content);
  cgi_printf("</pre></blockquote>\n");
  style_footer();
}

/*
** Look for "ci" and "filename" query parameters.  If found, try to
** use them to extract the record ID of an artifact for the file.
**
** Also look for "fn" as an alias for "filename".  If either "filename"
** or "fn" is present but "ci" is missing, use "tip" as a default value
** for "ci".
**
** If zNameParam is not NULL, this use that parameter as the filename
** rather than "fn" or "filename".
**
** If pUrl is not NULL, then record the "ci" and "filename" values in
** pUrl.
**
** At least one of pUrl or zNameParam must be NULL.
*/
int artifact_from_ci_and_filename(HQuery *pUrl, const char *zNameParam){
  const char *zFilename;
  const char *zCI;
  int cirid;
  Manifest *pManifest;
  ManifestFile *pFile;

  if( zNameParam ){
    zFilename = P(zNameParam);
  }else{
    zFilename = P("filename");
    if( zFilename==0 ){
      zFilename = P("fn");
    }
  }
  if( zFilename==0 ) return 0;

  zCI = P("ci");
  cirid = name_to_typed_rid(zCI ? zCI : "tip", "ci");
  if( cirid<=0 ) return 0;
  pManifest = manifest_get(cirid, CFTYPE_MANIFEST, 0);
  if( pManifest==0 ) return 0;
  manifest_file_rewind(pManifest);
  while( (pFile = manifest_file_next(pManifest,0))!=0 ){
    if( fossil_strcmp(zFilename, pFile->zName)==0 ){
      int rid = db_int(0, "SELECT rid FROM blob WHERE uuid=%Q", pFile->zUuid);
      manifest_destroy(pManifest);
      if( pUrl ){
        assert( zNameParam==0 );
        url_add_parameter(pUrl, "fn", zFilename);
        if( zCI ) url_add_parameter(pUrl, "ci", zCI);
      }
      return rid;
    }
  }
  manifest_destroy(pManifest);
  return 0;
}

/*
** The "z" argument is a string that contains the text of a source code
** file.  This routine appends that text to the HTTP reply with line numbering.
**
** zLn is the ?ln= parameter for the HTTP query.  If there is an argument,
** then highlight that line number and scroll to it once the page loads.
** If there are two line numbers, highlight the range of lines.
** Multiple ranges can be highlighed by adding additional line numbers
** separated by a non-digit character (also not one of [-,.]).
*/
void output_text_with_line_numbers(
  const char *z,
  const char *zLn
){
  int iStart, iEnd;    /* Start and end of region to highlight */
  int n = 0;           /* Current line number */
  int i = 0;           /* Loop index */
  int iTop = 0;        /* Scroll so that this line is on top of screen. */
  Stmt q;

  iStart = iEnd = atoi(zLn);
  db_multi_exec(
    "CREATE TEMP TABLE lnos(iStart INTEGER PRIMARY KEY, iEnd INTEGER)");
  if( iStart>0 ){
    do{
      while( fossil_isdigit(zLn[i]) ) i++;
      if( zLn[i]==',' || zLn[i]=='-' || zLn[i]=='.' ){
        i++;
        while( zLn[i]=='.' ){ i++; }
        iEnd = atoi(&zLn[i]);
        while( fossil_isdigit(zLn[i]) ) i++;
      }
      while( fossil_isdigit(zLn[i]) ) i++;
      if( iEnd<iStart ) iEnd = iStart;
      db_multi_exec(
        "INSERT OR REPLACE INTO lnos VALUES(%d,%d)", iStart, iEnd
      );
      iStart = iEnd = atoi(&zLn[i++]);
    }while( zLn[i] && iStart && iEnd );
  }
  db_prepare(&q, "SELECT min(iStart), max(iEnd) FROM lnos");
  if( db_step(&q)==SQLITE_ROW ){
    iStart = db_column_int(&q, 0);
    iEnd = db_column_int(&q, 1);
    iTop = iStart - 15 + (iEnd-iStart)/4;
    if( iTop>iStart - 2 ) iTop = iStart-2;
  }
  db_finalize(&q);
  cgi_printf("<pre>\n");
  while( z[0] ){
    n++;
    db_prepare(&q,
      "SELECT min(iStart), max(iEnd) FROM lnos"
      " WHERE iStart <= %d AND iEnd >= %d", n, n);
    if( db_step(&q)==SQLITE_ROW ){
      iStart = db_column_int(&q, 0);
      iEnd = db_column_int(&q, 1);
    }
    db_finalize(&q);
    for(i=0; z[i] && z[i]!='\n'; i++){}
    if( n==iTop ) cgi_append_content("<span id=\"scrollToMe\">", -1);
    if( n==iStart ){
      cgi_append_content("<div class=\"selectedText\">",-1);
    }
    cgi_printf("%6d  ", n);
    if( i>0 ){
      char *zHtml = htmlize(z, i);
      cgi_append_content(zHtml, -1);
      fossil_free(zHtml);
    }
    if( n==iTop ) cgi_append_content("</span>", -1);
    if( n==iEnd ) cgi_append_content("</div>", -1);
    else cgi_append_content("\n", 1);
    z += i;
    if( z[0]=='\n' ) z++;
  }
  if( n<iEnd ) cgi_printf("</div>");
  cgi_printf("</pre>\n");
  if( db_int(0, "SELECT EXISTS(SELECT 1 FROM lnos)") ){
    style_load_one_js_file("scroll.js");
  }
}


/*
** WEBPAGE: artifact
** WEBPAGE: file
** WEBPAGE: whatis
**
** Typical usage:
**
**    /artifact/HASH
**    /whatis/HASH
**    /file/NAME
**
** Additional query parameters:
**
**   ln              - show line numbers
**   ln=N            - highlight line number N
**   ln=M-N          - highlight lines M through N inclusive
**   ln=M-N+Y-Z      - highlight lines M through N and Y through Z (inclusive)
**   verbose         - show more detail in the description
**   download        - redirect to the download (artifact page only)
**   name=SHA1HASH   - Provide the SHA1HASH as a query parameter
**   filename=NAME   - Show information for content file NAME
**   fn=NAME         - "fn" is shorthand for "filename"
**   ci=VERSION      - The specific check-in to use for "filename=".
**
** The /artifact page show the complete content of a file
** identified by HASH as preformatted text.  The
** /whatis page shows only a description of the file.  The /file
** page shows the most recent version of the file or directory
** called NAME, or a list of the top-level directory if NAME is
** omitted.
*/
void artifact_page(void){
  int rid = 0;
  Blob content;
  const char *zMime;
  Blob downloadName;
  int renderAsWiki = 0;
  int renderAsHtml = 0;
  int objType;
  int asText;
  const char *zUuid;
  u32 objdescFlags = 0;
  int descOnly = fossil_strcmp(g.zPath,"whatis")==0;
  int isFile = fossil_strcmp(g.zPath,"file")==0;
  const char *zLn = P("ln");
  const char *zName = P("name");
  HQuery url;

  url_initialize(&url, g.zPath);
  rid = artifact_from_ci_and_filename(&url, 0);
  if( rid==0 ){
    url_add_parameter(&url, "name", zName);
    if( isFile ){
      /* Do a top-level directory listing in /file mode if no argument
      ** specified */
      if( zName==0 || zName[0]==0 ){
        if( P("ci")==0 ) cgi_set_query_parameter("ci","tip");
        page_tree();
        return;
      }
      /* Look for a single file with the given name */
      rid = db_int(0,
         "SELECT fid FROM filename, mlink, event"
         " WHERE name=%Q"
         "   AND mlink.fnid=filename.fnid"
         "   AND event.objid=mlink.mid"
         " ORDER BY event.mtime DESC LIMIT 1",
         zName
      );
      /* If no file called NAME exists, instead look for a directory
      ** with that name, and do a directory listing */
      if( rid==0 ){
        int nName = (int)strlen(zName);
        if( nName && zName[nName-1]=='/' ) nName--;
        if( db_exists(
           "SELECT 1 FROM filename"
           " WHERE name GLOB '%.*q/*' AND substr(name,1,%d)=='%.*q/';",
           nName, zName, nName+1, nName, zName
        ) ){
          if( P("ci")==0 ) cgi_set_query_parameter("ci","tip");
          page_tree();
          return;
        }
      }
      /* If no file or directory called NAME: issue an error */
      if( rid==0 ){
        style_header("No such file");
        cgi_printf("File '%h' does not exist in this repository.\n",(zName));
        style_footer();
        return;
      }
    }else{
      rid = name_to_rid_www("name");
    }
  }

  login_check_credentials();
  if( !g.perm.Read ){ login_needed(g.anon.Read); return; }
  if( rid==0 ){
    style_header("No such artifact");
    cgi_printf("Artifact '%h' does not exist in this repository.\n",(zName));
    style_footer();
    return;
  }
  if( descOnly || P("verbose")!=0 ){
    url_add_parameter(&url, "verbose", "1");
    objdescFlags |= OBJDESC_DETAIL;
  }
  zUuid = db_text("?", "SELECT uuid FROM blob WHERE rid=%d", rid);
  if( isFile ){
    cgi_printf("<h2>Latest version of file '%h':</h2>\n",(zName));
    style_submenu_element("Artifact", "%R/artifact/%S", zUuid);
  }else if( g.perm.Setup ){
    cgi_printf("<h2>Artifact %s (%d):</h2>\n",(zUuid),(rid));
  }else{
    cgi_printf("<h2>Artifact %s:</h2>\n",(zUuid));
  }
  blob_zero(&downloadName);
  objType = object_description(rid, objdescFlags, &downloadName);
  if( !descOnly && P("download")!=0 ){
    cgi_redirectf("%R/raw/%T?name=%s", blob_str(&downloadName),
          db_text("?", "SELECT uuid FROM blob WHERE rid=%d", rid));
    /*NOTREACHED*/
  }
  if( g.perm.Admin ){
    const char *zUuid = db_text("", "SELECT uuid FROM blob WHERE rid=%d", rid);
    if( db_exists("SELECT 1 FROM shun WHERE uuid=%Q", zUuid) ){
      style_submenu_element("Unshun", "%s/shun?accept=%s&sub=1#accshun",
            g.zTop, zUuid);
    }else{
      style_submenu_element("Shun", "%s/shun?shun=%s#addshun", g.zTop, zUuid);
    }
  }
  style_header("%s", isFile ? "File Content" :
                     descOnly ? "Artifact Description" : "Artifact Content");
  if( g.perm.Admin ){
    Stmt q;
    db_prepare(&q,
      "SELECT coalesce(user.login,rcvfrom.uid),"
      "       datetime(rcvfrom.mtime,toLocal()), rcvfrom.ipaddr"
      "  FROM blob, rcvfrom LEFT JOIN user ON user.uid=rcvfrom.uid"
      " WHERE blob.rid=%d"
      "   AND rcvfrom.rcvid=blob.rcvid;", rid);
    while( db_step(&q)==SQLITE_ROW ){
      const char *zUser = db_column_text(&q,0);
      const char *zDate = db_column_text(&q,1);
      const char *zIp = db_column_text(&q,2);
      cgi_printf("<p>Received on %s from %h at %h.</p>\n",(zDate),(zUser),(zIp));
    }
    db_finalize(&q);
  }
  style_submenu_element("Download", "%R/raw/%T?name=%s",
                         blob_str(&downloadName), zUuid);
  if( db_exists("SELECT 1 FROM mlink WHERE fid=%d", rid) ){
    style_submenu_element("Check-ins Using", "%R/timeline?n=200&uf=%s", zUuid);
  }
  asText = P("txt")!=0;
  zMime = mimetype_from_name(blob_str(&downloadName));
  if( zMime ){
    if( fossil_strcmp(zMime, "text/html")==0 ){
      if( asText ){
        style_submenu_element("Html", "%s", url_render(&url, "txt", 0, 0, 0));
      }else{
        renderAsHtml = 1;
        style_submenu_element("Text", "%s", url_render(&url, "txt", "1", 0, 0));
      }
    }else if( fossil_strcmp(zMime, "text/x-fossil-wiki")==0
           || fossil_strcmp(zMime, "text/x-markdown")==0 ){
      if( asText ){
        style_submenu_element("Wiki", "%s", url_render(&url, "txt", 0, 0, 0));
      }else{
        renderAsWiki = 1;
        style_submenu_element("Text", "%s", url_render(&url, "txt", "1", 0, 0));
      }
    }
  }
  if( (objType & (OBJTYPE_WIKI|OBJTYPE_TICKET))!=0 ){
    style_submenu_element("Parsed", "%R/info/%s", zUuid);
  }
  if( descOnly ){
    style_submenu_element("Content", "%R/artifact/%s", zUuid);
  }else{
    if( zLn==0 || atoi(zLn)==0 ){
      style_submenu_checkbox("ln", "Line Numbers", 0, 0);
    }
    cgi_printf("<hr />\n");
    content_get(rid, &content);
    if( renderAsWiki ){
      wiki_render_by_mimetype(&content, zMime);
    }else if( renderAsHtml ){
      cgi_printf("<iframe src=\"%R/raw/%T?name=%s\"\n"
             "width=\"100%%\" frameborder=\"0\" marginwidth=\"0\" marginheight=\"0\"\n"
             "sandbox=\"allow-same-origin\" id=\"ifm1\">\n"
             "</iframe>\n"
             "<script nonce=\"%h\">\n"
             "document.getElementById(\"ifm1\").addEventListener(\"load\",\n"
             "  function(){\n"
             "    this.height=this.contentDocument.documentElement.scrollHeight + 75;\n"
             "  }\n"
             ");\n"
             "</script>\n",(blob_str(&downloadName)),(zUuid),(style_nonce()));
    }else{
      style_submenu_element("Hex", "%s/hexdump?name=%s", g.zTop, zUuid);
      blob_to_utf8_no_bom(&content, 0);
      zMime = mimetype_from_content(&content);
      cgi_printf("<blockquote>\n");
      if( zMime==0 ){
        const char *z;
        z = blob_str(&content);
        if( zLn ){
          output_text_with_line_numbers(z, zLn);
        }else{
          cgi_printf("<pre>\n"
                 "%h\n"
                 "</pre>\n",(z));
        }
      }else if( strncmp(zMime, "image/", 6)==0 ){
        cgi_printf("<i>(file is %d bytes of image data)</i><br />\n"
               "<img src=\"%R/raw/%s?m=%s\" />\n",(blob_size(&content)),(zUuid),(zMime));
        style_submenu_element("Image", "%R/raw/%s?m=%s", zUuid, zMime);
      }else{
        cgi_printf("<i>(file is %d bytes of binary data)</i>\n",(blob_size(&content)));
      }
      cgi_printf("</blockquote>\n");
    }
  }
  style_footer();
}

/*
** WEBPAGE: tinfo
** URL: /tinfo?name=ARTIFACTID
**
** Show the details of a ticket change control artifact.
*/
void tinfo_page(void){
  int rid;
  char *zDate;
  const char *zUuid;
  char zTktName[HNAME_MAX+1];
  Manifest *pTktChng;
  int modPending;
  const char *zModAction;
  char *zTktTitle;
  login_check_credentials();
  if( !g.perm.RdTkt ){ login_needed(g.anon.RdTkt); return; }
  rid = name_to_rid_www("name");
  if( rid==0 ){ fossil_redirect_home(); }
  zUuid = db_text("", "SELECT uuid FROM blob WHERE rid=%d", rid);
  if( g.perm.Admin ){
    if( db_exists("SELECT 1 FROM shun WHERE uuid=%Q", zUuid) ){
      style_submenu_element("Unshun", "%s/shun?accept=%s&sub=1#accshun",
            g.zTop, zUuid);
    }else{
      style_submenu_element("Shun", "%s/shun?shun=%s#addshun", g.zTop, zUuid);
    }
  }
  pTktChng = manifest_get(rid, CFTYPE_TICKET, 0);
  if( pTktChng==0 ) fossil_redirect_home();
  zDate = db_text(0, "SELECT datetime(%.12f)", pTktChng->rDate);
  sqlite3_snprintf(sizeof(zTktName), zTktName, "%s", pTktChng->zTicketUuid);
  if( g.perm.ModTkt && (zModAction = P("modaction"))!=0 ){
    if( strcmp(zModAction,"delete")==0 ){
      moderation_disapprove(rid);
      /*
      ** Next, check if the ticket still exists; if not, we cannot
      ** redirect to it.
      */
      if( db_exists("SELECT 1 FROM ticket WHERE tkt_uuid GLOB '%q*'",
                    zTktName) ){
        cgi_redirectf("%R/tktview/%s", zTktName);
        /*NOTREACHED*/
      }else{
        cgi_redirectf("%R/modreq");
        /*NOTREACHED*/
      }
    }
    if( strcmp(zModAction,"approve")==0 ){
      moderation_approve(rid);
    }
  }
  zTktTitle = db_table_has_column("repository", "ticket", "title" )
      ? db_text("(No title)", 
                "SELECT title FROM ticket WHERE tkt_uuid=%Q", zTktName)
      : 0;
  style_header("Ticket Change Details");
  style_submenu_element("Raw", "%R/artifact/%s", zUuid);
  style_submenu_element("History", "%R/tkthistory/%s", zTktName);
  style_submenu_element("Page", "%R/tktview/%t", zTktName);
  style_submenu_element("Timeline", "%R/tkttimeline/%t", zTktName);
  if( P("plaintext") ){
    style_submenu_element("Formatted", "%R/info/%s", zUuid);
  }else{
    style_submenu_element("Plaintext", "%R/info/%s?plaintext", zUuid);
  }

  cgi_printf("<div class=\"section\">Overview</div>\n"
         "<p><table class=\"label-value\">\n"
         "<tr><th>Artifact&nbsp;ID:</th>\n"
         "<td>%z%s</a>\n",(href("%R/artifact/%!S",zUuid)),(zUuid));
  if( g.perm.Setup ){
    cgi_printf("(%d)\n",(rid));
  }
  modPending = moderation_pending_www(rid);
  cgi_printf("<tr><th>Ticket:</th>\n"
         "<td>%z%s</a>\n",(href("%R/tktview/%s",zTktName)),(zTktName));
  if( zTktTitle ){
       cgi_printf("<br />%h\n",(zTktTitle));
  }
 cgi_printf("</td></tr>\n"
         "<tr><th>User&nbsp;&amp;&nbsp;Date:</th><td>\n");
  hyperlink_to_user(pTktChng->zUser, zDate, " on ");
  hyperlink_to_date(zDate, "</td></tr>");
  cgi_printf("</table>\n");
  free(zDate);
  free(zTktTitle);

  if( g.perm.ModTkt && modPending ){
    cgi_printf("<div class=\"section\">Moderation</div>\n"
           "<blockquote>\n"
           "<form method=\"POST\" action=\"%R/tinfo/%s\">\n"
           "<label><input type=\"radio\" name=\"modaction\" value=\"delete\">\n"
           "Delete this change</label><br />\n"
           "<label><input type=\"radio\" name=\"modaction\" value=\"approve\">\n"
           "Approve this change</label><br />\n"
           "<input type=\"submit\" value=\"Submit\">\n"
           "</form>\n"
           "</blockquote>\n",(zUuid));
  }

  cgi_printf("<div class=\"section\">Changes</div>\n"
         "<p>\n");
  ticket_output_change_artifact(pTktChng, 0);
  manifest_destroy(pTktChng);
  style_footer();
}


/*
** WEBPAGE: info
** URL: info/ARTIFACTID
**
** The argument is a artifact ID which might be a check-in or a file or
** a ticket changes or a wiki edit or something else.
**
** Figure out what the artifact ID is and display it appropriately.
*/
void info_page(void){
  const char *zName;
  Blob uuid;
  int rid;
  int rc;
  int nLen;

  zName = P("name");
  if( zName==0 ) fossil_redirect_home();
  nLen = strlen(zName);
  blob_set(&uuid, zName);
  if( name_collisions(zName) ){
    cgi_set_parameter("src","info");
    ambiguous_page();
    return;
  }
  rc = name_to_uuid(&uuid, -1, "*");
  if( rc==1 ){
    if( validate16(zName, nLen) ){
      if( db_exists("SELECT 1 FROM ticket WHERE tkt_uuid GLOB '%q*'", zName) ){
        tktview_page();
        return;
      }
      if( db_exists("SELECT 1 FROM tag"
                    " WHERE tagname GLOB 'event-%q*'", zName) ){
        event_page();
        return;
      }
    }
    style_header("No Such Object");
    cgi_printf("<p>No such object: %h</p>\n",(zName));
    if( nLen<4 ){
      cgi_printf("<p>Object name should be no less than 4 characters.  Ten or more\n"
             "characters are recommended.</p>\n");
    }
    style_footer();
    return;
  }else if( rc==2 ){
    cgi_set_parameter("src","info");
    ambiguous_page();
    return;
  }
  zName = blob_str(&uuid);
  rid = db_int(0, "SELECT rid FROM blob WHERE uuid=%Q", zName);
  if( rid==0 ){
    style_header("Broken Link");
    cgi_printf("<p>No such object: %h</p>\n",(zName));
    style_footer();
    return;
  }
  if( db_exists("SELECT 1 FROM mlink WHERE mid=%d", rid) ){
    ci_page();
  }else
  if( db_exists("SELECT 1 FROM tagxref JOIN tag USING(tagid)"
                " WHERE rid=%d AND tagname LIKE 'wiki-%%'", rid) ){
    winfo_page();
  }else
  if( db_exists("SELECT 1 FROM tagxref JOIN tag USING(tagid)"
                " WHERE rid=%d AND tagname LIKE 'tkt-%%'", rid) ){
    tinfo_page();
  }else
  if( db_exists("SELECT 1 FROM plink WHERE cid=%d", rid) ){
    ci_page();
  }else
  if( db_exists("SELECT 1 FROM plink WHERE pid=%d", rid) ){
    ci_page();
  }else
  if( db_exists("SELECT 1 FROM attachment WHERE attachid=%d", rid) ){
    ainfo_page();
  }else
  if( db_table_exists("repository","forumpost")
   && db_exists("SELECT 1 FROM forumpost WHERE fpid=%d", rid)
  ){
    forumthread_page();
  }else
  {
    artifact_page();
  }
}

/*
** Do a comment comparison.
**
** +  Leading and trailing whitespace are ignored.
** +  \r\n characters compare equal to \n
**
** Return true if equal and false if not equal.
*/
static int comment_compare(const char *zA, const char *zB){
  if( zA==0 ) zA = "";
  if( zB==0 ) zB = "";
  while( fossil_isspace(zA[0]) ) zA++;
  while( fossil_isspace(zB[0]) ) zB++;
  while( zA[0] && zB[0] ){
    if( zA[0]==zB[0] ){ zA++; zB++; continue; }
    if( zA[0]=='\r' && zA[1]=='\n' && zB[0]=='\n' ){
      zA += 2;
      zB++;
      continue;
    }
    if( zB[0]=='\r' && zB[1]=='\n' && zA[0]=='\n' ){
      zB += 2;
      zA++;
      continue;
    }
    return 0;
  }
  while( fossil_isspace(zB[0]) ) zB++;
  while( fossil_isspace(zA[0]) ) zA++;
  return zA[0]==0 && zB[0]==0;
}

/*
** The following methods operate on the newtags temporary table
** that is used to collect various changes to be added to a control
** artifact for a check-in edit.
*/
static void init_newtags(void){
  db_multi_exec("CREATE TEMP TABLE newtags(tag UNIQUE, prefix, value)");
}

static void change_special(
  const char *zName,    /* Name of the special tag */
  const char *zOp,      /* Operation prefix (e.g. +,-,*) */
  const char *zValue    /* Value of the tag */
){
  db_multi_exec("REPLACE INTO newtags VALUES(%Q,'%q',%Q)", zName, zOp, zValue);
}

static void change_sym_tag(const char *zTag, const char *zOp){
  db_multi_exec("REPLACE INTO newtags VALUES('sym-%q',%Q,NULL)", zTag, zOp);
}

static void cancel_special(const char *zTag){
  change_special(zTag,"-",0);
}

static void add_color(const char *zNewColor, int fPropagateColor){
  change_special("bgcolor",fPropagateColor ? "*" : "+", zNewColor);
}

static void cancel_color(void){
  change_special("bgcolor","-",0);
}

static void add_comment(const char *zNewComment){
  change_special("comment","+",zNewComment);
}

static void add_date(const char *zNewDate){
  change_special("date","+",zNewDate);
}

static void add_user(const char *zNewUser){
  change_special("user","+",zNewUser);
}

static void add_tag(const char *zNewTag){
  change_sym_tag(zNewTag,"+");
}

static void cancel_tag(int rid, const char *zCancelTag){
  if( db_exists("SELECT 1 FROM tagxref, tag"
                " WHERE tagxref.rid=%d AND tagtype>0"
                "   AND tagxref.tagid=tag.tagid AND tagname='sym-%q'",
                rid, zCancelTag)
  ) change_sym_tag(zCancelTag,"-");
}

static void hide_branch(void){
  change_special("hidden","*",0);
}

static void close_leaf(int rid){
  change_special("closed",is_a_leaf(rid)?"+":"*",0);
}

static void change_branch(int rid, const char *zNewBranch){
  db_multi_exec(
    "REPLACE INTO newtags "
    " SELECT tagname, '-', NULL FROM tagxref, tag"
    "  WHERE tagxref.rid=%d AND tagtype==2"
    "    AND tagname GLOB 'sym-*'"
    "    AND tag.tagid=tagxref.tagid",
    rid
  );
  change_special("branch","*",zNewBranch);
  change_sym_tag(zNewBranch,"*");
}

/*
** The apply_newtags method is called after all newtags have been added
** and the control artifact is completed and then written to the DB.
*/
static void apply_newtags(
  Blob *ctrl,
  int rid,
  const char *zUuid,
  const char *zUserOvrd,  /* The user name on the control artifact */
  int fDryRun             /* Print control artifact, but make no changes */
){
  Stmt q;
  int nChng = 0;

  db_prepare(&q, "SELECT tag, prefix, value FROM newtags"
                 " ORDER BY prefix || tag");
  while( db_step(&q)==SQLITE_ROW ){
    const char *zTag = db_column_text(&q, 0);
    const char *zPrefix = db_column_text(&q, 1);
    const char *zValue = db_column_text(&q, 2);
    nChng++;
    if( zValue ){
      blob_appendf(ctrl, "T %s%F %s %F\n", zPrefix, zTag, zUuid, zValue);
    }else{
      blob_appendf(ctrl, "T %s%F %s\n", zPrefix, zTag, zUuid);
    }
  }
  db_finalize(&q);
  if( nChng>0 ){
    int nrid;
    Blob cksum;
    if( zUserOvrd && zUserOvrd[0] ){
      blob_appendf(ctrl, "U %F\n", zUserOvrd);
    }else{
      blob_appendf(ctrl, "U %F\n", login_name());
    }
    md5sum_blob(ctrl, &cksum);
    blob_appendf(ctrl, "Z %b\n", &cksum);
    if( fDryRun ){
      assert( g.isHTTP==0 ); /* Only print control artifact in console mode. */
      fossil_print("%s", blob_str(ctrl));
      blob_reset(ctrl);
    }else{
      db_begin_transaction();
      g.markPrivate = content_is_private(rid);
      nrid = content_put(ctrl);
      manifest_crosslink(nrid, ctrl, MC_PERMIT_HOOKS);
      db_end_transaction(0);
    }
    assert( blob_is_reset(ctrl) );
  }
}

/*
** This method checks that the date can be parsed.
** Returns 1 if datetime() can validate, 0 otherwise.
*/
int is_datetime(const char* zDate){
  return db_int(0, "SELECT datetime(%Q) NOT NULL", zDate);
}

/*
** WEBPAGE: ci_edit
**
** Edit a check-in.  (Check-ins are immutable and do not really change.
** This page really creates supplemental tags that affect the display
** of the check-in.)
**
** Query parmeters:
**
**     rid=INTEGER        Record ID of the check-in to edit (REQUIRED)
**
** POST parameters after pressing "Perview", "Cancel", or "Apply":
**
**     c=TEXT             New check-in comment
**     u=TEXT             New user name
**     newclr             Apply a background color
**     clr=TEXT           New background color (only if newclr)
**     pclr               Propagate new background color (only if newclr)
**     dt=TEXT            New check-in date/time (ISO8610 format)
**     newtag             Add a new tag to the check-in
**     tagname=TEXT       Name of the new tag to be added (only if newtag)
**     newbr              Put the check-in on a new branch
**     brname=TEXT        Name of the new branch (only if newbr)
**     close              Close this check-in
**     hide               Hide this check-in
**     cNNN               Cancel tag with tagid=NNN
**
**     cancel             Cancel the edit.  Return to the check-in view
**     preview            Show a preview of the edited check-in comment
**     apply              Apply changes
*/
void ci_edit_page(void){
  int rid;
  const char *zComment;         /* Current comment on the check-in */
  const char *zNewComment;      /* Revised check-in comment */
  const char *zUser;            /* Current user for the check-in */
  const char *zNewUser;         /* Revised user */
  const char *zDate;            /* Current date of the check-in */
  const char *zNewDate;         /* Revised check-in date */
  const char *zNewColorFlag;    /* "checked" if "Change color" is checked */
  const char *zColor;           /* Current background color */
  const char *zNewColor;        /* Revised background color */
  const char *zNewTagFlag;      /* "checked" if "Add tag" is checked */
  const char *zNewTag;          /* Name of the new tag */
  const char *zNewBrFlag;       /* "checked" if "New branch" is checked */
  const char *zNewBranch;       /* Name of the new branch */
  const char *zCloseFlag;       /* "checked" if "Close" is checked */
  const char *zHideFlag;        /* "checked" if "Hide" is checked */
  int fPropagateColor;          /* True if color propagates before edit */
  int fNewPropagateColor;       /* True if color propagates after edit */
  int fHasHidden = 0;           /* True if hidden tag already set */
  int fHasClosed = 0;           /* True if closed tag already set */
  const char *zChngTime = 0;    /* Value of chngtime= query param, if any */
  char *zUuid;
  Blob comment;
  char *zBranchName = 0;
  Stmt q;

  login_check_credentials();
  if( !g.perm.Write ){ login_needed(g.anon.Write); return; }
  rid = name_to_typed_rid(P("r"), "ci");
  zUuid = db_text(0, "SELECT uuid FROM blob WHERE rid=%d", rid);
  zComment = db_text(0, "SELECT coalesce(ecomment,comment)"
                        "  FROM event WHERE objid=%d", rid);
  if( zComment==0 ) fossil_redirect_home();
  if( P("cancel") ){
    cgi_redirectf("%R/ci/%S", zUuid);
  }
  if( g.perm.Setup ) zChngTime = P("chngtime");
  zNewComment = PD("c",zComment);
  zUser = db_text(0, "SELECT coalesce(euser,user)"
                     "  FROM event WHERE objid=%d", rid);
  if( zUser==0 ) fossil_redirect_home();
  zNewUser = PDT("u",zUser);
  zDate = db_text(0, "SELECT datetime(mtime)"
                     "  FROM event WHERE objid=%d", rid);
  if( zDate==0 ) fossil_redirect_home();
  zNewDate = PDT("dt",zDate);
  zColor = db_text("", "SELECT bgcolor"
                        "  FROM event WHERE objid=%d", rid);
  zNewColor = PDT("clr",zColor);
  fPropagateColor = db_int(0, "SELECT tagtype FROM tagxref"
                              " WHERE rid=%d AND tagid=%d",
                              rid, TAG_BGCOLOR)==2;
  fNewPropagateColor = P("clr")!=0 ? P("pclr")!=0 : fPropagateColor;
  zNewColorFlag = P("newclr") ? " checked" : "";
  zNewTagFlag = P("newtag") ? " checked" : "";
  zNewTag = PDT("tagname","");
  zNewBrFlag = P("newbr") ? " checked" : "";
  zNewBranch = PDT("brname","");
  zCloseFlag = P("close") ? " checked" : "";
  zHideFlag = P("hide") ? " checked" : "";
  if( P("apply") && cgi_csrf_safe(1) ){
    Blob ctrl;
    char *zNow;

    login_verify_csrf_secret();
    blob_zero(&ctrl);
    zNow = date_in_standard_format(zChngTime ? zChngTime : "now");
    blob_appendf(&ctrl, "D %s\n", zNow);
    init_newtags();
    if( zNewColorFlag[0]
     && zNewColor[0]
     && (fPropagateColor!=fNewPropagateColor
             || fossil_strcmp(zColor,zNewColor)!=0)
    ){
      add_color(zNewColor,fNewPropagateColor);
    }
    if( comment_compare(zComment,zNewComment)==0 ) add_comment(zNewComment);
    if( fossil_strcmp(zDate,zNewDate)!=0 ) add_date(zNewDate);
    if( fossil_strcmp(zUser,zNewUser)!=0 ) add_user(zNewUser);
    db_prepare(&q,
       "SELECT tag.tagid, tagname FROM tagxref, tag"
       " WHERE tagxref.rid=%d AND tagtype>0 AND tagxref.tagid=tag.tagid",
       rid
    );
    while( db_step(&q)==SQLITE_ROW ){
      int tagid = db_column_int(&q, 0);
      const char *zTag = db_column_text(&q, 1);
      char zLabel[30];
      sqlite3_snprintf(sizeof(zLabel), zLabel, "c%d", tagid);
      if( P(zLabel) ) cancel_special(zTag);
    }
    db_finalize(&q);
    if( zHideFlag[0] ) hide_branch();
    if( zCloseFlag[0] ) close_leaf(rid);
    if( zNewTagFlag[0] && zNewTag[0] ) add_tag(zNewTag);
    if( zNewBrFlag[0] && zNewBranch[0] ) change_branch(rid,zNewBranch);
    apply_newtags(&ctrl, rid, zUuid, 0, 0);
    cgi_redirectf("%R/ci/%S", zUuid);
  }
  blob_zero(&comment);
  blob_append(&comment, zNewComment, -1);
  zUuid[10] = 0;
  style_header("Edit Check-in [%s]", zUuid);
  if( P("preview") ){
    Blob suffix;
    int nTag = 0;
    cgi_printf("<b>Preview:</b>\n"
           "<blockquote>\n"
           "<table border=0>\n");
    if( zNewColorFlag[0] && zNewColor && zNewColor[0] ){
      cgi_printf("<tr><td style=\"background-color: %h;\">\n",(zNewColor));
    }else if( zColor[0] ){
      cgi_printf("<tr><td style=\"background-color: %h;\">\n",(zColor));
    }else{
      cgi_printf("<tr><td>\n");
    }
    cgi_printf("%!W\n",(blob_str(&comment)));
    blob_zero(&suffix);
    blob_appendf(&suffix, "(user: %h", zNewUser);
    db_prepare(&q, "SELECT substr(tagname,5) FROM tagxref, tag"
                   " WHERE tagname GLOB 'sym-*' AND tagxref.rid=%d"
                   "   AND tagtype>1 AND tag.tagid=tagxref.tagid",
                   rid);
    while( db_step(&q)==SQLITE_ROW ){
      const char *zTag = db_column_text(&q, 0);
      if( nTag==0 ){
        blob_appendf(&suffix, ", tags: %h", zTag);
      }else{
        blob_appendf(&suffix, ", %h", zTag);
      }
      nTag++;
    }
    db_finalize(&q);
    blob_appendf(&suffix, ")");
    cgi_printf("%s\n"
           "</td></tr></table>\n",(blob_str(&suffix)));
    if( zChngTime ){
      cgi_printf("<p>The timestamp on the tag used to make the changes above\n"
             "will be overridden as: %s</p>\n",(date_in_standard_format(zChngTime)));
    }
    cgi_printf("</blockquote>\n"
           "<hr />\n");
    blob_reset(&suffix);
  }
  cgi_printf("<p>Make changes to attributes of check-in\n"
         "[%z%s</a>]:</p>\n",(href("%R/ci/%!S",zUuid)),(zUuid));
  form_begin(0, "%R/ci_edit");
  login_insert_csrf_secret();
  cgi_printf("<div><input type=\"hidden\" name=\"r\" value=\"%s\" />\n"
         "<table border=\"0\" cellspacing=\"10\">\n",(zUuid));

  cgi_printf("<tr><th align=\"right\" valign=\"top\">User:</th>\n"
         "<td valign=\"top\">\n"
         "  <input type=\"text\" name=\"u\" size=\"20\" value=\"%h\" />\n"
         "</td></tr>\n",(zNewUser));

  cgi_printf("<tr><th align=\"right\" valign=\"top\">Comment:</th>\n"
         "<td valign=\"top\">\n"
         "<textarea name=\"c\" rows=\"10\" cols=\"80\">%h</textarea>\n"
         "</td></tr>\n",(zNewComment));

  cgi_printf("<tr><th align=\"right\" valign=\"top\">Check-in Time:</th>\n"
         "<td valign=\"top\">\n"
         "  <input type=\"text\" name=\"dt\" size=\"20\" value=\"%h\" />\n"
         "</td></tr>\n",(zNewDate));

  if( zChngTime ){
    cgi_printf("<tr><th align=\"right\" valign=\"top\">Timestamp of this change:</th>\n"
           "<td valign=\"top\">\n"
           "  <input type=\"text\" name=\"chngtime\" size=\"20\" value=\"%h\" />\n"
           "</td></tr>\n",(zChngTime));
  }

  cgi_printf("<tr><th align=\"right\" valign=\"top\">Background&nbsp;Color:</th>\n"
         "<td valign=\"top\">\n"
         "<div><label><input type='checkbox' name='newclr'%s />\n"
         "Change background color: "
         "<input type='color' name='clr'"
         "value='%s'></label></div>\n"
         "<div><label>\n",(zNewColorFlag),(zNewColor[0]?zNewColor:"#808080"));
  if( fNewPropagateColor ){
    cgi_printf("<input type=\"checkbox\" name=\"pclr\" checked=\"checked\" />\n");
  }else{
    cgi_printf("<input type=\"checkbox\" name=\"pclr\" />\n");
  }
  cgi_printf("Propagate color to descendants</label></div>\n"
         "</td></tr>\n");

  cgi_printf("<tr><th align=\"right\" valign=\"top\">Tags:</th>\n"
         "<td valign=\"top\">\n"
         "<label><input type=\"checkbox\" id=\"newtag\" name=\"newtag\"%s />\n"
         "Add the following new tag name to this check-in:</label>\n"
         "<input type=\"text\" size='15' name=\"tagname\" value=\"%h\" "
         "id='tagname' />\n",(zNewTagFlag),(zNewTag));
  zBranchName = db_text(0, "SELECT value FROM tagxref, tag"
     " WHERE tagxref.rid=%d AND tagtype>0 AND tagxref.tagid=tag.tagid"
     " AND tagxref.tagid=%d", rid, TAG_BRANCH);
  db_prepare(&q,
     "SELECT tag.tagid, tagname, tagxref.value FROM tagxref, tag"
     " WHERE tagxref.rid=%d AND tagtype>0 AND tagxref.tagid=tag.tagid"
     " ORDER BY CASE WHEN tagname GLOB 'sym-*' THEN substr(tagname,5)"
     "               ELSE tagname END /*sort*/",
     rid
  );
  while( db_step(&q)==SQLITE_ROW ){
    int tagid = db_column_int(&q, 0);
    const char *zTagName = db_column_text(&q, 1);
    int isSpecialTag = fossil_strncmp(zTagName, "sym-", 4)!=0;
    char zLabel[30];

    if( tagid == TAG_CLOSED ){
      fHasClosed = 1;
    }else if( (tagid == TAG_COMMENT) || (tagid == TAG_BRANCH) ){
      continue;
    }else if( tagid==TAG_HIDDEN ){
      fHasHidden = 1;
    }else if( !isSpecialTag && zTagName &&
        fossil_strcmp(&zTagName[4], zBranchName)==0){
      continue;
    }
    sqlite3_snprintf(sizeof(zLabel), zLabel, "c%d", tagid);
    cgi_printf("<br /><label>\n");
    if( P(zLabel) ){
      cgi_printf("<input type=\"checkbox\" name=\"c%d\" checked=\"checked\" />\n",(tagid));
    }else{
      cgi_printf("<input type=\"checkbox\" name=\"c%d\" />\n",(tagid));
    }
    if( isSpecialTag ){
      cgi_printf("Cancel special tag <b>%h</b></label>\n",(zTagName));
    }else{
      cgi_printf("Cancel tag <b>%h</b></label>\n",(&zTagName[4]));
    }
  }
  db_finalize(&q);
  cgi_printf("</td></tr>\n");

  if( !zBranchName ){
    zBranchName = db_get("main-branch", "trunk");
  }
  if( !zNewBranch || !zNewBranch[0]){
    zNewBranch = zBranchName;
  }
  cgi_printf("<tr><th align=\"right\" valign=\"top\">Branching:</th>\n"
         "<td valign=\"top\">\n"
         "<label><input id=\"newbr\" type=\"checkbox\" name=\"newbr\" "
         "data-branch='%h'%s />\n"
         "Make this check-in the start of a new branch named:</label>\n"
         "<input id=\"brname\" type=\"text\" style=\"width:15;\" name=\"brname\" "
         "value=\"%h\" /></td></tr>\n",(zBranchName),(zNewBrFlag),(zNewBranch));
  if( !fHasHidden ){
    cgi_printf("<tr><th align=\"right\" valign=\"top\">Branch Hiding:</th>\n"
           "<td valign=\"top\">\n"
           "<label><input type=\"checkbox\" id=\"hidebr\" name=\"hide\"%s />\n"
           "Hide branch\n"
           "<span style=\"font-weight:bold\" id=\"hbranch\">%h</span>\n"
           "from the timeline starting from this check-in</label>\n"
           "</td></tr>\n",(zHideFlag),(zBranchName));
  }
  if( !fHasClosed ){
    if( is_a_leaf(rid) ){
      cgi_printf("<tr><th align=\"right\" valign=\"top\">Leaf Closure:</th>\n"
             "<td valign=\"top\">\n"
             "<label><input type=\"checkbox\" name=\"close\"%s />\n"
             "Mark this leaf as \"closed\" so that it no longer appears on the\n"
             "\"leaves\" page and is no longer labeled as a \"<b>Leaf</b>\"</label>\n"
             "</td></tr>\n",(zCloseFlag));
    }else if( zBranchName ){
      cgi_printf("<tr><th align=\"right\" valign=\"top\">Branch Closure:</th>\n"
             "<td valign=\"top\">\n"
             "<label><input type=\"checkbox\" name=\"close\"%s />\n"
             "Mark branch\n"
             "<span style=\"font-weight:bold\" id=\"cbranch\">%h</span>\n"
             "as \"closed\".</label>\n"
             "</td></tr>\n",(zCloseFlag),(zBranchName));
    }
  }
  if( zBranchName ) fossil_free(zBranchName);


  cgi_printf("<tr><td colspan=\"2\">\n"
         "<input type=\"submit\" name=\"cancel\" value=\"Cancel\" />\n"
         "<input type=\"submit\" name=\"preview\" value=\"Preview\" />\n");
  if( P("preview") ){
    cgi_printf("<input type=\"submit\" name=\"apply\" value=\"Apply Changes\" />\n");
  }
  cgi_printf("</td></tr>\n"
         "</table>\n"
         "</div></form>\n");
  style_load_one_js_file("ci_edit.js");
  style_footer();
}

/*
** Prepare an ammended commit comment.  Let the user modify it using the
** editor specified in the global_config table or either
** the VISUAL or EDITOR environment variable.
**
** Store the final commit comment in pComment.  pComment is assumed
** to be uninitialized - any prior content is overwritten.
**
** Use zInit to initialize the check-in comment so that the user does
** not have to retype.
*/
static void prepare_amend_comment(
  Blob *pComment,
  const char *zInit,
  const char *zUuid
){
  Blob prompt;
#if defined(_WIN32) || defined(__CYGWIN__)
  int bomSize;
  const unsigned char *bom = get_utf8_bom(&bomSize);
  blob_init(&prompt, (const char *) bom, bomSize);
  if( zInit && zInit[0]){
    blob_append(&prompt, zInit, -1);
  }
#else
  blob_init(&prompt, zInit, -1);
#endif
  blob_append(&prompt, "\n# Enter a new comment for check-in ", -1);
  if( zUuid && zUuid[0] ){
    blob_append(&prompt, zUuid, -1);
  }
  blob_append(&prompt, ".\n# Lines beginning with a # are ignored.\n", -1);
  prompt_for_user_comment(pComment, &prompt);
  blob_reset(&prompt);
}

#define AMEND_USAGE_STMT "UUID OPTION ?OPTION ...?"
/*
** COMMAND: amend
**
** Usage: %fossil amend UUID OPTION ?OPTION ...?
**
** Amend the tags on check-in UUID to change how it displays in the timeline.
**
** Options:
**
**    --author USER           Make USER the author for check-in
**    -m|--comment COMMENT    Make COMMENT the check-in comment
**    -M|--message-file FILE  Read the amended comment from FILE
**    -e|--edit-comment       Launch editor to revise comment
**    --date DATETIME         Make DATETIME the check-in time
**    --bgcolor COLOR         Apply COLOR to this check-in
**    --branchcolor COLOR     Apply and propagate COLOR to the branch
**    --tag TAG               Add new TAG to this check-in
**    --cancel TAG            Cancel TAG from this check-in
**    --branch NAME           Make this check-in the start of branch NAME
**    --hide                  Hide branch starting from this check-in
**    --close                 Mark this "leaf" as closed
**    -n|--dry-run            Print control artifact, but make no changes
**    --date-override DATETIME  Set the change time on the control artifact
**    --user-override USER      Set the user name on the control artifact
**
** DATETIME may be "now" or "YYYY-MM-DDTHH:MM:SS.SSS". If in
** year-month-day form, it may be truncated, the "T" may be replaced by
** a space, and it may also name a timezone offset from UTC as "-HH:MM"
** (westward) or "+HH:MM" (eastward). Either no timezone suffix or "Z"
** means UTC.
*/
void ci_amend_cmd(void){
  int rid;
  const char *zComment;         /* Current comment on the check-in */
  const char *zNewComment;      /* Revised check-in comment */
  const char *zComFile;         /* Filename from which to read comment */
  const char *zUser;            /* Current user for the check-in */
  const char *zNewUser;         /* Revised user */
  const char *zDate;            /* Current date of the check-in */
  const char *zNewDate;         /* Revised check-in date */
  const char *zColor;
  const char *zNewColor;
  const char *zNewBrColor;
  const char *zNewBranch;
  const char **pzNewTags = 0;
  const char **pzCancelTags = 0;
  int fClose;                   /* True if leaf should be closed */
  int fHide;                    /* True if branch should be hidden */
  int fPropagateColor;          /* True if color propagates before amend */
  int fNewPropagateColor = 0;   /* True if color propagates after amend */
  int fHasHidden = 0;           /* True if hidden tag already set */
  int fHasClosed = 0;           /* True if closed tag already set */
  int fEditComment;             /* True if editor to be used for comment */
  int fDryRun;                  /* Print control artifact, make no changes */
  const char *zChngTime;        /* The change time on the control artifact */
  const char *zUserOvrd;        /* The user name on the control artifact */
  const char *zUuid;
  Blob ctrl;
  Blob comment;
  char *zNow;
  int nTags, nCancels;
  int i;
  Stmt q;

  if( g.argc==3 ) usage(AMEND_USAGE_STMT);
  fEditComment = find_option("edit-comment","e",0)!=0;
  zNewComment = find_option("comment","m",1);
  zComFile = find_option("message-file","M",1);
  zNewBranch = find_option("branch",0,1);
  zNewColor = find_option("bgcolor",0,1);
  zNewBrColor = find_option("branchcolor",0,1);
  if( zNewBrColor ){
    zNewColor = zNewBrColor;
    fNewPropagateColor = 1;
  }
  zNewDate = find_option("date",0,1);
  zNewUser = find_option("author",0,1);
  pzNewTags = find_repeatable_option("tag",0,&nTags);
  pzCancelTags = find_repeatable_option("cancel",0,&nCancels);
  fClose = find_option("close",0,0)!=0;
  fHide = find_option("hide",0,0)!=0;
  fDryRun = find_option("dry-run","n",0)!=0;
  if( fDryRun==0 ) fDryRun = find_option("dryrun","n",0)!=0;
  zChngTime = find_option("date-override",0,1);
  if( zChngTime==0 ) zChngTime = find_option("chngtime",0,1);
  zUserOvrd = find_option("user-override",0,1);
  db_find_and_open_repository(0,0);
  user_select();
  verify_all_options();
  if( g.argc<3 || g.argc>=4 ) usage(AMEND_USAGE_STMT);
  rid = name_to_typed_rid(g.argv[2], "ci");
  if( rid==0 && !is_a_version(rid) ) fossil_fatal("no such check-in");
  zUuid = db_text(0, "SELECT uuid FROM blob WHERE rid=%d", rid);
  if( zUuid==0 ) fossil_fatal("Unable to find UUID");
  zComment = db_text(0, "SELECT coalesce(ecomment,comment)"
                        "  FROM event WHERE objid=%d", rid);
  zUser = db_text(0, "SELECT coalesce(euser,user)"
                     "  FROM event WHERE objid=%d", rid);
  zDate = db_text(0, "SELECT datetime(mtime)"
                     "  FROM event WHERE objid=%d", rid);
  zColor = db_text("", "SELECT bgcolor"
                        "  FROM event WHERE objid=%d", rid);
  fPropagateColor = db_int(0, "SELECT tagtype FROM tagxref"
                              " WHERE rid=%d AND tagid=%d",
                              rid, TAG_BGCOLOR)==2;
  fNewPropagateColor = zNewColor && zNewColor[0]
                        ? fNewPropagateColor : fPropagateColor;
  db_prepare(&q,
     "SELECT tag.tagid FROM tagxref, tag"
     " WHERE tagxref.rid=%d AND tagtype>0 AND tagxref.tagid=tag.tagid",
     rid
  );
  while( db_step(&q)==SQLITE_ROW ){
    int tagid = db_column_int(&q, 0);

    if( tagid == TAG_CLOSED ){
      fHasClosed = 1;
    }else if( tagid==TAG_HIDDEN ){
      fHasHidden = 1;
    }else{
      continue;
    }
  }
  db_finalize(&q);
  blob_zero(&ctrl);
  zNow = date_in_standard_format(zChngTime && zChngTime[0] ? zChngTime : "now");
  blob_appendf(&ctrl, "D %s\n", zNow);
  init_newtags();
  if( zNewColor && zNewColor[0]
      && (fPropagateColor!=fNewPropagateColor
            || fossil_strcmp(zColor,zNewColor)!=0)
  ){
    add_color(
      mprintf("%s%s", (zNewColor[0]!='#' &&
        validate16(zNewColor,strlen(zNewColor)) &&
        (strlen(zNewColor)==6 || strlen(zNewColor)==3)) ? "#" : "",
        zNewColor
      ),
      fNewPropagateColor
    );
  }
  if( (zNewColor!=0 && zNewColor[0]==0) && (zColor && zColor[0] ) ){
    cancel_color();
  }
  if( fEditComment ){
    prepare_amend_comment(&comment, zComment, zUuid);
    zNewComment = blob_str(&comment);
  }else if( zComFile ){
    blob_zero(&comment);
    blob_read_from_file(&comment, zComFile, ExtFILE);
    blob_to_utf8_no_bom(&comment, 1);
    zNewComment = blob_str(&comment);
  }
  if( zNewComment && zNewComment[0]
      && comment_compare(zComment,zNewComment)==0 ) add_comment(zNewComment);
  if( zNewDate && zNewDate[0] && fossil_strcmp(zDate,zNewDate)!=0 ){
    if( is_datetime(zNewDate) ){
      add_date(zNewDate);
    }else{
      fossil_fatal("Unsupported date format, use YYYY-MM-DD HH:MM:SS");
    }
  }
  if( zNewUser && zNewUser[0] && fossil_strcmp(zUser,zNewUser)!=0 ){
    add_user(zNewUser);
  }
  if( pzNewTags!=0 ){
    for(i=0; i<nTags; i++){
      if( pzNewTags[i] && pzNewTags[i][0] ) add_tag(pzNewTags[i]);
    }
    fossil_free((void *)pzNewTags);
  }
  if( pzCancelTags!=0 ){
    for(i=0; i<nCancels; i++){
      if( pzCancelTags[i] && pzCancelTags[i][0] )
        cancel_tag(rid,pzCancelTags[i]);
    }
    fossil_free((void *)pzCancelTags);
  }
  if( fHide && !fHasHidden ) hide_branch();
  if( fClose && !fHasClosed ) close_leaf(rid);
  if( zNewBranch && zNewBranch[0] ) change_branch(rid,zNewBranch);
  apply_newtags(&ctrl, rid, zUuid, zUserOvrd, fDryRun);
  if( fDryRun==0 ){
    show_common_info(rid, "uuid:", 1, 0);
  }
  if( g.localOpen ){
    manifest_to_disk(rid);
  }
}
