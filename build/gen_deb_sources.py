#!/usr/bin/python

# Generate deb package sources for each distro 
# from the input debian.in directory contents


output_distros = [
    ("mantic", "ubuntu23.10", "2310", ""),
    ("lunar", "ubuntu23.04", "2304", ""),
    ("kinetic", "ubuntu22.10", "2210", ""),
    ("jammy", "ubuntu22.04", "2204", ""),
    ("impish", "ubuntu21.10", "2110", ""),
    ("focal", "ubuntu20.04", "2004", ""),
    ("groovy", "ubuntu20.10", "2010", ""),
    ("disco", "ubuntu19.04", "1904", ""),
    ("bionic", "ubuntu18.04", "1804", ""),
    ("stretch", "debian9", "9", ""),
    ("buster", "debian10", "10", ""),
    ("bullseye", "debian11", "11", "")
]

editions = ["community", "commercial"]

import shutil
import os

def preprocess(inpath, inf, outf, vars):
        # Preprocessor accepts
        # @ifdef <variable> [<variable>...]
        # @else
        # @endif
        # Where variable is the name of the distro or of the edition

        def evaluate(options, distro, edition, bundle, version):
                return distro in options or edition in options or bundle in options or version in options
              
        def evaluate_version(options, vars):
                version = int(vars['version'])
                eval_command = 'version' + ''.join(options)
                return eval(eval_command)

        conditions = [True]

        # @ifndistro and @ifnedition also accepted
        for line in inf:
                for key, value in list(vars.items()):
                        line = line.replace('@%s@' % key, value)

                if line.startswith("@") and not line.startswith("@@"):
                        d, _, args = line.strip().partition(" ")
                        conds = [s.strip() for s in args.split()]
                        if d == "@ifversion":
                                conditions.append(evaluate_version(conds, vars))
                                continue
                        if d == "@ifdef":
                                conditions.append(evaluate(conds, vars['distro'], vars['edition'], vars['bundle'], vars['version']))
                                continue
                        elif d == "@ifndef":
                                conditions.append(not evaluate(conds, vars['distro'], vars['edition'], vars['bundle'], vars['version']))
                                continue
                        elif d == "@else":
                                conditions[-1] = not conditions[-1]
                                continue
                        elif d == "@endif":
                                conditions.pop()
                                continue
                        else:
                                print((inpath+": unknown directive", line))
                if conditions[-1]:
                        outf.write(line)
                        

edition_specific_file_exts = [".menu", ".mime", ".sharedmimeinfo",".lintian-overrides"]

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
        if vars['edition'] == "commercial":
                shutil.copyfile("debian.in/copyright.commercial.in", os.path.join(target_dir,"copyright"))
        else:
                shutil.copyfile("debian.in/copyright.gpl.in", os.path.join(target_dir,"copyright"))
        shutil.copyfile(os.path.join(source_dir, "source/format"), os.path.join(target_source_dir,"format"))

        outf = open(os.path.join(target_source_dir,"lintian-override"), "w+")
        inpath = os.path.join(source_dir, "source/lintian-override")
        preprocess(inpath, open(inpath), outf, vars)

        print((target_dir, "generated"))

if os.path.isdir("../internal"):
    edition = "commercial"
else:
    edition = "community"

for distro, distro_version, version, bundle in output_distros:
        #for edition in editions:
        vars = {}
        vars['distro'] = distro
        vars['distrov'] = distro_version
        vars['edition'] = edition
        vars['bundle'] = bundle
        vars['version'] = version
        generate_distro("debian.in", vars)

