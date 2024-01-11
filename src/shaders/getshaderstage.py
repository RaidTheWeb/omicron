import os
import sys
import re


def main():
    with open(sys.argv[1], "r") as f:
        firstline = f.readline()
        if "OMICRON_FRAGMENT" in firstline:
            print("frag", end="")
        elif "OMICRON_VERTEX" in firstline:
            print("vert", end="")
        elif "OMICRON_TESC" in firstline:
            print("tesc", end="")
        elif "OMICRON_TESE" in firstline:
            print("tese", end="")
        elif "OMICRON_COMP" in firstline:
            print("comp", end="")

if __name__ == "__main__":
    main()
