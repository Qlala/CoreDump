#line 1 "..\\src\\stat.c"
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
** This file contains code to implement the stat web page
**
*/
#include "VERSION.h"
#include "config.h"
#include <string.h>
#include "stat.h"

/*
** For a sufficiently large integer, provide an alternative
** representation as MB or GB or TB.
*/
void bigSizeName(int nOut, char *zOut, sqlite3_int64 v){
  if( v<100000 ){
    sqlite3_snprintf(nOut, zOut, "%,lld bytes", v);
  }else if( v<1000000000 ){
    sqlite3_snprintf(nOut, zOut, "%,lld bytes (%.1fMB)",
                    v, (double)v/1000000.0);
  }else{
    sqlite3_snprintf(nOut, zOut, "%,lld bytes (%.1fGB)",
                    v, (double)v/1000000000.0);
  }
}

/*
** Return the approximate size as KB, MB, GB, or TB.
*/
void approxSizeName(int nOut, char *zOut, sqlite3_int64 v){
  if( v<1000 ){
    sqlite3_snprintf(nOut, zOut, "%,lld bytes", v);
  }else if( v<1000000 ){
    sqlite3_snprintf(nOut, zOut, "%.1fKB", (double)v/1000.0);
  }else if( v<1000000000 ){
    sqlite3_snprintf(nOut, zOut, "%.1fMB", (double)v/1000000.0);
  }else{
    sqlite3_snprintf(nOut, zOut, "%.1fGB", (double)v/1000000000.0);
  }
}

/*
** Generate stats for the email notification subsystem.
*/
void stats_for_email(void){
  const char *zDest = db_get("email-send-method",0);
  int nSub, nASub, nPend, nDPend;
  const char *zDir, *zDb, *zCmd, *zRelay;
  cgi_printf("<tr><th>Outgoing&nbsp;Email:</th><td>\n");
  if( fossil_strcmp(zDest,"pipe")==0
   && (zCmd = db_get("email-send-command",0))!=0
  ){
    cgi_printf("Piped to command \"%h\"\n",(zCmd));
  }else
  if( fossil_strcmp(zDest,"db")==0
   && (zDb = db_get("email-send-db",0))!=0
  ){
    sqlite3 *db;
    sqlite3_stmt *pStmt;
    int rc;
    cgi_printf("Queued to database \"%h\"\n",(zDb));
    rc = sqlite3_open(zDb, &db);
    if( rc==SQLITE_OK ){
      rc = sqlite3_prepare_v2(db, "SELECT count(*) FROM email",-1,&pStmt,0);
      if( rc==SQLITE_OK && sqlite3_step(pStmt)==SQLITE_ROW ){
        cgi_printf("(%,d messages,\n"
               "%,d bytes)\n",(sqlite3_column_int(pStmt,0)),(file_size(zDb,ExtFILE)));
      }
      sqlite3_finalize(pStmt);
    }
    sqlite3_close(db);
  }else
  if( fossil_strcmp(zDest,"dir")==0
   && (zDir = db_get("email-send-dir",0))!=0
  ){
    cgi_printf("Written to files in \"%h\"\n"
           "(%,d messages)\n",(zDir),(file_directory_size(zDir,0,1)));
  }else
  if( fossil_strcmp(zDest,"relay")==0
   && (zRelay = db_get("email-send-relayhost",0))!=0
  ){
    cgi_printf("Relay to %h using SMTP\n",(zRelay));
  }
  else{
    cgi_printf("Off\n");
  }
  cgi_printf("</td></tr>\n");
  nPend = db_int(0,"SELECT count(*) FROM pending_alert WHERE NOT sentSep");
  nDPend = db_int(0,"SELECT count(*) FROM pending_alert"
                    " WHERE NOT sentDigest");
  cgi_printf("<tr><th>Pending&nbsp;Alerts:</th><td>\n"
         "%,d normal, %,d digest\n"
         "</td></tr>\n"
         "<tr><th>Subscribers:</th><td>\n",(nPend),(nDPend));
  nSub = db_int(0, "SELECT count(*) FROM subscriber");
  nASub = db_int(0, "SELECT count(*) FROM subscriber WHERE sverified"
                   " AND NOT sdonotcall AND length(ssub)>1");
  cgi_printf("%,d active, %,d total\n"
         "</td></tr>\n",(nASub),(nSub));
}

