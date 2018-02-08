#!/usr/bin/env python3

"""
Made by Christian Oliveros on 08/02/2018 dd/mm/yy
Perform tests using matlab
"""
import os 
import subprocess


def main():
	dir_path = os.path.dirname(os.path.realpath(__file__))
	run_str = "matlab -nodisplay -nosplash -nodesktop -r \"try, run(\'{}\'), catch me, fprintf(\'%s / %s\\n\', \
		me.identifier,me.message), end, exit\""
	tests_path = os.path.join(dir_path, "tests")
	matlab_analysis_file = os.path.join(tests_path, "Analysis.m")
	print("Starting Analysis")
	p = subprocess.Popen(run_str.format(matlab_analysis_file))
	p.wait()

if __name__ == '__main__':
	main()