#!/usr/bin/env python3

"""
Generate compile_commands.json file for matcher unit test
"""

import argparse
import os
import sys
import re
import subprocess
import platform

def gen_compdb(file, llvmroot):
    file = os.path.abspath(file)
    llvmroot = os.path.abspath(llvmroot)
    dirname, filename = os.path.split(file)
    rootname = 'clang-xform'
    dir_rel = dirname[dirname.find(rootname):]
    dir_rel = os.path.relpath(dir_rel, rootname)    
    root = dirname[:dirname.find(rootname)+1]
        
    # generate compilation database for the file
    json_file = os.path.join(dirname, 'compile_commands.json') 
    print('generating ' + json_file)
    
    # read template file
    script_dir = os.path.dirname(__file__);
    template = os.path.join(script_dir, 'template/compile_commands.json')
    template_data = None

    with open(template, "r") as f:
        template_data = f.read()
        # replace the directory name and file name
        template_data = template_data.replace('__DIRECTORY__', dirname)
        template_data = template_data.replace('__FILE__', filename)
        template_data = template_data.replace('__LLVMROOT__', llvmroot)

    # write the data into dirname path
    with open(json_file, "w") as f:
        f.write(template_data)

def main(argv):
    parser = argparse.ArgumentParser()
    
    parser.add_argument(
        'input',
        type=str,
        nargs=1,
        help='file for compilation database generation')

    parser.add_argument(
        '-p',
        '--path',
        required=True,
        type=str,
        help='llvm root path')
    
    args = parser.parse_args()

    file = args.input[0]
    llvmroot = args.path
    
    gen_compdb(file, llvmroot)
        
if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
