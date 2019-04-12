/* DO NOT EDIT
** This code is generated automatically using 'mkcss.c'
*/
const struct strctCssDefaults {
  const char *elementClass;  /* Name of element needed */
  const char *value;         /* CSS text */
} cssDefaultList[] = {
  { "div.sidebox",
    "  float: right;\n"
    "  background-color: white;\n"
    "  border-width: medium;\n"
    "  border-style: double;\n"
    "  margin: 10px;\n"
  },
  { "div.sideboxTitle",
    "  display: inline;\n"
    "  font-weight: bold;\n"
  },
  { "div.sideboxDescribed",
    "  display: inline;\n"
    "  font-weight: bold;\n"
  },
  { "span.disabled",
    "  color: red;\n"
  },
  { "table.timelineTable",
    "  border-spacing: 0px 2px;\n"
  },
  { ".timelineDate",
    "  white-space: nowrap;\n"
  },
  { "span.timelineDisabled",
    "  font-style: italic;\n"
    "  font-size: small;\n"
  },
  { "tr.timelineCurrent",
    "  padding: .1em .2em;\n"
    "  border: 1px dashed #446979;\n"
    "  box-shadow: 1px 1px 4px rgba(0, 0, 0, 0.5);\n"
  },
  { "tr.timelineSelected",
    "  padding: .1em .2em;\n"
    "  border: 2px solid lightgray;\n"
    "  background-color: #ffc;\n"
    "  box-shadow: 1px 1px 4px rgba(0, 0, 0, 0.5);\n"
  },
  { "tr.timelineSelected td",
    "  border-radius: 0;\n"
    "  border-width: 0;\n"
  },
  { "tr.timelineCurrent td",
    "  border-radius: 0;\n"
    "  border-width: 0;\n"
  },
  { "span.timelineLeaf",
    "  font-weight: bold;\n"
  },
  { "span.timelineHistDsp",
    "  font-weight: bold;\n"
  },
  { "td.timelineTime",
    "  vertical-align: top;\n"
    "  text-align: right;\n"
    "  white-space: nowrap;\n"
  },
  { "td.timelineGraph",
    "  width: 20px;\n"
    "  text-align: left;\n"
    "  vertical-align: top;\n"
  },
  { "span.timelineCompactComment",
    "  cursor: pointer;\n"
  },
  { "span.timelineEllipsis",
    "  cursor: pointer;\n"
  },
  { ".timelineModernCell, .timelineColumnarCell, .timelineDetailCell",
    "  vertical-align: top;\n"
    "  text-align: left;\n"
    "  padding: 0.75em;\n"
    "  border-radius: 1em;\n"
  },
  { ".timelineModernCell[id], .timelineColumnarCell[id], .timelineDetailCell[id]",
    "  background-color: #efefef;\n"
  },
  { ".timelineModernDetail",
    "  font-size: 80%;\n"
    "  text-align: right;\n"
    "  float: right;\n"
    "  opacity: 0.75;\n"
    "  margin-top: 0.5em;\n"
    "  margin-left: 1em;\n"
  },
  { ".tl-canvas",
    "  margin: 0 6px 0 10px;\n"
  },
  { ".tl-rail",
    "  width: 18px;\n"
  },
  { ".tl-mergeoffset",
    "  width: 2px;\n"
  },
  { ".tl-nodemark",
    "  margin-top: 5px;\n"
  },
  { ".tl-node",
    "  width: 10px;\n"
    "  height: 10px;\n"
    "  border: 1px solid #000;\n"
    "  background: #fff;\n"
    "  cursor: pointer;\n"
  },
  { ".tl-node.leaf:after",
    "  content: '';\n"
    "  position: absolute;\n"
    "  top: 3px;\n"
    "  left: 3px;\n"
    "  width: 4px;\n"
    "  height: 4px;\n"
    "  background: #000;\n"
  },
  { ".tl-node.sel:after",
    "  content: '';\n"
    "  position: absolute;\n"
    "  top: 2px;\n"
    "  left: 2px;\n"
    "  width: 6px;\n"
    "  height: 6px;\n"
    "  background: red;\n"
  },
  { ".tl-arrow",
    "  width: 0;\n"
    "  height: 0;\n"
    "  transform: scale(.999);\n"
    "  border: 0 solid transparent;\n"
  },
  { ".tl-arrow.u",
    "  margin-top: -1px;\n"
    "  border-width: 0 3px;\n"
    "  border-bottom: 7px solid #000;\n"
  },
  { ".tl-arrow.u.sm",
    "  border-bottom: 5px solid #000;\n"
  },
  { ".tl-line",
    "  background: #000;\n"
    "  width: 2px;\n"
  },
  { ".tl-arrow.merge",
    "  height: 1px;\n"
    "  border-width: 2px 0;\n"
  },
  { ".tl-arrow.merge.l",
    "  border-right: 3px solid #000;\n"
  },
  { ".tl-arrow.merge.r",
    "  border-left: 3px solid #000;\n"
  },
  { ".tl-line.merge",
    "  width: 1px;\n"
  },
  { ".tl-arrow.cherrypick",
    "  height: 1px;\n"
    "  border-width: 2px 0;\n"
  },
  { ".tl-arrow.cherrypick.l",
    "  border-right: 3px solid #000;\n"
  },
  { ".tl-arrow.cherrypick.r",
    "  border-left: 3px solid #000;\n"
  },
  { ".tl-line.cherrypick.h",
    "  width: 0px;\n"
    "  border-top: 1px dashed #000;\n"
    "  border-left: 0px dashed #000;\n"
    "  background: rgba(255,255,255,0);\n"
  },
  { ".tl-line.cherrypick.v",
    "  width: 0px;\n"
    "  border-top: 0px dashed #000;\n"
    "  border-left: 1px dashed #000;\n"
    "  background: rgba(255,255,255,0);\n"
  },
  { ".tl-arrow.warp",
    "  margin-left: 1px;\n"
    "  border-width: 3px 0;\n"
    "  border-left: 7px solid #600000;\n"
  },
  { ".tl-line.warp",
    "  background: #600000;\n"
  },
  { "span.tagDsp",
    "  font-weight: bold;\n"
  },
  { "span.wikiError",
    "  font-weight: bold;\n"
    "  color: red;\n"
  },
  { "span.infoTagCancelled",
    "  font-weight: bold;\n"
    "  text-decoration: line-through;\n"
  },
  { "span.infoTag",
    "  font-weight: bold;\n"
  },
  { "span.wikiTagCancelled",
    "  text-decoration: line-through;\n"
  },
  { "div.columns",
    "  padding: 0 2em 0 2em;\n"
    "  max-width: 1000px;\n"
  },
  { "div.columns > ul",
    "  margin: 0;\n"
    "  padding: 0 0 0 1em;\n"
  },
  { "div.columns > ul li:first-child",
    "  margin-top:0px;\n"
  },
  { ".columns li",
    "  break-inside: avoid;\n"
    "  page-break-inside: avoid;\n"
  },
  { ".filetree",
    "  margin: 1em 0;\n"
    "  line-height: 1.5;\n"
  },
  { ".filetree > ul",
    "  display: inline-block;\n"
  },
  { ".filetree ul",
    "  margin: 0;\n"
    "  padding: 0;\n"
    "  list-style: none;\n"
  },
  { ".filetree ul.collapsed",
    "  display: none;\n"
  },
  { ".filetree ul ul",
    "  position: relative;\n"
    "  margin: 0 0 0 21px;\n"
  },
  { ".filetree li",
    "  position: relative;\n"
    "  margin: 0;\n"
    "  padding: 0;\n"
  },
  { ".filetree li li:before",
    "  content: '';\n"
    "  position: absolute;\n"
    "  top: -.8em;\n"
    "  left: -14px;\n"
    "  width: 14px;\n"
    "  height: 1.5em;\n"
    "  border-left: 2px solid #aaa;\n"
    "  border-bottom: 2px solid #aaa;\n"
  },
  { ".filetree li > ul:before",
    "  content: '';\n"
    "  position: absolute;\n"
    "  top: -1.5em;\n"
    "  bottom: 0;\n"
    "  left: -35px;\n"
    "  border-left: 2px solid #aaa;\n"
  },
  { ".filetree li.last > ul:before",
    "  display: none;\n"
  },
  { ".filetree a",
    "  position: relative;\n"
    "  z-index: 1;\n"
    "  display: table-cell;\n"
    "  min-height: 16px;\n"
    "  padding-left: 21px;\n"
    "  background-image: url(data:image/gif;base64,R0lGODlhEAAQAJEAAP\\/\\/\\/yEhIf\\/\\/\\/wAAACH5BAEHAAIALAAAAAAQABAAAAIvlIKpxqcfmgOUvoaqDSCxrEEfF14GqFXImJZsu73wepJzVMNxrtNTj3NATMKhpwAAOw==);\n"
    "  background-position: center left;\n"
    "  background-repeat: no-repeat;\n"
  },
  { "ul.browser",
    "  list-style-type: none;\n"
    "  padding: 10px;\n"
    "  margin: 0px;\n"
    "  white-space: nowrap;\n"
  },
  { "ul.browser li.file",
    "  background-image: url(data:image/gif;base64,R0lGODlhEAAQAJEAAP\\/\\/\\/yEhIf\\/\\/\\/wAAACH5BAEHAAIALAAAAAAQABAAAAIvlIKpxqcfmgOUvoaqDSCxrEEfF14GqFXImJZsu73wepJzVMNxrtNTj3NATMKhpwAAOw==);\n"
    "  background-repeat: no-repeat;\n"
    "  background-position: 0px center;\n"
    "  padding-left: 20px;\n"
    "  padding-top: 2px;\n"
  },
  { "ul.browser li.dir",
    "  background-image: url(data:image/gif;base64,R0lGODlhEAAQAJEAAP/WVCIiIv\\/\\/\\/wAAACH5BAEHAAIALAAAAAAQABAAAAInlI9pwa3XYniCgQtkrAFfLXkiFo1jaXpo+jUs6b5Z/K4siDu5RPUFADs=);\n"
    "  background-repeat: no-repeat;\n"
    "  background-position: 0px center;\n"
    "  padding-left: 20px;\n"
    "  padding-top: 2px;\n"
  },
  { "div.filetreeline",
    "  display: table;\n"
    "  width: 100%;\n"
    "  white-space: nowrap;\n"
  },
  { ".filetree .dir > div.filetreeline > a",
    "  background-image: url(data:image/gif;base64,R0lGODlhEAAQAJEAAP/WVCIiIv\\/\\/\\/wAAACH5BAEHAAIALAAAAAAQABAAAAInlI9pwa3XYniCgQtkrAFfLXkiFo1jaXpo+jUs6b5Z/K4siDu5RPUFADs=);\n"
  },
  { "div.filetreeage",
    "  display: table-cell;\n"
    "  padding-left: 3em;\n"
    "  text-align: right;\n"
  },
  { "div.filetreeline:hover",
    "  background-color: #eee;\n"
  },
  { "table.login_out",
    "  text-align: left;\n"
    "  margin-right: 10px;\n"
    "  margin-left: 10px;\n"
    "  margin-top: 10px;\n"
  },
  { "div.captcha",
    "  text-align: center;\n"
    "  padding: 1ex;\n"
  },
  { "table.captcha",
    "  margin: auto;\n"
    "  padding: 10px;\n"
    "  border-width: 4px;\n"
    "  border-style: double;\n"
    "  border-color: black;\n"
  },
  { "pre.captcha",
    "  font-size: 50%;\n"
  },
  { "td.login_out_label",
    "  text-align: center;\n"
  },
  { "span.loginError",
    "  color: red;\n"
  },
  { "span.note",
    "  font-weight: bold;\n"
  },
  { "span.textareaLabel",
    "  font-weight: bold;\n"
  },
  { "table.usetupLayoutTable",
    "  outline-style: none;\n"
    "  padding: 0;\n"
    "  margin: 25px;\n"
  },
  { "td.usetupColumnLayout",
    "  vertical-align: top\n"
  },
  { "table.usetupUserList",
    "  outline-style: double;\n"
    "  outline-width: 1px;\n"
    "  padding: 10px;\n"
  },
  { "th.usetupListUser",
    "  text-align: right;\n"
    "  padding-right: 20px;\n"
  },
  { "th.usetupListCap",
    "  text-align: center;\n"
    "  padding-right: 15px;\n"
  },
  { "th.usetupListCon",
    "  text-align: left;\n"
  },
  { "td.usetupListUser",
    "  text-align: right;\n"
    "  padding-right: 20px;\n"
    "  white-space:nowrap;\n"
  },
  { "td.usetupListCap",
    "  text-align: center;\n"
    "  padding-right: 15px;\n"
  },
  { "td.usetupListCon",
    "  text-align: left\n"
  },
  { "div.ueditCapBox",
    "  margin-right: 20px;\n"
    "  margin-bottom: 20px;\n"
  },
  { "td.usetupEditLabel",
    "  text-align: right;\n"
    "  vertical-align: top;\n"
    "  white-space: nowrap;\n"
  },
  { "span.ueditInheritNobody",
    "  color: green;\n"
    "  padding: .2em;\n"
  },
  { "span.ueditInheritDeveloper",
    "  color: red;\n"
    "  padding: .2em;\n"
  },
  { "span.ueditInheritReader",
    "  color: black;\n"
    "  padding: .2em;\n"
  },
  { "span.ueditInheritAnonymous",
    "  color: blue;\n"
    "  padding: .2em;\n"
  },
  { "span.capability",
    "  font-weight: bold;\n"
  },
  { "span.usertype",
    "  font-weight: bold;\n"
  },
  { "span.usertype:before",
    "  content:\"'\";\n"
  },
  { "span.usertype:after",
    "  content:\"'\";\n"
  },
  { "div.selectedText",
    "  font-weight: bold;\n"
    "  color: blue;\n"
    "  background-color: #d5d5ff;\n"
    "  border: 1px blue solid;\n"
  },
  { "p.missingPriv",
    "  color: blue;\n"
  },
  { "span.wikiruleHead",
    "  font-weight: bold;\n"
  },
  { "td.tktDspLabel",
    "  text-align: right;\n"
  },
  { "td.tktDspValue",
    "  text-align: left;\n"
    "  vertical-align: top;\n"
    "  background-color: #d0d0d0;\n"
  },
  { "span.tktError",
    "  color: red;\n"
    "  font-weight: bold;\n"
  },
  { "table.rpteditex",
    "  float: right;\n"
    "  margin: 0;\n"
    "  padding: 0;\n"
    "  width: 125px;\n"
    "  text-align: center;\n"
    "  border-collapse: collapse;\n"
    "  border-spacing: 0;\n"
  },
  { "table.report",
    "  border-collapse:collapse;\n"
    "  border: 1px solid #999;\n"
    "  margin: 1em 0 1em 0;\n"
    "  cursor: pointer;\n"
  },
  { "td.rpteditex",
    "  border-width: thin;\n"
    "  border-color: #000000;\n"
    "  border-style: solid;\n"
  },
  { "div.endContent",
    "  clear: both;\n"
  },
  { "p.generalError",
    "  color: red;\n"
  },
  { "p.tktsetupError",
    "  color: red;\n"
    "  font-weight: bold;\n"
  },
  { "p.xfersetupError",
    "  color: red;\n"
    "  font-weight: bold;\n"
  },
  { "p.thmainError",
    "  color: red;\n"
    "  font-weight: bold;\n"
  },
  { "span.thTrace",
    "  color: red;\n"
  },
  { "p.reportError",
    "  color: red;\n"
    "  font-weight: bold;\n"
  },
  { "blockquote.reportError",
    "  color: red;\n"
    "  font-weight: bold;\n"
  },
  { "p.noMoreShun",
    "  color: blue;\n"
  },
  { "p.shunned",
    "  color: blue;\n"
  },
  { "span.brokenlink",
    "  color: red;\n"
  },
  { "ul.filelist",
    "  margin-top: 3px;\n"
    "  line-height: 100%;\n"
  },
  { "ul.filelist li",
    "  padding-top: 1px;\n"
  },
  { "table.sbsdiffcols",
    "  width: 90%;\n"
    "  border-spacing: 0;\n"
    "  font-size: xx-small;\n"
  },
  { "table.sbsdiffcols td",
    "  padding: 0;\n"
    "  vertical-align: top;\n"
  },
  { "table.sbsdiffcols pre",
    "  margin: 0;\n"
    "  padding: 0;\n"
    "  border: 0;\n"
    "  font-size: inherit;\n"
    "  background: inherit;\n"
    "  color: inherit;\n"
  },
  { "div.difflncol",
    "  padding-right: 1em;\n"
    "  text-align: right;\n"
    "  color: #a0a0a0;\n"
  },
  { "div.difftxtcol",
    "  width: 45em;\n"
    "  overflow-x: auto;\n"
  },
  { "div.diffmkrcol",
    "  padding: 0 1em;\n"
  },
  { "span.diffchng",
    "  background-color: #c0c0ff;\n"
  },
  { "span.diffadd",
    "  background-color: #c0ffc0;\n"
  },
  { "span.diffrm",
    "  background-color: #ffc8c8;\n"
  },
  { "span.diffhr",
    "  display: inline-block;\n"
    "  margin: .5em 0 1em;\n"
    "  color: #0000ff;\n"
  },
  { "span.diffln",
    "  color: #a0a0a0;\n"
  },
  { "span.modpending",
    "  color: #b03800;\n"
    "  font-style: italic;\n"
  },
  { "pre.th1result",
    "  white-space: pre-wrap;\n"
    "  word-wrap: break-word;\n"
  },
  { "pre.th1error",
    "  white-space: pre-wrap;\n"
    "  word-wrap: break-word;\n"
    "  color: red;\n"
  },
  { "pre.textPlain",
    "  white-space: pre-wrap;\n"
    "  word-wrap: break-word;\n"
  },
  { ".statistics-report-graph-line",
    "  background-color: #446979;\n"
  },
  { ".statistics-report-table-events th",
    "  padding: 0 1em 0 1em;\n"
  },
  { ".statistics-report-table-events td",
    "  padding: 0.1em 1em 0.1em 1em;\n"
  },
  { ".statistics-report-row-year",
    "  text-align: left;\n"
  },
  { ".statistics-report-week-number-label",
    "  text-align: right;\n"
    "  font-size: 0.8em;\n"
  },
  { ".statistics-report-week-of-year-list",
    "  font-size: 0.8em;\n"
  },
  { "#usetupEditCapability",
    "  font-weight: bold;\n"
  },
  { "table.adminLogTable",
    "  text-align: left;\n"
  },
  { ".adminLogTable .adminTime",
    "  text-align: left;\n"
    "  vertical-align: top;\n"
    "  white-space: nowrap;\n"
  },
  { ".fileage table",
    "  border-spacing: 0;\n"
  },
  { ".fileage tr:hover",
    "  background-color: #eee;\n"
  },
  { ".fileage td",
    "  vertical-align: top;\n"
    "  text-align: left;\n"
    "  border-top: 1px solid #ddd;\n"
    "  padding-top: 3px;\n"
  },
  { ".fileage td:first-child",
    "  white-space: nowrap;\n"
  },
  { ".fileage td:nth-child(2)",
    "  padding-left: 1em;\n"
    "  padding-right: 1em;\n"
  },
  { ".fileage td:nth-child(3)",
    "  word-wrap: break-word;\n"
    "  max-width: 50%;\n"
  },
  { ".brlist table",
    "  border-spacing: 0;\n"
  },
  { ".brlist table th",
    "  text-align: left;\n"
    "  padding: 0px 1em 0.5ex 0px;\n"
    "  vertical-align: bottom;\n"
  },
  { ".brlist table td",
    "  padding: 0px 2em 0px 0px;\n"
    "  white-space: nowrap;\n"
  },
  { "th.sort:after",
    "  margin-left: .4em;\n"
    "  cursor: pointer;\n"
    "  text-shadow: 0 0 0 #000;\n"
  },
  { "th.sort.none:after",
    "  content: '\\2666';\n"
  },
  { "th.sort.asc:after",
    "  content: '\\2193';\n"
  },
  { "th.sort.desc:after",
    "  content: '\\2191';\n"
  },
  { "span.snippet>mark",
    "  background-color: inherit;\n"
    "  font-weight: bold;\n"
  },
  { "div.searchForm",
    "  text-align: center;\n"
  },
  { "p.searchEmpty",
    "  font-style: italic;\n"
  },
  { ".clutter",
    "  display: none;\n"
  },
  { "table.label-value th",
    "  vertical-align: top;\n"
    "  text-align: right;\n"
    "  padding: 0.2ex 1ex;\n"
  },
  { "table.forum_post",
    "  margin-top: 1ex;\n"
    "  margin-bottom: 1ex;\n"
    "  margin-left: 0;\n"
    "  margin-right: 0;\n"
    "  border-spacing: 0;\n"
  },
  { "span.forum_author",
    "  color: #888;\n"
    "  font-size: 75%;\n"
  },
  { "span.forum_author::after",
    "  content: \" | \";\n"
  },
  { "span.forum_age",
    "  color: #888;\n"
    "  font-size: 85%;\n"
  },
  { "span.forum_buttons",
    "  font-size: 85%;\n"
  },
  { "span.forum_buttons::before",
    "  color: #888;\n"
    "  content: \" | \";\n"
  },
  { "span.forum_npost",
    "  color: #888;\n"
    "  font-size: 75%;\n"
  },
  { "table.forumeditform td",
    "  vertical-align: top;\n"
    "  border-collapse: collapse;\n"
    "  padding: 1px;\n"
  },
  { "div.forum_body p",
    "  margin-top: 0;\n"
  },
  { "td.form_label",
    "  vertical-align: top;\n"
    "  text-align: right;\n"
  },
  { ".debug",
    "  background-color: #ffc;\n"
    "  border: 2px solid #ff0;\n"
  },
  { "div.forumEdit",
    "  border: 1px solid black;\n"
    "  padding-left: 1ex;\n"
    "  padding-right: 1ex;\n"
  },
  { "div.forumHier, div.forumTime",
    "  border: 1px solid black;\n"
    "  padding-left: 1ex;\n"
    "  padding-right: 1ex;\n"
    "  margin-top: 1ex;\n"
  },
  { "div.forumSel",
    "  background-color: #cef;\n"
  },
  { "div.forumObs",
    "  color: #bbb;\n"
  },
  { "#capabilitySummary",
    "  text-align: center;\n"
  },
  { "#capabilitySummary td",
    "  padding-left: 3ex;\n"
    "  padding-right: 3ex;\n"
  },
  { "#capabilitySummary th",
    "  padding-left: 1ex;\n"
    "  padding-right: 1ex;\n"
  },
  { ".capsumOff",
    "  background-color: #bbb;\n"
  },
  { ".capsumRead",
    "  background-color: #bfb;\n"
  },
  { ".capsumWrite",
    "  background-color: #ffb;\n"
  },
  { "label",
    "  white-space: nowrap;\n"
  },
  {0,0}
};
