#!/usr/bin/env python3

import os
import glob
import re
import argparse
import subprocess
import sys

from pathlib import Path
from typing import Optional

from colorama import Fore, Style

SCRIPT_DIR      = Path(os.path.dirname(os.path.realpath(__file__)))
KLEE_DIR        = os.getenv("KLEE_DIR")
KLEE_INCLUDE    = os.getenv("KLEE_INCLUDE")
BOILERPLATE_DIR = SCRIPT_DIR / "boilerplate"

KLEE_BDD_TO_C = f"{KLEE_DIR}/Release/bin/bdd-to-c"

CHOICE_SEQUENTIAL        = "seq"
CHOICE_CALLPATH_HIT_RATE = "cph"

CHOICE_TO_BOILERPLATE = {
	CHOICE_SEQUENTIAL: BOILERPLATE_DIR / "sequential.c",
	CHOICE_CALLPATH_HIT_RATE: BOILERPLATE_DIR / "callpath-hit-rate.c",
}

def __print(msg: str, color):
	print(color, end="")
	print(msg, end="")
	print(Style.RESET_ALL)

def log(msg: str = ""):
	__print(msg, Fore.LIGHTBLUE_EX)

def error(msg: str = ""):
	__print(msg, Fore.RED)

def warning(msg: str = ""):
	__print(msg, Fore.MAGENTA)

def run(args, abort_on_fail: bool = True, **pargs):
	process = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, **pargs)
	stdout = process.stdout.decode("utf-8")
	stderr = process.stderr.decode("utf-8")

	if abort_on_fail and process.returncode != 0:
		error(f"Command FAILED: {args}")
		error(f"Return code: {process.returncode}")
		error("stdout:")
		error(stdout)
		error("stderr:")
		error(stderr)
		exit(1)

	return process.returncode, stdout, stderr

def symbex(nf: Path, rerun=False, vars: dict[str,str] = {}) -> list[Path]:
	call_paths_dir = Path(f"{nf}/klee-last")

	if not call_paths_dir.exists() or rerun:
		log("Running symbolic execution")
		run("rm -rf klee-*", shell=True, cwd=os.path.abspath(nf))
		
		env = os.environ.copy()
		env.update(vars)

		code, _, _ = run([ "make", "symbex" ], cwd=str(nf.absolute()), env=env)

	assert os.path.exists(call_paths_dir)
	call_paths = glob.glob(f"{call_paths_dir}/*.call_path")
	call_paths.sort(key=lambda f: int(re.sub('\\D', '', f)))

	return [ Path(call_paths) for call_paths in call_paths ]

def read_boilerplate(boilerplate: Path) -> str:
	with open(boilerplate, "r") as f:
		return f.read()

def synthesize_nf(target: str, out: Path, bdd: Optional[Path] = None, call_paths: Optional[list[Path]] = None):
	assert bdd is not None or call_paths is not None

	bdd_to_c_args = [
		f"-target={target}",
	]

	if bdd:
		bdd_to_c_args.append(f"-in={str(bdd)}")
	elif call_paths:
		bdd_to_c_args += [ str(cp) for cp in call_paths ]

	_, stdout, _ = run([ KLEE_BDD_TO_C ] + bdd_to_c_args)
	boilerplate = read_boilerplate(CHOICE_TO_BOILERPLATE[target])
	
	with open(out, "w") as f:
		f.write(boilerplate)
		f.write(stdout)

def setup():
	global KLEE_DIR

	if not KLEE_DIR and KLEE_INCLUDE:
		KLEE_DIR = f"{KLEE_INCLUDE}/../"
	elif not KLEE_DIR:
		error("Missing KLEE_DIR env var. Exiting.")
		exit(1)

def parse_symbex_vars(nf: Path, vars, rerun_symbex):
	vars = [] if vars == None else vars

	if len(vars) > 0 and not rerun_symbex:
		error("Error: requesting symbex vars without running symbex. Run with --symbex.")
		exit(1)

	parsed = {}
	for var in vars:
		assert '=' in var and len(var.split('=')) == 2
		k, v = var.split('=')
		parsed[k] = v

	return parsed

if __name__ == "__main__":
	parser = argparse.ArgumentParser(description='Parallelize an NF.')

	parser.add_argument(
		'out',
		type=str,
		help='Output file',
	)
	
	parser.add_argument(
		'--target',
		help='implementation model target',
		choices=[ CHOICE_SEQUENTIAL, CHOICE_CALLPATH_HIT_RATE, ],
		default=CHOICE_SEQUENTIAL
	)

	parser.add_argument(
		'--bdd',
		type=str,
		required=False,
		help='Path to the BDD file',
	)

	parser.add_argument(
		'--nf',
		type=str,
		required=False,
		help='path to the NF',
	)

	parser.add_argument(
		'--symbex',
		action='store_true',
		required=False,
		default=False,
		help='Rerun symbolic execution',
	)

	parser.add_argument(
		'--var',
		action='append',
		help='NF configuration variable for symbex (e.g., --var EXPIRATION_TIME=123).'
			 'Requires --symbex flag to take effect.',
		required=False
	)

	args = parser.parse_args()
	
	out = Path(args.out)
	nf = None if args.nf == None else Path(args.nf)
	bdd = None if args.bdd == None else Path(args.bdd)

	assert nf is not None or bdd is not None
	assert nf is None or nf.exists()
	assert bdd is None or bdd.exists()

	if args.symbex and bdd is not None:
		warning("WARNING: cannot run symbex and provide a BDD file at the same time.")
		warning("Using the BDD file and ignoring the symbex flag.")

	if nf is not None and bdd is not None:
		warning("WARNING: provided both the path to the NF and a BDD file at the same time.")
		warning("Using the BDD file and ignoring the NF path.")

	setup()

	if nf:
		vars = parse_symbex_vars(nf, args.var, args.symbex)
		call_paths = symbex(nf, rerun=args.symbex, vars=vars)
		synthesize_nf(args.target, args.out, call_paths=call_paths)
	else:
		synthesize_nf(args.target, args.out, bdd=bdd)

