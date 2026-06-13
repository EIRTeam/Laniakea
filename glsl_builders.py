import os
from methods import generated_wrapper
from typing import List, Union


class RDHeaderStruct:
    def __init__(self):
        self.vertex_lines = []
        self.fragment_lines = []
        self.compute_lines = []

        self.vertex_included_files = []
        self.fragment_included_files = []
        self.compute_included_files = []

        self.reading = ""
        self.line_offset = 0
        self.vertex_offset = 0
        self.fragment_offset = 0
        self.compute_offset = 0


def to_raw_cstring(value: Union[str, List[str]]) -> str:
    MAX_LITERAL = 16 * 1024

    if isinstance(value, list):
        value = "\n".join(value) + "\n"

    split: List[bytes] = []
    offset = 0
    encoded = value.encode()

    while offset <= len(encoded):
        segment = encoded[offset : offset + MAX_LITERAL]
        offset += MAX_LITERAL
        if len(segment) == MAX_LITERAL:
            # Try to segment raw strings at double newlines to keep readable.
            pretty_break = segment.rfind(b"\n\n")
            if pretty_break != -1:
                segment = segment[: pretty_break + 1]
                offset -= MAX_LITERAL - pretty_break - 1
            # If none found, ensure we end with valid utf8.
            # https://github.com/halloleo/unicut/blob/master/truncate.py
            elif segment[-1] & 0b10000000:
                last_11xxxxxx_index = [
                    i
                    for i in range(-1, -5, -1)
                    if segment[i] & 0b11000000 == 0b11000000
                ][0]
                last_11xxxxxx = segment[last_11xxxxxx_index]
                if not last_11xxxxxx & 0b00100000:
                    last_char_length = 2
                elif not last_11xxxxxx & 0b0010000:
                    last_char_length = 3
                elif not last_11xxxxxx & 0b0001000:
                    last_char_length = 4

                if last_char_length > -last_11xxxxxx_index:
                    segment = segment[:last_11xxxxxx_index]
                    offset += last_11xxxxxx_index

        split += [segment]

    if len(split) == 1:
        return f'R"<!>({split[0].decode()})<!>"'
    else:
        # Wrap multiple segments in parenthesis to suppress `string-concatenation` warnings on clang.
        return "({})".format(
            " ".join(f'R"<!>({segment.decode()})<!>"' for segment in split)
        )


def build_rd_header(filename: str, shader: str) -> None:
    include_file_in_rd_header(shader, header_data := RDHeaderStruct(), 0)
    class_name = (
        os.path.basename(shader)
        .replace(".glsl", "")
        .title()
        .replace("_", "")
        .replace(".", "")
        + "ShaderRD"
    )

    with generated_wrapper(filename) as file:
        file.write(f"""\
#include "eirteam_shader_rd.h"
class {class_name} : public EIRTeamShaderRD {{
public:
	{class_name}() {{
""")

        if header_data.compute_lines:
            file.write(f"""\
		static const char *_vertex_code = nullptr;
		static const char *_fragment_code = nullptr;
		static const char _compute_code[] = {{
{to_raw_cstring(header_data.compute_lines)}
		}};
""")
        else:
            file.write(f"""\
		static const char _vertex_code[] = {{
{to_raw_cstring(header_data.vertex_lines)}
		}};
		static const char _fragment_code[] = {{
{to_raw_cstring(header_data.fragment_lines)}
		}};
		static const char *_compute_code = nullptr;
""")
        file.write("""\
        setup(_compute_code);
        """)

        file.write("""\
}
};
""")


def include_file_in_rd_header(filename: str, header_data: RDHeaderStruct, depth: int):
    with open(filename, "r", encoding="utf-8") as fs:
        line = fs.readline()

        while line:
            index = line.find("//")
            if index != -1:
                line = line[:index]

            if line.find("#[vertex]") != -1:
                header_data.reading = "vertex"
                line = fs.readline()
                header_data.line_offset += 1
                header_data.vertex_offset = header_data.line_offset
                continue

            if line.find("#[fragment]") != -1:
                header_data.reading = "fragment"
                line = fs.readline()
                header_data.line_offset += 1
                header_data.fragment_offset = header_data.line_offset
                continue

            if line.find("#[compute]") != -1:
                header_data.reading = "compute"
                line = fs.readline()
                header_data.line_offset += 1
                header_data.compute_offset = header_data.line_offset
                continue

            while line.find("#include ") != -1:
                includeline = line.replace("#include ", "").strip()[1:-1]

                if includeline.startswith("thirdparty/"):
                    included_file = os.path.relpath(includeline)

                else:
                    included_file = os.path.relpath(
                        os.path.dirname(filename) + "/" + includeline
                    )

                if (
                    included_file not in header_data.vertex_included_files
                    and header_data.reading == "vertex"
                ):
                    header_data.vertex_included_files += [included_file]
                    if (
                        include_file_in_rd_header(included_file, header_data, depth + 1)
                        is None
                    ):
                        print_error(
                            f'In file "{filename}": #include "{includeline}" could not be found!"'
                        )
                elif (
                    included_file not in header_data.fragment_included_files
                    and header_data.reading == "fragment"
                ):
                    header_data.fragment_included_files += [included_file]
                    if (
                        include_file_in_rd_header(included_file, header_data, depth + 1)
                        is None
                    ):
                        print_error(
                            f'In file "{filename}": #include "{includeline}" could not be found!"'
                        )
                elif (
                    included_file not in header_data.compute_included_files
                    and header_data.reading == "compute"
                ):
                    header_data.compute_included_files += [included_file]
                    if (
                        include_file_in_rd_header(included_file, header_data, depth + 1)
                        is None
                    ):
                        print_error(
                            f'In file "{filename}": #include "{includeline}" could not be found!"'
                        )

                line = fs.readline()

            line = line.replace("\r", "").replace("\n", "")

            if header_data.reading == "vertex":
                header_data.vertex_lines += [line]
            if header_data.reading == "fragment":
                header_data.fragment_lines += [line]
            if header_data.reading == "compute":
                header_data.compute_lines += [line]

            line = fs.readline()
            header_data.line_offset += 1

    return header_data


def build_rd_headers(target, source, env):
    env.NoCache(target)
    for src in source:
        build_rd_header(f"{src}.gen.h", str(src))


class RAWHeaderStruct:
    def __init__(self):
        self.code = ""


def include_file_in_raw_header(
    filename: str, header_data: RAWHeaderStruct, depth: int
) -> None:
    with open(filename, "r", encoding="utf-8") as fs:
        line = fs.readline()

        while line:
            while line.find("#include ") != -1:
                includeline = line.replace("#include ", "").strip()[1:-1]

                included_file = os.path.relpath(
                    os.path.dirname(filename) + "/" + includeline
                )
                include_file_in_raw_header(included_file, header_data, depth + 1)

                line = fs.readline()

            header_data.code += line
            line = fs.readline()


def build_raw_header(filename: str, shader: str) -> None:
    include_file_in_raw_header(shader, header_data := RAWHeaderStruct(), 0)

    with generated_wrapper(filename) as file:
        file.write(f"""\
static const char {os.path.basename(shader).replace(".glsl", "_shader_glsl")}[] = {{
{to_raw_cstring(header_data.code)}
}};
""")


def build_raw_headers(target, source, env):
    env.NoCache(target)
    for src in source:
        build_raw_header(f"{src}.gen.h", str(src))
