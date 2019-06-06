#!/usr/bin/env python3
"""
Convenience script to create Docker, Singularity and Vagrant recipe files for a release
"""

import argparse
import os
import sys

from utils import fill_template, execute

CONTAINER_REPO = os.environ.get('DOWNWARD_CONTAINER_REPO')

def create_parser(argv):
    parser = argparse.ArgumentParser(description=
        'Convenience script to create Docker, Singularity and Vagrant recipe files for a release')

    parser.add_argument('-t', metavar='tag', required=True,
                        help='The image tag to be generated/built.')

    args = parser.parse_args(argv)
    return args


def create_recipe_file(template, tag, revision, filename):
    fill_template(template, filename, dict(TAG=tag, REVISION=revision))

def create_recipe_files(tag, revision):
    base_dir = os.path.join(CONTAINER_REPO, tag)
    os.makedirs(base_dir)

    create_recipe_file("_dockerfile", tag, revision,
        os.path.join(base_dir, "Dockerfile.{}".format(tag)))

    create_recipe_file("_singularity", tag, revision,
        os.path.join(base_dir, "Singularity.{}".format(tag)))

    create_recipe_file("_vagrantfile", tag, revision,
        os.path.join(base_dir, "Vagrantfile.{}".format(tag)))

def main(args):
    tag = args.t.lower()

    if CONTAINER_REPO is None or not os.path.isdir(CONTAINER_REPO):
        print('Please set the environment variable DOWNWARD_CONTAINER_REPO to the path '
              'of the repository in which the recipe files should be created.')
        sys.exit(1)

    create_recipe_files(tag, tag)

if __name__ == "__main__":
    main(create_parser(sys.argv[1:]))

# TODO
# - Add option to build the docker image
# - Add option to push the docker image to DockerHub
# - Add option to commit all generated files to container repository and push
