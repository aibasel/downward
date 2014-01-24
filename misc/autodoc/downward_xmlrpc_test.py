#! /usr/bin/env python2.6
# -*- coding: utf-8 -*-

import logging
from os.path import dirname, join
import sys
import xmlrpclib


logging.basicConfig(level=logging.INFO, stream=sys.stdout)


BOT_USERNAME = "XmlRpcBot"
PASSWORD_FILE = "downward-xmlrpc.secret" # relative to this source file
WIKI_URL = "http://www.fast-downward.org"


def read_password():
    path = join(dirname(__file__), PASSWORD_FILE)
    with open(path) as password_file:
        try:
            return password_file.read().strip()
        except IOError, e:
            sys.exit("Could not find password file %s!\nIs it present?"
                     % PASSWORD_FILE)


def send_pages(pages):
    wiki = xmlrpclib.ServerProxy(WIKI_URL + "?action=xmlrpc2", allow_none=True)
    auth_token = wiki.getAuthToken(BOT_USERNAME, read_password())
    multi_call = xmlrpclib.MultiCall(wiki)
    multi_call.applyAuthToken(auth_token)
    for page_name, page_text in pages:
        multi_call.putPage(page_name, page_text)
    for entry in multi_call():
        logging.info(repr(entry))


if __name__ == "__main__":
    PAGE_NAME_1 = "XmlRpcBotTest1"
    PAGE_TEXT_1 = """#acl All:
= XmlRpcBot Test Page Number 1 =

This is a test of the XmlRpcBot.
"""
    PAGE_NAME_2 = "XmlRpcBotTest2"
    PAGE_TEXT_2 = """#acl All:
= XmlRpcBot Test Page Number 2 =

This is also a test of the XmlRpcBot.
"""
    send_pages([
            (PAGE_NAME_1, PAGE_TEXT_1),
            (PAGE_NAME_2, PAGE_TEXT_2),
            ])