/*
** WEBPAGE: stat
**
** Show statistics and global information about the repository.
*/
void stat_page(void){
  i64 t, fsize;
  int n, m;
  int szMax, szAvg;
  int brief;
  const char *p;

  login_check_credentials();
  if( !g.perm.Read ){ login_needed(g.anon.Read); return; }
  brief = P("brief")!=0;
  style_header("Repository Statistics");
  style_adunit_config(ADUNIT_RIGHT_OK);
  if( g.perm.Admin ){
    style_submenu_element("URLs", "urllist");
    style_submenu_element("Schema", "repo_schema");
    style_submenu_element("Web-Cache", "cachestat");
  }
  style_submenu_element("Activity Reports", "reports");
  style_submenu_element("Hash Collisions", "hash-collisions");
  style_submenu_element("Artifacts", "bloblist");
  if( sqlite3_compileoption_used("ENABLE_DBSTAT_VTAB") ){
    style_submenu_element("Table Sizes", "repo-tabsize");
  }
  if( g.perm.Admin || g.perm.Setup || db_get_boolean("test_env_enable",0) ){
    style_submenu_element("Environment", "test_env");
  }
  cgi_printf("<table class=\"label-value\">\n");
  fsize = file_size(g.zRepositoryName, ExtFILE);
  cgi_printf("<tr><th>Repository&nbsp;Size:</th><td>%,lld bytes</td>\n"
         "</td></tr>\n",(fsize));
  if( !brief ){
    cgi_printf("<tr><th>Number&nbsp;Of&nbsp;Artifacts:</th><td>\n");
    n = db_int(0, "SELECT count(*) FROM blob");
    m = db_int(0, "SELECT count(*) FROM delta");
    cgi_printf("%.d (%,d fulltext and %,d deltas)\n",(n),(n-m),(m));
    if( g.perm.Write ){
      cgi_printf("<a href='%R/artifact_stats'>Details</a>\n");
    }
    cgi_printf("</td></tr>\n");
    if( n>0 ){
      int a, b;
      Stmt q;
      cgi_printf("<tr><th>Uncompressed&nbsp;Artifact&nbsp;Size:</th><td>\n");
      db_prepare(&q, "SELECT total(size), avg(size), max(size)"
                     " FROM blob WHERE size>0 /*scan*/");
      db_step(&q);
      t = db_column_int64(&q, 0);
      szAvg = db_column_int(&q, 1);
      szMax = db_column_int(&q, 2);
      db_finalize(&q);
      cgi_printf("%,d bytes average, %,d bytes max, %,lld total\n"
             "</td></tr>\n"
             "<tr><th>Compression&nbsp;Ratio:</th><td>\n",(szAvg),(szMax),(t));
      if( t/fsize < 5 ){
        b = 10;
        a = t/(fsize/10);
      }else{
        b = 1;
        a = t/fsize;
      }
      cgi_printf("%d:%d\n"
             "</td></tr>\n",(a),(b));
    }
    if( db_table_exists("repository","unversioned") ){
      Stmt q;
      char zStored[100];
      db_prepare(&q,
        "SELECT count(*), sum(sz), sum(length(content))"
        "  FROM unversioned"
        " WHERE length(hash)>1"
      );
      if( db_step(&q)==SQLITE_ROW && (n = db_column_int(&q,0))>0 ){
        sqlite3_int64 iStored, pct;
        iStored = db_column_int64(&q,2);
        pct = (iStored*100 + fsize/2)/fsize;
        approxSizeName(sizeof(zStored), zStored, iStored);
        cgi_printf("<tr><th>Unversioned&nbsp;Files:</th><td>\n"
               "%z%d files</a>,\n"
               "%s compressed, %d%% of total repository space\n"
               "</td></tr>\n",(href("%R/uvlist")),(n),(zStored),(pct));
      }
      db_finalize(&q);
    }
    cgi_printf("<tr><th>Number&nbsp;Of&nbsp;Check-ins:</th><td>\n");
    n = db_int(0, "SELECT count(*) FROM event WHERE type='ci' /*scan*/");
    cgi_printf("%,d\n"
           "</td></tr>\n"
           "<tr><th>Number&nbsp;Of&nbsp;Files:</th><td>\n",(n));
    n = db_int(0, "SELECT count(*) FROM filename /*scan*/");
    cgi_printf("%,d\n"
           "</td></tr>\n"
           "<tr><th>Number&nbsp;Of&nbsp;Wiki&nbsp;Pages:</th><td>\n",(n));
    n = db_int(0, "SELECT count(*) FROM tag  /*scan*/"
                  " WHERE +tagname GLOB 'wiki-*'");
    cgi_printf("%,d\n"
           "</td></tr>\n"
           "<tr><th>Number&nbsp;Of&nbsp;Tickets:</th><td>\n",(n));
    n = db_int(0, "SELECT count(*) FROM tag  /*scan*/"
                  " WHERE +tagname GLOB 'tkt-*'");
    cgi_printf("%,d\n"
           "</td></tr>\n",(n));
  }
  cgi_printf("<tr><th>Duration&nbsp;Of&nbsp;Project:</th><td>\n");
  n = db_int(0, "SELECT julianday('now') - (SELECT min(mtime) FROM event)"
                " + 0.99");
  cgi_printf("%,d days or approximately %.2f years.\n"
         "</td></tr>\n",(n),(n/365.2425));
  p = db_get("project-code", 0);
  if( p ){
    cgi_printf("<tr><th>Project&nbsp;ID:</th>\n"
           "    <td>%h %h</td></tr>\n",(p),(db_get("project-name","")));
  }
  p = db_get("parent-project-code", 0);
  if( p ){
    cgi_printf("<tr><th>Parent&nbsp;Project&nbsp;ID:</th>\n"
           "     <td>%h %h</td></tr>\n",(p),(db_get("parent-project-name","")));
  }
  /* @ <tr><th>Server&nbsp;ID:</th><td>%h(db_get("server-code",""))</td></tr> */
  cgi_printf("<tr><th>Fossil&nbsp;Version:</th><td>\n"
         "%h %h\n"
         "(%h) <a href='version?verbose'>(details)</a>\n"
         "</td></tr>\n"
         "<tr><th>SQLite&nbsp;Version:</th><td>%.19s\n"
         "[%.10s] (%s)\n"
         "<a href='version?verbose'>(details)</a></td></tr>\n",(MANIFEST_DATE),(MANIFEST_VERSION),(RELEASE_VERSION),(sqlite3_sourceid()),(&sqlite3_sourceid()[20]),(sqlite3_libversion()));
  if( g.eHashPolicy!=HPOLICY_AUTO ){
    cgi_printf("<tr><th>Schema&nbsp;Version:</th><td>%h,\n"
           "%s</td></tr>\n",(g.zAuxSchema),(hpolicy_name()));
  }else{
    cgi_printf("<tr><th>Schema&nbsp;Version:</th><td>%h</td></tr>\n",(g.zAuxSchema));
  }
  cgi_printf("<tr><th>Repository Rebuilt:</th><td>\n"
         "%h\n"
         "By Fossil %h</td></tr>\n"
         "<tr><th>Database&nbsp;Stats:</th><td>\n"
         "%,d pages,\n"
         "%d bytes/page,\n"
         "%,d free pages,\n"
         "%s,\n"
         "%s mode\n"
         "</td></tr>\n",(db_get_mtime("rebuilt","%Y-%m-%d %H:%M:%S","Never")),(db_get("rebuilt","Unknown")),(db_int(0, "PRAGMA repository.page_count")),(db_int(0, "PRAGMA repository.page_size")),(db_int(0, "PRAGMA repository.freelist_count")),(db_text(0, "PRAGMA repository.encoding")),(db_text(0, "PRAGMA repository.journal_mode")));
  if( g.perm.Admin && g.zErrlog && g.zErrlog[0] ){
    i64 szFile = file_size(g.zErrlog, ExtFILE);
    if( szFile>=0 ){
      cgi_printf("<tr><th>Error Log:</th>\n"
             "<td><a href='%R/errorlog'>%h</a> (%,lld bytes)\n",(g.zErrlog),(szFile));
    }
    cgi_printf("</td></tr>\n");
  }
  if( g.perm.Admin ){
    cgi_printf("<tr><th>Backoffice:</th>\n"
           "<td>Last run: %z</td></tr>\n",(backoffice_last_run()));
  }
  if( g.perm.Admin && alert_enabled() ){
    stats_for_email();
  }

  cgi_printf("</table>\n");
  style_footer();
}

