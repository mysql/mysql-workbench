#!/usr/bin/python

# Generate deb package sources for each distro 
# from the input debian.in directory contents


output_distros = [("vivid", "ubu1504", ""), ("utopic", "ubu1410", 'paramiko'), ("trusty", "ubu1404",'paramiko'), ("precise", "ubu1204", 'paramiko')]

editions = ["community", "commercial"]

import shutil
import os

def preprocess(inpath, inf, outf, vars):
        # Preprocessor accepts
        # @ifdef <variable> [<variable>...]
        # @else
        # @endif
        # Where variable is the name of the distro or of the edition

        def evaluate(options, distro, edition, bundle):
                return distro in options or edition in options or bundle in options

        conditions = [True]

        # @ifndistro and @ifnedition also accepted
        for line in inf:
                for key, value in vars.items():
                        line = line.replace('@%s@' % key, value)

                if line.startswith("@") and not line.startswith("@@"):
                        d, _, args = line.strip().partition(" ")
                        conds = [s.strip() for s in args.split()]
                        if d == "@ifdef":
                                conditions.append(evaluate(conds, vars['distro'], vars['edition'], vars['bundle']))
                                continue
                        elif d == "@ifndef":
                                conditions.append(not evaluate(conds, vars['distro'], vars['edition'], vars['bundle']))
                                continue
                        elif d == "@else":
                                conditions[-1] = not conditions[-1]
                                continue
                        elif d == "@endif":
                                conditions.pop()
                                continue
                        else:
                                print inpath+": unknown directive", line
                if conditions[-1]:
                        outf.write(line)
                        

edition_specific_file_exts = [".menu", ".mime", ".sharedmimeinfo"]

def generate_distro(source_dir, vars):
        target_dir = '%s.deb-%s' % (vars['edition'], vars['distro'])
        shutil.rmtree(target_dir, ignore_errors=True)
        os.mkdir(target_dir)
        target_source_dir = os.path.join(target_dir, "source")
        os.mkdir(target_source_dir)


        for f in os.listdir(source_dir):
                inpath = os.path.join(source_dir, f)
                outpath = os.path.join(target_dir, f)
                if os.path.splitext(inpath)[-1] in edition_specific_file_exts:
                        if vars['edition'] not in inpath:
                                continue
                if not os.path.isdir(os.path.join(target_dir, f)):
                        outf = open(outpath, "w+")
                        preprocess(inpath, open(inpath), outf, vars)
                        outf.close()

                # set the same permissions as the original file
                        os.chmod(outpath, os.stat(inpath).st_mode)

        # always copy this file, since the tar will come with the right README file
        shutil.copyfile("../README", os.path.join(target_dir,"copyright"))
        shutil.copyfile(os.path.join(source_dir, "source/format"), os.path.join(target_source_dir,"format"))

        print target_dir, "generated"

for distro, distro_version, bundle in output_distros:
        for edition in editions:
                vars = {}
                vars['distro'] = distro
                vars['distrov'] = distro_version
                vars['edition'] = edition
                vars['bundle'] = bundle
                generate_distro("debian.in", vars)

