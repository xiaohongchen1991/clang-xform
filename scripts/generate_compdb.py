#!/usr/bin/env python

"""
Helper for generating compile DBs for clang tooling. On non-Windows platforms,
this is pretty straightforward. On Windows, the tool does a bit of extra work to
integrate the content of response files, force clang tooling to run in clang-cl
mode, etc.
"""
import argparse
import json
import re
import subprocess
import os
import sys

def main(argv):
    parser = argparse.ArgumentParser()
    parser.add_argument(
        'json_files',
        nargs='*',
        help='Compile commands json files to update')
    args = parser.parse_args()

    for json_file in args.json_files:
        # read compile commands from json file
        with open(json_file, "r+") as f:
            build_path = os.path.dirname(os.path.abspath(json_file))
            compdbs = json.load(f)
            header_set = set()
            # deal with header files first
            for compdb in reversed(compdbs):
                command = compdb['command']
                file = compdb['file']
                compdb['directory'] = build_path
                name, extension = os.path.splitext(file)
                if ((extension == '.h') or (extension == '.hpp')):
                    header_set.add(file)
                    continue
                # find -I and -isystem arguments
                paths = re.findall(r'-(?:I|isystem)\s*.+?\s', command)
                # set up and run cmd to get a list of included files
                cmd = ''.join(paths)
                cmd = 'cpp -M ' + cmd + file
                process = subprocess.run(cmd, shell=True,
                                         stdout=subprocess.PIPE,
                                         stderr=subprocess.PIPE)
                headers = re.findall(r'/.+?(?:\.hpp|\.h)', process.stdout.decode())
                # create compile commands for header files under build path
                for header in headers:
                    if (build_path in header) and (header not in header_set):
                        header_set.add(header)
                        header_compdb = dict()
                        header_compdb['directory'] = build_path
                        header_command = re.sub(r'-o\s.+/.+?\.o(?=\s)',
                                                '-o /tmp/CMakeFiles/'+os.path.relpath(header, build_path+'/..')+'.o',
                                                command)
                        header_compdb['command'] = re.sub(r'-c\s+.+?\.(?:cpp|cc|c)',
                                                          '-c '+header,
                                                          header_command)
                        header_compdb['file'] = header
                        compdbs.append(header_compdb)

            # rewrite new compdbs into the json file
            f.seek(0)
            f.write(json.dumps(compdbs, indent=4))
            f.truncate()


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