/*
** COMMAND: dbstat*
**
** Usage: %fossil dbstat OPTIONS
**
** Shows statistics and global information about the repository.
**
** Options:
**
**   --brief|-b           Only show essential elements
**   --db-check           Run a PRAGMA quick_check on the repository database
**   --omit-version-info  Omit the SQLite and Fossil version information
*/
void dbstat_cmd(void){
  i64 t, fsize;
  int n, m;
  int szMax, szAvg;
  int brief;
  int omitVers;            /* Omit Fossil and SQLite version information */
  int dbCheck;             /* True for the --db-check option */
  const int colWidth = -19 /* printf alignment/width for left column */;
  const char *p, *z;

  brief = find_option("brief", "b",0)!=0;
  omitVers = find_option("omit-version-info", 0, 0)!=0;
  dbCheck = find_option("db-check",0,0)!=0;
  db_find_and_open_repository(0,0);

  /* We should be done with options.. */
  verify_all_options();

  if( (z = db_get("project-name",0))!=0
   || (z = db_get("short-project-name",0))!=0
  ){
    fossil_print("%*s%s\n", colWidth, "project-name:", z);
  }
  fsize = file_size(g.zRepositoryName, ExtFILE);
  fossil_print( "%*s%,lld bytes\n", colWidth, "repository-size:", fsize);
  if( !brief ){
    n = db_int(0, "SELECT count(*) FROM blob");
    m = db_int(0, "SELECT count(*) FROM delta");
    fossil_print("%*s%,d (stored as %,d full text and %,d deltas)\n",
                 colWidth, "artifact-count:",
                 n, n-m, m);
    if( n>0 ){
      int a, b;
      Stmt q;
      db_prepare(&q, "SELECT total(size), avg(size), max(size)"
                     " FROM blob WHERE size>0");
      db_step(&q);
      t = db_column_int64(&q, 0);
      szAvg = db_column_int(&q, 1);
      szMax = db_column_int(&q, 2);
      db_finalize(&q);
      fossil_print( "%*s%,d average, "
                    "%,d max, %,lld total\n",
                    colWidth, "artifact-sizes:",
                    szAvg, szMax, t);
      if( t/fsize < 5 ){
        b = 10;
        fsize /= 10;
      }else{
        b = 1;
      }
      a = t/fsize;
      fossil_print("%*s%d:%d\n", colWidth, "compression-ratio:", a, b);
    }
    n = db_int(0, "SELECT COUNT(*) FROM event e WHERE e.type='ci'");
    fossil_print("%*s%,d\n", colWidth, "check-ins:", n);
    n = db_int(0, "SELECT count(*) FROM filename /*scan*/");
    fossil_print("%*s%,d across all branches\n", colWidth, "files:", n);
    n = db_int(0, "SELECT count(*) FROM tag  /*scan*/"
                  " WHERE tagname GLOB 'wiki-*'");
    m = db_int(0, "SELECT COUNT(*) FROM event WHERE type='w'");
    fossil_print("%*s%,d (%,d changes)\n", colWidth, "wiki-pages:", n, m);
    n = db_int(0, "SELECT count(*) FROM tag  /*scan*/"
                  " WHERE tagname GLOB 'tkt-*'");
    m = db_int(0, "SELECT COUNT(*) FROM event WHERE type='t'");
    fossil_print("%*s%,d (%,d changes)\n", colWidth, "tickets:", n, m);
    n = db_int(0, "SELECT COUNT(*) FROM event WHERE type='e'");
    fossil_print("%*s%,d\n", colWidth, "events:", n);
    n = db_int(0, "SELECT COUNT(*) FROM event WHERE type='g'");
    fossil_print("%*s%,d\n", colWidth, "tag-changes:", n);
    z = db_text(0, "SELECT datetime(mtime) || ' - about ' ||"
                   " CAST(julianday('now') - mtime AS INTEGER)"
                   " || ' days ago' FROM event "
                   " ORDER BY mtime DESC LIMIT 1");
    fossil_print("%*s%s\n", colWidth, "latest-change:", z);
  }
  n = db_int(0, "SELECT julianday('now') - (SELECT min(mtime) FROM event)"
                " + 0.99");
  fossil_print("%*s%,d days or approximately %.2f years.\n",
               colWidth, "project-age:", n, n/365.2425);
  p = db_get("project-code", 0);
  if( p ){
    fossil_print("%*s%s\n", colWidth, "project-id:", p);
  }
#if 0
  /* Server-id is not useful information any more */
  fossil_print("%*s%s\n", colWidth, "server-id:", db_get("server-code", 0));
#endif
  fossil_print("%*s%s\n", colWidth, "schema-version:", g.zAuxSchema);
  if( !omitVers ){
    fossil_print("%*s%s %s [%s] (%s)\n",
                 colWidth, "fossil-version:",
                 MANIFEST_DATE, MANIFEST_VERSION, RELEASE_VERSION,
                 COMPILER_NAME);
    fossil_print("%*s%.19s [%.10s] (%s)\n",
                 colWidth, "sqlite-version:",
                 sqlite3_sourceid(), &sqlite3_sourceid()[20],
                 sqlite3_libversion());
  }
  fossil_print("%*s%,d pages, %d bytes/pg, %,d free pages, "
               "%s, %s mode\n",
               colWidth, "database-stats:",
               db_int(0, "PRAGMA repository.page_count"),
               db_int(0, "PRAGMA repository.page_size"),
               db_int(0, "PRAGMA repository.freelist_count"),
               db_text(0, "PRAGMA repository.encoding"),
               db_text(0, "PRAGMA repository.journal_mode"));
  if( dbCheck ){
    fossil_print("%*s%s\n", colWidth, "database-check:",
                 db_text(0, "PRAGMA quick_check(1)"));
  }
}

