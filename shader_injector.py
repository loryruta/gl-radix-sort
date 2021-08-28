
import re
import random


def inject_shaders(in_filename, out_filename):
    f = open(in_filename, "r")
    content = f.read()
    f.close()

    shader_sources = []

    while True:
        m = re.search(r'(RGC_SHADER_INJECTOR_LOAD_SRC\(([a-zA-Z_$.]+),\s*\"([^\"]+)\"\))', content, re.MULTILINE)
        if m is None:
            break

        shader_name = m.group(2)
        shader_path = m.group(3)
        shader_src_name = "__rgc_shader_injector_shader_src_%0.32x" % random.getrandbits(128)

        shader_sources.append((shader_src_name, shader_path))

        call = "__rgc_shader_injector_load_src(%s, %s)" % (shader_name, shader_src_name)
        content = content[:m.span()[0]] + call + content[m.span()[1]:]

    m = re.search('^[ \t]*RGC_SHADER_INJECTOR_INJECTION_POINT', content, re.MULTILINE)
    if m is None:
        raise Exception("Couldn't find RGC_SHADER_INJECTOR_INJECTION_POINT, don't know where to put sources.")

    shader_sources_content = ""

    for shader_src_name, shader_path in shader_sources:
        shader_f = open(shader_path, "r")
        shader_src = shader_f.read()
        shader_f.close()

        shader_src = shader_src.replace("\"", "'")
        shader_src = shader_src.replace("\n", "\\n")

        shader_sources_content += "inline const char* %s = \"%s\";\n" % (shader_src_name, shader_src)

    content = content[:m.span()[0]] + shader_sources_content + content[m.span()[1]:]

    with open(out_filename, "w") as output:
        output.write(content)


if __name__ == "__main__":
    import sys

    if len(sys.argv) < 3:
        raise Exception("Syntax error: shader_injector <in> <out>")

    inject_shaders(sys.argv[1], sys.argv[2])
