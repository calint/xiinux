#!/usr/bin/python3
import re
import sys

input_string = sys.stdin.read()
pattern = r'(\w+)\s+(\d+)\s+(\w+)\.rep\s+Requests/sec:\s+([\d]+)'
matches = re.findall(pattern, input_string)
output = [' '.join(match) for match in matches]
for line in output:
    columns = line.split(" ")
    formatted_line = " ".join("{:<16}".format(column) for column in columns)
    print(formatted_line)