/*
** WEBPAGE: urllist
**
** Show ways in which this repository has been accessed
*/
void urllist_page(void){
  Stmt q;
  int cnt;
  int showAll = P("all")!=0;
  int nOmitted;
  sqlite3_int64 iNow;
  char *zRemote;
  login_check_credentials();
  if( !g.perm.Admin ){ login_needed(0); return; }

  style_header("URLs and Checkouts");
  style_adunit_config(ADUNIT_RIGHT_OK);
  style_submenu_element("Stat", "stat");
  style_submenu_element("Schema", "repo_schema");
  iNow = db_int64(0, "SELECT strftime('%%s','now')");
  cgi_printf("<div class=\"section\">URLs</div>\n"
         "<table border=\"0\" width='100%%'>\n");
  db_prepare(&q, "SELECT substr(name,9), datetime(mtime,'unixepoch'), mtime"
                 "  FROM config WHERE name GLOB 'baseurl:*' ORDER BY 3 DESC");
  cnt = 0;
  nOmitted = 0;
  while( db_step(&q)==SQLITE_ROW ){
    if( !showAll && db_column_int64(&q,2)<(iNow - 3600*24*30) && cnt>8 ){
      nOmitted++;
    }else{
      cgi_printf("<tr><td width='100%%'>%h</td>\n"
             "<td><nobr>%h</nobr></td></tr>\n",(db_column_text(&q,0)),(db_column_text(&q,1)));
    }
    cnt++;
  }
  db_finalize(&q);
  if( cnt==0 ){
    cgi_printf("<tr><td>(none)</td>\n");
  }else if( nOmitted ){
    cgi_printf("<tr><td><a href=\"urllist?all\"><i>Show %d more...</i></a>\n",(nOmitted));
  }
  cgi_printf("</table>\n"
         "<div class=\"section\">Checkouts</div>\n"
         "<table border=\"0\" width='100%%'>\n");
  db_prepare(&q, "SELECT substr(name,7), datetime(mtime,'unixepoch')"
                 "  FROM config WHERE name GLOB 'ckout:*' ORDER BY 2 DESC");
  cnt = 0;
  while( db_step(&q)==SQLITE_ROW ){
    const char *zPath = db_column_text(&q,0);
    if( vfile_top_of_checkout(zPath) ){
      cgi_printf("<tr><td width='100%%'>%h</td>\n"
             "<td><nobr>%h</nobr></td></tr>\n",(zPath),(db_column_text(&q,1)));
    }
    cnt++;
  }
  db_finalize(&q);
  if( cnt==0 ){
    cgi_printf("<tr><td>(none)</td>\n");
  }
  cgi_printf("</table>\n");
  zRemote = db_text(0, "SELECT value FROM config WHERE name='last-sync-url'");
  if( zRemote ){
    cgi_printf("<div class=\"section\">Last Sync URL</div>\n");
    if( sqlite3_strlike("http%", zRemote, 0)==0 ){
      UrlData x;
      url_parse_local(zRemote, URL_OMIT_USER, &x);
      cgi_printf("<p><a href='%h'>%h</a>\n",(x.canonical),(zRemote));
    }else{
      cgi_printf("<p>%h</p>\n",(zRemote));
    }
    cgi_printf("</div>\n");
  }
  style_footer();
}

