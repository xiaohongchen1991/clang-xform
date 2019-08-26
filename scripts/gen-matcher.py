#!/usr/bin/env python

"""
Generate a template cpp file for registering new clang AST matcher and callback
"""

import argparse
import os
import sys

def main(argv):
    parser = argparse.ArgumentParser()
    parser.add_argument(
        'matcher_name',
        type=str,
        nargs=1,
        help='Matcher name')
    parser.add_argument(
	'-o',
        '--option',
        action = 'store_true',
        help = 'Support command line options for this matcher',
        required=False)
    args = parser.parse_args()
    matcher_name = args.matcher_name[0]
    option = args.option
    script_dir = os.path.dirname(__file__);
    file_path = None
    if not option:
        file_path = os.path.join(script_dir, 'template/MatcherTemplate.cpp')
    else:
        file_path = os.path.join(script_dir, 'template/OptionMatcherTemplate.cpp')

    # read template file
    file_data = None
    with open(file_path, "r") as file:
        file_data = file.read()
        # replace with the matcher name provided by the user
        file_data = file_data.replace('__NAME__', matcher_name)

    # write the data into matcher_name.cpp
    new_file = matcher_name + '.cpp'
    with open(new_file, "w") as file:
        file.write(file_data)

    print(new_file + ' is generated')

if __name__ == '__main__':
    sys.exit(main(sys.argv[1]))
