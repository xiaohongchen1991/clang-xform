#!/usr/bin/env python3

"""
Generating header-file compile commands for clang tooling.
"""
import argparse
import json
import re
import subprocess
import os
import sys
import glob

def main(argv):
    parser = argparse.ArgumentParser()
    parser.add_argument(
        'json_files',
        nargs=1,
        help='Compile commands json file to update')
    parser.add_argument(
	'-a',
        '--accurate',
        action = 'store_true',
        help = 'Generate accurate compdb by using preprocessor',
        required=False)
    args = parser.parse_args()
    json_file = args.json_files[0]
    accurate = args.accurate
    # read compile commands from json file
    with open(json_file, "r+") as f:
        build_path = os.path.dirname(os.path.abspath(json_file))
        compdbs = json.load(f)
        header_set = set()
        # deal with header files first
        for compdb in reversed(compdbs):
            command = compdb['command']
            file = compdb['file']
            # update directory to the build path
            compdb['directory'] = build_path
            name, extension = os.path.splitext(file)
            if ((extension == '.h') or (extension == '.hpp')):
                header_set.add(file)
                continue

            # If "-a, --accurate" flag is set, then use preprocessor to find included headers.
            # This will generate more accurate compilation database
            if accurate:
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
                                                '-o ' + os.path.abspath('/tmp/'+os.path.relpath(header, build_path)+'.o'),
                                                command)
                        header_compdb['command'] = re.sub(r'-c\s+.+?\.(?:cpp|cc|c)',
                                                          '-c '+header,
                                                          header_command)
                        header_compdb['file'] = header
                        compdbs.append(header_compdb)

        # By default, assume all header files have the same compilation database
        if not accurate:
            # find header files in build directory
            headers = glob.glob(build_path + '/**/*.hpp', recursive=True)
            headers.extend(glob.glob(build_path + '/**/*.h', recursive=True))
            default_command = compdbs[0]['command']
            for header in headers:
                if header not in header_set:
                    header_compdb = dict()
                    header_compdb['directory'] = build_path
                    header_command = re.sub(r'-o\s.+/.+?\.o(?=\s)',
                                            '-o ' + os.path.abspath('/tmp/'+os.path.relpath(header, build_path)+'.o'),
                                            default_command)
                    header_compdb['command'] = re.sub(r'-c\s+.+?\.(?:cpp|cc|c|hpp|h)',
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
