import os
import sys
from enum import Enum
from typing import Generator, Optional
from io import StringIO, TextIOBase
import contextlib

# Colors are disabled in non-TTY environments such as pipes. This means
# that if output is redirected to a file, it won't contain color codes.
# Colors are always enabled on continuous integration.
_colorize = bool(sys.stdout.isatty() or os.environ.get("CI"))


@contextlib.contextmanager
def generated_wrapper(
    path: str,
    guard: Optional[bool] = None,
) -> Generator[TextIOBase, None, None]:
    """
    Wrapper class to automatically handle copyright headers and header guards
    for generated scripts. Meant to be invoked via `with` statement similar to
    creating a file.

    - `path`: The path of the file to be created.
    - `guard`: Optional bool to determine if `#pragma once` should be added. If
    unassigned, the value is determined by file extension.
    """

    with open(path, "wt", encoding="utf-8", newline="\n") as file:
        if not path.endswith(
            ".out"
        ):  # For test output, we only care about the content.
            file.write("\n/* THIS FILE IS GENERATED. EDITS WILL BE LOST. */\n\n")

            if guard is None:
                guard = path.endswith((".h", ".hh", ".hpp", ".hxx", ".inc"))
            if guard:
                file.write("#pragma once\n\n")

        with StringIO(newline="\n") as str_io:
            yield str_io
            file.write(str_io.getvalue().strip() or "/* NO CONTENT */")

        file.write("\n")


class ANSI(Enum):
    """
    Enum class for adding ansi colorcodes directly into strings.
    Automatically converts values to strings representing their
    internal value, or an empty string in a non-colorized scope.
    """

    RESET = "\x1b[0m"

    BOLD = "\x1b[1m"
    ITALIC = "\x1b[3m"
    UNDERLINE = "\x1b[4m"
    STRIKETHROUGH = "\x1b[9m"
    REGULAR = "\x1b[22;23;24;29m"

    BLACK = "\x1b[30m"
    RED = "\x1b[31m"
    GREEN = "\x1b[32m"
    YELLOW = "\x1b[33m"
    BLUE = "\x1b[34m"
    MAGENTA = "\x1b[35m"
    CYAN = "\x1b[36m"
    WHITE = "\x1b[37m"

    PURPLE = "\x1b[38;5;93m"
    PINK = "\x1b[38;5;206m"
    ORANGE = "\x1b[38;5;214m"
    GRAY = "\x1b[38;5;244m"

    def __str__(self) -> str:
        global _colorize
        return str(self.value) if _colorize else ""


def print_warning(*values: object) -> None:
    """Prints a warning message with formatting."""
    print(
        f"{ANSI.YELLOW}{ANSI.BOLD}WARNING:{ANSI.REGULAR}",
        *values,
        ANSI.RESET,
        file=sys.stderr,
    )


def print_error(*values: object) -> None:
    """Prints an error message with formatting."""
    print(
        f"{ANSI.RED}{ANSI.BOLD}ERROR:{ANSI.REGULAR}",
        *values,
        ANSI.RESET,
        file=sys.stderr,
    )


def Run(env, function, comstr="$GENCOMSTR"):
    from SCons.Script import Action

    return Action(function, comstr)
