#! /usr/bin/env python
# -*- coding: utf-8 -*-

import logging
import os
from os.path import dirname, join
import re
import subprocess
import sys
import time
import xmlrpclib

import markup


#how many seconds to wait after a failed requests. Will be doubled after each failed request.
#Don't lower this below ~5, or we may get locked out for an hour.
sleep_time = 10

BOT_USERNAME = "XmlRpcBot"
PASSWORD_FILE = "downward-xmlrpc.secret" # relative to this source file
WIKI_URL = "http://www.fast-downward.org"

titles = []

def read_password():
    path = join(dirname(__file__), PASSWORD_FILE)
    with open(path) as password_file:
        try:
            return password_file.read().strip()
        except IOError, e:
            sys.exit("Could not find password file %s!\nIs it present?"
                     % PASSWORD_FILE)

def connect():
    wiki = xmlrpclib.ServerProxy(WIKI_URL + "?action=xmlrpc2", allow_none=True)
    auth_token = wiki.getAuthToken(BOT_USERNAME, read_password())
    multi_call = xmlrpclib.MultiCall(wiki)
    multi_call.applyAuthToken(auth_token)
    return multi_call

def get_all_titles():
    multi_call = connect()
    multi_call.getAllPages()
    result = []
    entry1, entry2 = multi_call()
    return entry2

def get_pages(titles):
    multi_call = connect()
    for title in titles:
        print title
        multi_call.getPage(title)
    return multi_call()

#returns the text of the page titled title
def get_page(title):
    multi_call = connect()
    multi_call.getPage(title)
    return multi_call()[1]

def send_pages(pages):
    multi_call = connect()
    for page_name, page_text in pages:
        multi_call.putPage(page_name, page_text)
    return multi_call()

def attempt(func, *args):
    global sleep_time
    try:
        result = func(*args)
    except xmlrpclib.Fault as error:
        #this usually means the page content did not change. Should not happen anymore.
        print error
    except xmlrpclib.ProtocolError, err:
        print "Error: ", err.errcode
        print "Will retry after ", sleep_time, " seconds."
        #retry after sleeping
        time.sleep(sleep_time)
        sleep_time *= 2
        return attempt(func,*args)
    except:
        print "Unexpected error:", sys.exc_info()[0]
        raise
    else:
        for entry in result:
            logging.info(repr(entry))
        print "Call to ", func.__name__, " successful."
        return result

#helper function for insert_wiki_links
def make_doc_link(m):
    s = m.group(0)
    key = s[1:-1]
    #leave out the "DOC/"-part in the link name
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
    logging.info("getting existing page titles...")
    titles = attempt(get_all_titles)
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

    #build a dictionary of existing pages
    logging.info("getting existing doc pages...")
    doctitles = [title for title in titles if title.startswith("DOC/")]
    get_old_pages_multicall = attempt(get_pages, doctitles)
    old_pages = dict(zip(["statusdummy"] + doctitles, get_old_pages_multicall))

    #build a list of changed pages
    changed_pages = []
    pagetitles = [];
    for page in pages:
        title = page[0]
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
        #if a page with this title exists, re-use the text preceding the table of contents:
        introduction = ""
        if title in doctitles:
            old_text = old_pages[title]
            introduction = old_text[0:old_text.find("<<TableOfContents>>")]
        text = introduction + text
        #check if this page is new or changed)
        if(not title in doctitles or old_pages[title] != text):
            changed_pages.append([title, text])
            logging.info("%s changed, adding to update list...", title)

    #update the overview page
    title = "DOC/Overview"
    text = "";
    for pagetitle in pagetitles:
        text = text + "\n * [[" + pagetitle + "]]"
    if(not title in doctitles or old_pages[title] != text):
        changed_pages.append([title, text])
        logging.info("%s changed, adding to update list...", title)

    logging.info("sending changed pages...")
    attempt(send_pages, changed_pages)

    print "Done"
    print "You may need to run this script again to insert links to newly created pages."