/*
** WEBPAGE: repo_schema
**
** Show the repository schema
*/
void repo_schema_page(void){
  Stmt q;
  Blob sql;
  const char *zArg = P("n");
  login_check_credentials();
  if( !g.perm.Admin ){ login_needed(0); return; }

  style_header("Repository Schema");
  style_adunit_config(ADUNIT_RIGHT_OK);
  style_submenu_element("Stat", "stat");
  style_submenu_element("URLs", "urllist");
  if( sqlite3_compileoption_used("ENABLE_DBSTAT_VTAB") ){
    style_submenu_element("Table Sizes", "repo-tabsize");
  }
  blob_init(&sql,
    "SELECT sql FROM repository.sqlite_master WHERE sql IS NOT NULL", -1);
  if( zArg ){
    style_submenu_element("All", "repo_schema");
    blob_appendf(&sql, " AND (tbl_name=%Q OR name=%Q)", zArg, zArg);
  }
  blob_appendf(&sql, " ORDER BY tbl_name, type<>'table', name");
  db_prepare(&q, "%s", blob_str(&sql)/*safe-for-%s*/);
  blob_reset(&sql);
  cgi_printf("<pre>\n");
  while( db_step(&q)==SQLITE_ROW ){
    cgi_printf("%h;\n",(db_column_text(&q, 0)));
  }
  cgi_printf("</pre>\n");
  db_finalize(&q);
  if( db_table_exists("repository","sqlite_stat1") ){
    if( zArg ){
      db_prepare(&q,
        "SELECT tbl, idx, stat FROM repository.sqlite_stat1"
        " WHERE tbl LIKE %Q OR idx LIKE %Q"
        " ORDER BY tbl, idx", zArg, zArg);

      cgi_printf("<hr>\n"
             "<pre>\n");
      while( db_step(&q)==SQLITE_ROW ){
        const char *zTab = db_column_text(&q,0);
        const char *zIdx = db_column_text(&q,1);
        const char *zStat = db_column_text(&q,2);
        cgi_printf("INSERT INTO sqlite_stat1 VALUES('%h','%h','%h');\n",(zTab),(zIdx),(zStat));
      }
      cgi_printf("</pre>\n");
      db_finalize(&q);
    }else{
      style_submenu_element("Stat1","repo_stat1");
    }
  }
  style_footer();
}

/*
** WEBPAGE: repo_stat1
**
** Show the sqlite_stat1 table for the repository schema
*/
void repo_stat1_page(void){
  login_check_credentials();
  if( !g.perm.Admin ){ login_needed(0); return; }

  style_header("Repository STAT1 Table");
  style_adunit_config(ADUNIT_RIGHT_OK);
  style_submenu_element("Stat", "stat");
  style_submenu_element("Schema", "repo_schema");
  if( db_table_exists("repository","sqlite_stat1") ){
    Stmt q;
    db_prepare(&q,
      "SELECT tbl, idx, stat FROM repository.sqlite_stat1"
      " ORDER BY tbl, idx");
    cgi_printf("<pre>\n");
    while( db_step(&q)==SQLITE_ROW ){
      const char *zTab = db_column_text(&q,0);
      const char *zIdx = db_column_text(&q,1);
      const char *zStat = db_column_text(&q,2);
      char *zUrl = href("%R/repo_schema?n=%t",zTab);
      cgi_printf("INSERT INTO sqlite_stat1 VALUES('%z%h</a>','%h','%h');\n",(zUrl),(zTab),(zIdx),(zStat));
    }
    cgi_printf("</pre>\n");
    db_finalize(&q);
  }
  style_footer();
}

