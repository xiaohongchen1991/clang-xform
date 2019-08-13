#!/usr/bin/env python

"""
update compile_commands.json file used in unit test framework 
"""

import argparse
import os
import sys
import re
import json

def main(argv):
    parser = argparse.ArgumentParser()
    parser.add_argument(
        'json_file',
        type=str,
        nargs='?',
        default='compile_commands.json',
        help='compile_commands.json file to udpate')

    args = parser.parse_args()
    json_file = args.json_file

    json_file = os.path.abspath(json_file)
    if not os.path.exists(json_file):
        sys.exit('compile_commands.json file does not exist!')
                
    compdbs = None
    with open(json_file, "r") as file:
        compdbs = json.load(file)

    cwd = os.getcwd()
    
    for compdb in compdbs:
        # update directory
        compdb['directory'] = cwd;

    # write compdbs back to compile_commands.json
    with open(json_file, "w") as file:
        file.write(json.dumps(compdbs, indent=4))
                
        
if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
