import re

from lab.parser import Parser

class BottomUpParser(Parser):

    def __init__(self):
        super().__init__()

    def add_bottom_up_pattern(self, name, regex, file="run.log", required=False, type=int, flags=""):

        def search_from_bottom(content, props):
            reversed_content = "\n".join(reversed(content.splitlines()))
            match = re.search(regex, reversed_content)
            if required and not match:
                logging.error("Pattern {0} not found in file {1}".format(regex, file))
            if match:
                props[name] = type(match.group(1))

        self.add_function(search_from_bottom, file=file)

parser = BottomUpParser()
parser.add_bottom_up_pattern("landmarks", r"Discovered (\d+) landmarks")
parser.add_bottom_up_pattern("conj_landmarks", r"(\d+) are conjunctive")
parser.add_bottom_up_pattern("disj_landmarks", r"(\d+) are disjunctive")
parser.add_bottom_up_pattern("edges", r"(\d+) edges")
parser.add_bottom_up_pattern("landmark_generation_time", r"Landmarks generation time: (.+)s",type=float)    
parser.parse()