/*
** WEBPAGE: repo-tabsize
**
** Show relative sizes of tables in the repository database.
*/
void repo_tabsize_page(void){
  int nPageFree;
  sqlite3_int64 fsize;
  char zBuf[100];

  login_check_credentials();
  if( !g.perm.Read ){ login_needed(g.anon.Read); return; }
  style_header("Repository Table Sizes");
  style_adunit_config(ADUNIT_RIGHT_OK);
  style_submenu_element("Stat", "stat");
  if( g.perm.Admin ){
    style_submenu_element("Schema", "repo_schema");
  }
  db_multi_exec(
    "CREATE TEMP TABLE trans(name TEXT PRIMARY KEY,tabname TEXT)WITHOUT ROWID;"
    "INSERT INTO trans(name,tabname)"
    "   SELECT name, tbl_name FROM repository.sqlite_master;"
    "CREATE TEMP TABLE piechart(amt REAL, label TEXT);"
    "INSERT INTO piechart(amt,label)"
    "  SELECT count(*), "
    "  coalesce((SELECT tabname FROM trans WHERE trans.name=dbstat.name),name)"
    "    FROM dbstat('repository')"
    "   GROUP BY 2 ORDER BY 2;"
  );
  nPageFree = db_int(0, "PRAGMA repository.freelist_count");
  if( nPageFree>0 ){
    db_multi_exec(
      "INSERT INTO piechart(amt,label) VALUES(%d,'freelist')",
      nPageFree
    );
  }
  fsize = file_size(g.zRepositoryName, ExtFILE);
  approxSizeName(sizeof(zBuf), zBuf, fsize);
  cgi_printf("<h2>Repository Size: %s</h2>\n"
         "<center><svg width='800' height='500'>\n",(zBuf));
  piechart_render(800,500,PIE_OTHER|PIE_PERCENT);
  cgi_printf("</svg></center>\n");

  if( g.localOpen ){
    db_multi_exec(
      "DELETE FROM trans;"
      "INSERT INTO trans(name,tabname)"
      "   SELECT name, tbl_name FROM localdb.sqlite_master;"
      "DELETE FROM piechart;"
      "INSERT INTO piechart(amt,label)"
      "  SELECT count(*), "
      " coalesce((SELECT tabname FROM trans WHERE trans.name=dbstat.name),name)"
      "    FROM dbstat('localdb')"
      "   GROUP BY 2 ORDER BY 2;"
    );
    nPageFree = db_int(0, "PRAGMA localdb.freelist_count");
    if( nPageFree>0 ){
      db_multi_exec(
        "INSERT INTO piechart(amt,label) VALUES(%d,'freelist')",
        nPageFree
      );
    }
    fsize = file_size(g.zLocalDbName, ExtFILE);
    approxSizeName(sizeof(zBuf), zBuf, fsize);
    cgi_printf("<h2>%h Size: %s</h2>\n"
           "<center><svg width='800' height='500'>\n",(file_tail(g.zLocalDbName)),(zBuf));
    piechart_render(800,500,PIE_OTHER|PIE_PERCENT);
    cgi_printf("</svg></center>\n");
  }
  style_footer();
}

/*
** Gather statistics on artifact types, counts, and sizes.
**
** Only populate the artstat.atype field if the bWithTypes parameter is true.
*/
static void gather_artifact_stats(int bWithTypes){
  static const char zSql[] = 
    "CREATE TEMP TABLE artstat(\n"
    "  id INTEGER PRIMARY KEY,\n"
    "  atype TEXT,\n"
    "  isDelta BOOLEAN,\n"
    "  szExp,\n"
    "  szCmpr\n"
    ");\n"
    "INSERT INTO artstat(id,atype,isDelta,szExp,szCmpr)\n"
    "   SELECT blob.rid, NULL,\n"
    "          EXISTS(SELECT 1 FROM delta WHERE delta.rid=blob.rid),\n"
    "          size, length(content)\n"
    "     FROM blob\n"
    "    WHERE content IS NOT NULL;\n"
  ;
  static const char zSql2[] = 
    "UPDATE artstat SET atype='file'\n"
    " WHERE id IN (SELECT fid FROM mlink)\n"
    "   AND atype IS NULL;\n"
    "UPDATE artstat SET atype='manifest'\n"
    " WHERE id IN (SELECT objid FROM event WHERE type='ci') AND atype IS NULL;\n"
    "UPDATE artstat SET atype='cluster'\n"
    " WHERE atype IS NULL\n"
    "   AND id IN (SELECT rid FROM tagxref\n"
    "               WHERE tagid=(SELECT tagid FROM tag\n"
    "                             WHERE tagname='cluster'));\n"
    "UPDATE artstat SET atype='ticket'\n"
    " WHERE atype IS NULL\n"
    "   AND id IN (SELECT rid FROM tagxref\n"
    "               WHERE tagid IN (SELECT tagid FROM tag\n"
    "                             WHERE tagname GLOB 'tkt-*'));\n"
    "UPDATE artstat SET atype='wiki'\n"
    " WHERE atype IS NULL\n"
    "   AND id IN (SELECT rid FROM tagxref\n"
    "               WHERE tagid IN (SELECT tagid FROM tag\n"
    "                             WHERE tagname GLOB 'wiki-*'));\n"
    "UPDATE artstat SET atype='technote'\n"
    " WHERE atype IS NULL\n"
    "   AND id IN (SELECT rid FROM tagxref\n"
    "               WHERE tagid IN (SELECT tagid FROM tag\n"
    "                             WHERE tagname GLOB 'event-*'));\n"
    "UPDATE artstat SET atype='attachment'\n"
    " WHERE atype IS NULL\n"
    "   AND id IN (SELECT attachid FROM attachment UNION\n"
    "              SELECT blob.rid FROM attachment JOIN blob ON uuid=src);\n"
    "UPDATE artstat SET atype='tag'\n"
    " WHERE atype IS NULL\n"
    "   AND id IN (SELECT srcid FROM tagxref);\n"
    "UPDATE artstat SET atype='tag'\n"
    " WHERE atype IS NULL\n"
    "   AND id IN (SELECT objid FROM event WHERE type='g');\n"
    "UPDATE artstat SET atype='unused' WHERE atype IS NULL;\n"
  ;
  db_multi_exec("%s", zSql/*safe-for-%s*/);
  if( bWithTypes ){
    db_multi_exec("%s", zSql2/*safe-for-%s*/);
  }
}

/*
** Output text "the largest N artifacts".  Make this text a hyperlink
** to bigbloblist if N is not too big.
*/
static void largest_n_artifacts(int N){
  if( N>250 ){
    cgi_printf("(the largest %,d artifacts)\n",(N));
  }else{
    cgi_printf("(the <a href='%R/bigbloblist?n=%d'>largest %d artifacts</a>)\n",(N),(N));
  }
}

