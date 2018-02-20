#!/usr/bin/env python3

"""
Made by Christian Oliveros on 01/02/2018 dd/mm/yy
Takes all the png's from pngs_path and moves to a folder with the test name in tests_final_path
"""
import os
import re
import sys


RE_PNG_PATTERN = r'(?P<test_name>.+)_(temporal|ground_truth|no_aa|log)\.(png|txt)'

def main(pngs_path=".", tests_final_path="."):
	prog = re.compile(RE_PNG_PATTERN)
	for filename in os.listdir(pngs_path):
		if filename.endswith(".png") or filename.endswith(".txt"):
			match = re.search(prog, filename)

			test_name = match.group('test_name')
			print("Found Test: ", test_name)

			test_folder = os.path.join(tests_final_path, test_name)
			create_folder(test_folder)
			
			old_path = os.path.join(pngs_path, filename)
			new_path = os.path.join(test_folder, filename)

			print("Moving from:", old_path, "\nTo:", new_path)

			try:
				os.rename(old_path, new_path)
				print("Successfully moved\n")
			except Exception as e:
				print("Failed moving file")
				print(e, "\n")
			


def create_folder(path_with_name):
	os.makedirs(path_with_name, exist_ok=True)

if __name__ == '__main__':
	sys_len = len(sys.argv)
	if sys_len > 3:
		print("ERROR: Please, the format is: <pngs_path> <tests_final_path>", file=sys.stderr)
		sys.exit(-1)
	elif sys_len == 3:
		print("PNGs path:", sys.argv[1])
		print("Test final path:", sys.argv[2], "\n")
		main(sys.argv[1], sys.argv[2])
	elif sys_len == 2:
		print("PNGs path:", sys.argv[1], "\n")
		main(sys.argv[1])
	else:
		main()