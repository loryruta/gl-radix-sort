import re
from os import path

script_dir = path.dirname(path.realpath(__file__))


def generate_standalone_header_code(in_filepath: str) -> str:
    with open(in_filepath, "rt") as in_file:
        code = in_file.read()

        while True:
            matches = [match for match in re.finditer(r'^#include\s+"(\S+?)"', code, re.MULTILINE)]
            if len(matches) == 0:
                break

            match = matches[0]
            included_filepath = path.join(path.dirname(in_filepath), match.group(1))
            included_code = generate_standalone_header_code(included_filepath)

            code = code[:match.span()[0]] + included_code + "\n" + code[match.span()[1]:]
        return code


def generate_standalone_header(in_filepath: str, out_filepath: str):
    print("Generating %s from %s" % (out_filepath, in_filepath))
    with open(out_filepath, "wt") as out_file:
        out_file.write(generate_standalone_header_code(in_filepath))


if __name__ == "__main__":
    def p(filename: str):
        return path.join(script_dir, "glu/%s" % filename), path.join(script_dir, "dist/%s" % filename)


    generate_standalone_header(*p("BlellochScan.hpp"))
    generate_standalone_header(*p("RadixSort.hpp"))
    generate_standalone_header(*p("Reduce.hpp"))
