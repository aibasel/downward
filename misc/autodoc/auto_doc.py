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
DOC_PREFIX = "DOC/"

def read_password():
    path = join(dirname(__file__), PASSWORD_FILE)
    with open(path) as password_file:
        try:
            return password_file.read().strip()
        except IOError, e:
            logging.critical("Could not find password file %s!\nIs it present?"
                     % PASSWORD_FILE)
            sys.exit(1)

def connect():
    wiki = xmlrpclib.ServerProxy(WIKI_URL + "?action=xmlrpc2", allow_none=True)
    auth_token = wiki.getAuthToken(BOT_USERNAME, read_password())
    multi_call = xmlrpclib.MultiCall(wiki)
    multi_call.applyAuthToken(auth_token)
    return multi_call

def get_all_titles_from_wiki():
    multi_call = connect()
    multi_call.getAllPages()
    response = list(multi_call())
    assert(response[0] == 'SUCCESS' and len(response) == 2)
    return response[1]

def get_pages_from_wiki(titles):
    multi_call = connect()
    for title in titles:
        print title
        multi_call.getPage(title)
    response = list(multi_call())
    assert(response[0] == 'SUCCESS')
    return dict(zip(titles, response[1:]))

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
        logging.warning("Error: %s\n"
            "Will retry after %s seconds." % (err.errcode, sleep_time))
        #retry after sleeping
        time.sleep(sleep_time)
        sleep_time *= 2
        return attempt(func,*args)
    except:
        logging.exception("Unexpected error: %s" % sys.exc_info()[0])
        sys.exit(1)
    else:
        for entry in result:
            logging.info(repr(entry))
        logging.info("Call to %s successful." % func.__name__)
        return result

def insert_wiki_links(text, titles):
    def make_link(m, prefix=''):
        s = m.group(0)
        key = s[1:-1]
        #leave out the prefix in the link name
        result = s[0] + "[[" + prefix + key + "|" + key + "]]" + s[-1]
        return result

    def make_doc_link(m):
        return make_link(m, prefix=DOC_PREFIX)

    doctitles = [title[4:] for title in titles if title.startswith(DOC_PREFIX)]
    for key in doctitles:
        text = re.sub("\W" + key + "\W", make_doc_link, text)
    othertitles = [title for title in titles
                        if not title.startswith(DOC_PREFIX) and title not in doctitles]
    for key in othertitles:
        text = re.sub("\W" + key + "\W", make_link, text)
    return text

def build_planner():
    build_dir = "../../src/search"
    cwd = os.getcwd()
    os.chdir(build_dir)
    os.system("make")
    os.chdir(cwd)

def get_pages_from_planner():
    p = subprocess.Popen(["../../src/search/downward-1", "--help", "--txt2tags"], stdout=subprocess.PIPE)
    out = p.communicate()[0]
    #split the output into tuples (title, markup_text)
    pagesplitter = re.compile(r'>>>>CATEGORY: ([\w\s]+?)<<<<(.+?)>>>>CATEGORYEND<<<<', re.DOTALL)
    pages = dict()
    for title, markup_text in pagesplitter.findall(out):
        document = markup.Document(date='')
        document.add_text("<<TableOfContents>>")
        document.add_text(markup_text)
        rendered_text = document.render("moin").strip()
        pages[DOC_PREFIX + title] = rendered_text
    return pages

def get_changed_pages(old_doc_pages, new_doc_pages, all_titles):
    changed_pages = []
    overview_lines = [];
    for title, text in sorted(new_doc_pages.items()):
        overview_lines.append(" * [[" + title + "]]")
        new_text = insert_wiki_links(text, all_titles)
        #if a page with this title exists, re-use the text preceding the table of contents
        old_text = old_doc_pages.get(title, '')
        introduction = old_text[0:old_text.find("<<TableOfContents>>")]
        new_text = introduction + new_text
        #check if this page is new or changed
        if old_text != new_text:
            logging.info("%s changed, adding to update list...", title)
            changed_pages.append([title, new_text])
    #update the overview page
    overview_title = DOC_PREFIX + "Overview"
    overview_text = "\n".join(overview_lines);
    if old_doc_pages.get(overview_title) != overview_text:
        logging.info("%s changed, adding to update list...", overview_title)
        changed_pages.append([overview_title, overview_text])
    return changed_pages

if __name__ == '__main__':
    logging.info("building planner...")
    build_planner()
    logging.info("getting new pages from planner...")
    new_doc_pages = get_pages_from_planner()
    logging.info("getting existing page titles from wiki...")
    old_titles = attempt(get_all_titles_from_wiki)
    old_doc_titles = [title for title in old_titles if title.startswith(DOC_PREFIX)]
    all_titles = set(old_titles) | set(new_doc_pages.keys())
    logging.info("getting existing doc page contents from wiki...")
    old_doc_pages = attempt(get_pages_from_wiki, old_doc_titles)
    logging.info("looking for changed pages...")
    changed_pages = get_changed_pages(old_doc_pages, new_doc_pages, all_titles)
    if changed_pages:
        logging.info("sending changed pages...")
        attempt(send_pages, changed_pages)
    else:
        logging.info("no changes found")
    print "Done"
