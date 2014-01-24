#! /usr/bin/env python
# -*- coding: utf-8 -*-

import logging
import os
from os.path import dirname, join
import sys
import xmlrpclib
import getpass, subprocess, re
import markup


logging.basicConfig(level=logging.INFO, stream=sys.stdout)


BOT_USERNAME = "TodoPutInUsername"
WIKI_URL = "http://localhost:8080"

titles = []

def connect():
    wiki = xmlrpclib.ServerProxy(WIKI_URL + "?action=xmlrpc2", allow_none=True)
    auth_token = wiki.getAuthToken(BOT_USERNAME, "geheimeswort")
    multi_call = xmlrpclib.MultiCall(wiki)
    multi_call.applyAuthToken(auth_token)
    return multi_call

def get_all_titles():
    multi_call = connect()
    multi_call.getAllPages()
    result = []
    entry1, entry2 = multi_call()
    return entry2

#returns the text of the page titled title
def get_page(title):
    multi_call = connect()
    multi_call.getPage(title)
    return multi_call()[1]

def send_pages(pages):
    multi_call = connect()
    for page_name, page_text in pages:
        multi_call.putPage(page_name, page_text)
    try:
        for entry in multi_call():
            logging.info(repr(entry))
    except xmlrpclib.Fault as error:
        print error
    except:
        print "Unexpected error:", sys.exc_info()[0]
        raise
    else:
        print "Update successful"

#helper function for insert_wiki_links
def make_doc_link(m):
    s = m.group(0)
    key = s[1:-1]
    result = s[0] + "[[DOC/" + key + "|" + key + "]]" + s[-1]
    return result

#helper function for insert_wiki_links
def make_other_link(m):
    s = m.group(0)
    key = s[1:-1]
    result = s[0] + "[[" + key + "|" + key + "]]" + s[-1]
    return result

def insert_wiki_links(text):
    doctitles = [title[4:] for title in titles if title.startswith("DOC/")]
    for key in doctitles:
        text = re.sub("\W" + key + "\W", make_doc_link, text)
    othertitles = [title for title in titles if not title.startswith("DOC/")]
    for key in othertitles:
        text = re.sub("\W" + key + "\W", make_other_link, text)
    return text

if __name__ == '__main__':
    titles = get_all_titles()
    #update the planner executable if necessary
    build_dir = "../../src/search"
    cwd = os.getcwd()
    os.chdir(build_dir)
    os.system("make")
    os.chdir(cwd)

    #retrieve documentation output from the planner
    p = subprocess.Popen(["../../src/search/downward-1", "--help", "--txt2tags"], stdout=subprocess.PIPE)
    out = p.communicate()[0]

    #split the output into tuples (category, text)
    pagesplitter = re.compile(r'>>>>CATEGORY: ([\w\s]+?)<<<<(.+?)>>>>CATEGORYEND<<<<', re.DOTALL)
    pages = pagesplitter.findall(out)

    doctitles = [title for title in titles if title.startswith("DOC/")]

    #format and send to wiki:
    pagetitles = [];
    for page in pages:
        title = page[0]
        if(title == "Synergy"):
            title = "LAMAFFSynergy"
        title = "DOC/"+title
        pagetitles.append(title)
        text = page[1]
        text = "<<TableOfContents>>\n" + text
        doc = markup.Document()
        doc.add_text(text)
        text = doc.render("moin")
        #remove anything before the table of contents (to get rid of the date):
        text = text[text.find("<<TableOfContents>>"):]
        text = insert_wiki_links(text)
        #if a page with this title exists, re-use the part preceding the table of contents:
        introduction = ""
        if title in doctitles:
            old_text = get_page(title)
            introduction = old_text[0:old_text.find("<<TableOfContents>>")]
        text = introduction + text
        print "updating ", title
        send_pages([(title, text)])

    #update overview page:
    title = "DOC/Overview"
    text = "";
    for pagetitle in pagetitles:
        text = text + "\n * [[" + pagetitle + "]]"
    print "updating ", title
    send_pages([(title, text)])
    print "Done."
    print "You may need to run this script again to insert links to newly created pages."

