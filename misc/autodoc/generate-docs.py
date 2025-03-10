#! /usr/bin/env python3

import argparse
import logging
from pathlib import Path
import re
import subprocess
import sys

import markup

SCRIPT_DIR = Path(__file__).resolve().parent
REPO_ROOT_DIR = SCRIPT_DIR.parents[1]

TXT2TAGS_OPTIONS = {
    "preproc": [
        [r"<<BR>>", "ESCAPED_LINEBREAK"],
        [r"``` (.+?)$", r"```\n\1\n```"],
    ],
    "postproc": [
        [r"ESCAPED_LINEBREAK", "<br />"],
        # Convert MoinMoin syntax for code to Markdown.
        [r"{{{", "```"],
        [r"}}}", "```"],
    ],
}


INDEX_TEXT = """
# Search Plugins

Fast Downward's search component consists of many different plugins of different
plugin types such as [search algorithms](SearchAlgorithm.md) or [evaluators](Evaluator.md).

The following pages document the plugins of each type:

{plugin_list}

The [syntax documentation](../search-plugin-syntax.md) contains more
information on how to read their documentation.

"""

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--outdir", default=f"{REPO_ROOT_DIR}/docs/search")
    parser.add_argument("--build", default="release")
    parser.add_argument('--skipbuild', action='store_true')
    return parser.parse_args()


def build_planner(build):
    subprocess.check_call([sys.executable, "build.py", build, "downward"], cwd=REPO_ROOT_DIR)


def insert_wiki_links(text, titles):
    def make_link(m):
        anchor = m.group("anchor") or ""
        link_name = m.group("link")
        target = link_name + ".md"
        if anchor:
            target = link_name + ".md#" + anchor.lower()
            link_name = anchor
        link_name = link_name.replace("_", " ")
        result = m.group("before") + f'[""{link_name}"" {target}]' + m.group("after")
        return result

    # List of characters allowed in doc titles. Matches 'word characters' including '_', '+', and '-'.
    title_white_list = r"[\w\+-]"

    re_link = r"(?P<before>\W)(?P<link>%s)(#(?P<anchor>" + title_white_list + r"+))?(?P<after>\W)"
    for title in titles:
        text = re.sub(re_link % title, make_link, text)
    return text


def build_docs(build, outdir):
    out = subprocess.check_output(
        ["./fast-downward.py", "--build", build, "--search", "--", "--help", "--txt2tags"],
        cwd=REPO_ROOT_DIR).decode("utf-8")
    # Split the output into tuples (title, markup_text).
    pagesplitter = re.compile(r">>>>CATEGORY: ([\w\s]+?)<<<<(.+?)>>>>CATEGORYEND<<<<", re.DOTALL)
    pages = pagesplitter.findall(out)
    titles = [title for title, _ in pages]
    title_list = "\n".join(f"-  [{title}]({title}.md)" for title in titles)
    pages.extend([("index", INDEX_TEXT.format(plugin_list=title_list))])
    for title, text in pages:
        if title == "index":
            output = text
        else:
            text = insert_wiki_links(text, titles)
            document = markup.Document(title="", date="")
            document.add_text(text)
            output = document.render("md", options=TXT2TAGS_OPTIONS)
        with open(f"{outdir}/{title}.md", "w") as f:
            f.write(output)


if __name__ == '__main__':
    args = parse_args()
    if not args.skipbuild:
        logging.info("building planner...")
        build_planner(args.build)
    logging.info("building documentation...")
    outdir = SCRIPT_DIR / args.outdir
    try:
        outdir.mkdir(parents=True)
    except FileExistsError as e:
        sys.exit(e)
    html = build_docs(args.build, outdir)