/*
** WEBPAGE: artifact_stats
**
** Show information about the sizes of artifacts in this repository
*/
void artifact_stats_page(void){
  Stmt q;
  int nTotal = 0;            /* Total number of artifacts */
  int nDelta = 0;            /* Total number of deltas */
  int nFull = 0;             /* Total number of full-texts */
  double avgCmpr = 0.0;      /* Average size of an artifact after compression */
  double avgExp = 0.0;       /* Average size of an uncompressed artifact */
  int mxCmpr = 0;            /* Maximum compressed artifact size */
  int mxExp = 0;             /* Maximum uncompressed artifact size */
  sqlite3_int64 sumCmpr = 0; /* Total size of all compressed artifacts */
  sqlite3_int64 sumExp = 0;  /* Total size of all expanded artifacts */
  sqlite3_int64 sz1pct = 0;  /* Space used by largest 1% */
  sqlite3_int64 sz10pct = 0; /* Space used by largest 10% */
  sqlite3_int64 sz25pct = 0; /* Space used by largest 25% */
  sqlite3_int64 sz50pct = 0; /* Space used by largest 50% */
  int n50pct = 0;            /* Artifacts using the first 50% of space */
  int n;                     /* Loop counter */
  int medCmpr = 0;           /* Median compressed artifact size */
  int medExp = 0;            /* Median expanded artifact size */
  int med;
  double r;

  login_check_credentials();

  /* These stats are expensive to compute.  To disable them for
  ** user without check-in privileges, to prevent excessive usage by
  ** robots and random passers-by on the internet
  */
  if( !g.perm.Write ){
    login_needed(g.anon.Admin);
    return;
  }

  style_header("Artifact Statistics");
  style_submenu_element("Repository Stats", "stat");
  style_submenu_element("Artifact List", "bloblist");
  gather_artifact_stats(1);

  db_prepare(&q,
    "SELECT count(*), sum(isDelta), max(szCmpr),"
    "       max(szExp), sum(szCmpr), sum(szExp)"
    "  FROM artstat"
  );
  db_step(&q);
  nTotal = db_column_int(&q,0);
  nDelta = db_column_int(&q,1);
  nFull = nTotal - nDelta;
  mxCmpr = db_column_int(&q, 2);
  mxExp = db_column_int(&q, 3);
  sumCmpr = db_column_int64(&q, 4);
  sumExp = db_column_int64(&q, 5);
  db_finalize(&q);
  if( nTotal==0 ){
    cgi_printf("No artifacts in this repository!\n");
    style_footer();
    return;
  }
  avgCmpr = (double)sumCmpr/nTotal;
  avgExp = (double)sumExp/nTotal;

  db_prepare(&q, "SELECT szCmpr FROM artstat ORDER BY 1 DESC");
  r = 0;
  n = 0;
  while( db_step(&q)==SQLITE_ROW ){
    r += db_column_int(&q, 0);
    if( n50pct==0 && r>=sumCmpr/2 ) n50pct = n;
    if( n==(nTotal+99)/100 ) sz1pct = r;
    if( n==(nTotal+9)/10 ) sz10pct = r;
    if( n==(nTotal+4)/5 ) sz25pct = r;
    if( n==(nTotal+1)/2 ){ sz50pct = r; medCmpr = db_column_int(&q,0); }
    n++;
  }
  db_finalize(&q);

  cgi_printf("<h1>Overall Artifact Size Statistics:</h1>\n"
         "<table class=\"label-value\">\n"
         "<tr><th>Number of artifacts:</th><td>%,d</td></tr>\n"
         "<tr><th>Number of deltas:</th>"
         "<td>%,d (%d%%)</td></tr>\n"
         "<tr><th>Number of full-text:</th><td>%,d "
         "(%d%%)</td></tr>\n",(nTotal),(nDelta),(nDelta*100/nTotal),(nFull),(nFull*100/nTotal));
  medExp = db_int(0, "SELECT szExp FROM artstat ORDER BY szExp"
                     " LIMIT 1 OFFSET %d", nTotal/2);
  cgi_printf("<tr><th>Uncompressed artifact sizes:</th>"
         "<td>largest: %,d, average: %,d, median: %,d</td>\n"
         "<tr><th>Compressed artifact sizes:</th>"
         "<td>largest: %,d, average: %,d, "
         "median: %,d</td>\n",(mxExp),((int)avgExp),(medExp),(mxCmpr),((int)avgCmpr),(medCmpr));

  db_prepare(&q,
    "SELECT avg(szCmpr), max(szCmpr) FROM artstat WHERE isDelta"
  );
  if( db_step(&q)==SQLITE_ROW ){
    int mxDelta = db_column_int(&q,1);
    double avgDelta = db_column_double(&q,0);
    med = db_int(0, "SELECT szCmpr FROM artstat WHERE isDelta ORDER BY szCmpr"
                    " LIMIT 1 OFFSET %d", nDelta/2);
    cgi_printf("<tr><th>Delta artifact sizes:</th>"
           "<td>largest: %,d, average: %,d, "
           "median: %,d</td>\n",(mxDelta),((int)avgDelta),(med));
  }
  db_finalize(&q);
  r = db_double(0.0, "SELECT avg(szCmpr) FROM artstat WHERE NOT isDelta;");
  med = db_int(0, "SELECT szCmpr FROM artstat WHERE NOT isDelta ORDER BY szCmpr"
                  " LIMIT 1 OFFSET %d", nFull/2);
  cgi_printf("<tr><th>Full-text artifact sizes:</th>\n"
         "<td>largest: %,d, average: %,d, median: %,d</td>\n"
         "</table>\n",(mxCmpr),((int)r),(med));

  cgi_printf("<h1>Artifact size distribution facts:</h1>\n"
         "<ol>\n"
         "<li><p>The largest %.2f%% of artifacts\n",(n50pct*100.0/nTotal));
  largest_n_artifacts(n50pct);
  cgi_printf("use 50%% of the total artifact space.\n"
         "<li><p>The largest 1%% of artifacts\n");
  largest_n_artifacts((nTotal+99)/100);
  cgi_printf("use %lld%% of the total artifact space.\n"
         "<li><p>The largest 10%% of artifacts\n",(sz1pct*100/sumCmpr));
  largest_n_artifacts((nTotal+9)/10);
  cgi_printf("use %lld%% of the total artifact space.\n"
         "<li><p>The largest 25%% of artifacts\n",(sz10pct*100/sumCmpr));
  largest_n_artifacts((nTotal+4)/5);
  cgi_printf("use %lld%% of the total artifact space.\n"
         "<li><p>The largest 50%% of artifacts\n",(sz25pct*100/sumCmpr));
  largest_n_artifacts((nTotal+1)/2);
  cgi_printf("use %lld%% of the total artifact space.\n"
         "</ol>\n",(sz50pct*100/sumCmpr));

  cgi_printf("<h1>Artifact Sizes By Type:</h1>\n");
  db_prepare(&q,
    "SELECT atype, count(*), sum(isDelta), sum(szCmpr), sum(szExp)"
    "  FROM artstat GROUP BY 1"
    " UNION ALL "
    "SELECT 'ALL', count(*), sum(isDelta), sum(szCmpr), sum(szExp)"
    "  FROM artstat"
    " ORDER BY 4;"
  );
  cgi_printf("<table class='sortable' border='1' "
         "data-column-types='tkkkkk' data-init-sort='5'>\n"
         "<thead><tr>\n"
         "<th>Artifact Type</th>\n"
         "<th>Count</th>\n"
         "<th>Full-Text</th>\n"
         "<th>Delta</th>\n"
         "<th>Compressed Size</th>\n"
         "<th>Uncompressed Size</th>\n"
         "</tr></thead><tbody>\n");
  while( db_step(&q)==SQLITE_ROW ){
    const char *zType = db_column_text(&q, 0);
    int nTotal = db_column_int(&q, 1);
    int nDelta = db_column_int(&q, 2);
    int nFull = nTotal - nDelta;
    sqlite3_int64 szCmpr = db_column_int64(&q, 3);
    sqlite3_int64 szExp = db_column_int64(&q, 4);
    cgi_printf("<tr><td>%h</td>\n"
           "<td data-sortkey='%08x' align='right'>%,d</td>\n"
           "<td data-sortkey='%08x' align='right'>%,d</td>\n"
           "<td data-sortkey='%08x' align='right'>%,d</td>\n"
           "<td data-sortkey='%016x' align='right'>%,lld</td>\n"
           "<td data-sortkey='%016x' align='right'>%,lld</td>\n",(zType),(nTotal),(nTotal),(nFull),(nFull),(nDelta),(nDelta),(szCmpr),(szCmpr),(szExp),(szExp));
  }
  cgi_printf("</tbody></table>\n");
  db_finalize(&q);

  if( db_exists("SELECT 1 FROM artstat WHERE atype='unused'") ){
    cgi_printf("<h1>Unused Artifacts:</h1>\n");
    db_prepare(&q,
      "SELECT artstat.id, blob.uuid, user.login,"
      "       datetime(rcvfrom.mtime), rcvfrom.rcvid"
      "  FROM artstat JOIN blob ON artstat.id=blob.rid"
      "       LEFT JOIN rcvfrom USING(rcvid)"
      "       LEFT JOIN user USING(uid)"
      " WHERE atype='unused'"
    );
    cgi_printf("<table class='sortable' border='1' "
           "data-column-types='ntttt' data-init-sort='0'>\n"
           "<thead><tr>\n"
           "<th>RecordID</th>\n"
           "<th>Hash</th>\n"
           "<th>User</th>\n"
           "<th>Date</th>\n"
           "<th>RcvID</th>\n"
           "</tr></thead><tbody>\n");
    while( db_step(&q)==SQLITE_ROW ){
      int rid = db_column_int(&q, 0);
      const char *zHash = db_column_text(&q, 1);
      const char *zUser = db_column_text(&q, 2);
      const char *zDate = db_column_text(&q, 3);
      int iRcvid = db_column_int(&q, 4);
      cgi_printf("<tr><td>%d</td>\n"
             "<td>%z%S</a></td>\n"
             "<td>%h</td>\n"
             "<td>%h</td>\n"
             "<td>%z%d</a></td></tr>\n",(rid),(href("%R/info/%!S",zHash)),(zHash),(zUser),(zDate),(href("%R/rcvfrom?rcvid=%d",iRcvid)),(iRcvid));
    }
    cgi_printf("</tbody></table></div>\n");
    db_finalize(&q);
  }
  style_table_sorter();
  style_footer();
}
