"""Structured controls for the parameters that are really little languages.

Three families of inputs are not free text in practice, even though WarpX stores them as
strings:

* **boundary conditions** - ``boundary.field_lo = periodic periodic`` is one keyword per
  direction, so the UI offers one dropdown per direction instead of a raw string;
* **parser expressions** - ``warpx.Ex_external_grid_function(x,y,z) = "1.0*sin(k*x)"`` is a
  small expression language with a fixed variable and function set; the UI offers templates
  and a syntax check instead of letting a typo reach the simulation;
* **external-field initialisation styles** - ``warpx.E_ext_grid_init_style`` selects whether
  the field comes from a constant, a parser expression or a file, which decides which
  expression fields are even shown.
"""
from __future__ import annotations

import re

FIELD_BC = ["periodic", "pml", "absorbing_silver_mueller", "damped", "pec", "pmc",
            "pec_insulator", "neumann", "open", "none"]
PARTICLE_BC = ["periodic", "absorbing", "reflecting", "thermal", "none"]

BOUNDARY_KEYS = {
    "boundary.field_lo": "field", "boundary.field_hi": "field",
    "boundary.particle_lo": "particle", "boundary.particle_hi": "particle",
}

# AMReX parser: variables plus the function set WarpX documents.
VARIABLES = ("x", "y", "z")
FUNCTIONS = ("sin", "cos", "tan", "asin", "acos", "atan", "atan2", "sinh", "cosh", "tanh",
             "exp", "log", "log10", "sqrt", "pow", "abs", "fabs", "min", "max", "floor",
             "ceil", "rint", "if", "tanh")
CONSTANTS = ("pi", "e", "q_e", "m_e", "clight", "epsilon0", "mu0")
KNOWN_SYMBOLS = set(VARIABLES) | set(FUNCTIONS) | set(CONSTANTS)

TEMPLATES = {
    "常数": "1.0",
    "高斯光束": "exp(-((x-x0)**2 + (y-y0)**2)/w0**2) * cos(k*z)",
    "正弦剖面": "sin(2*pi*x/L)",
    "余弦剖面": "cos(k*x) * exp(-(y/w)**2)",
    "双曲正切阶跃": "0.5*(1.0 + tanh((x - x0)/w))",
}

IDENT = re.compile(r"[A-Za-z_][A-Za-z_0-9]*")
ALLOWED_CHARS = re.compile(r"^[A-Za-z_0-9\s+\-*/(),.**<>=!&|?:.]+$")


# WarpX 的坐标约定（见 Docs/source/usage/parameters.rst 的 geometry.dims）：
#   1D → 只有 z（x、y 平移对称）；2D → x-z 平面（y 平移对称）；3D → x、y、z；RZ → r、z
AXES = {"1": ["z"], "2": ["x", "z"], "3": ["x", "y", "z"], "RZ": ["r", "z"],
        "RCYLINDER": ["r", "z"], "RSPHERE": ["r"]}


def axis_labels(dims: str) -> list[str]:
    return AXES.get(str(dims).strip().upper(), ["x", "z"])


def dims_of(deck) -> int:
    """Number of directions ``boundary.*`` must have entries for."""
    raw = str(deck.params.get("geometry.dims", "2")).strip()
    return 1 if raw in ("1", "RCYLINDER", "RSPHERE") else 2 if raw == "RZ" else (3 if raw == "3" else 2)


def boundary_values(deck, key: str) -> list[str]:
    """Current per-direction values, padded with ``periodic`` to the deck's dimension."""
    kind = BOUNDARY_KEYS.get(key)
    if kind is None:
        return []
    values = str(deck.params.get(key, "")).split()
    count = dims_of(deck)
    if not values:
        values = ["periodic"] * count
    values = values[:count] + ["periodic"] * max(0, count - len(values))
    return values


def set_boundary(deck, key: str, values: list[str]) -> None:
    deck.set(key, " ".join(v for v in values if v))


def check_expression(text: str, variables: tuple[str, ...] = VARIABLES,
                     known: tuple[str, ...] = ()) -> dict:
    """Syntax check for an AMReX parser expression.

    ``problems`` are hard errors (illegal characters, unbalanced parentheses, a function
    name without arguments).  Undefined-looking names are only **warnings**: WarpX accepts
    any ``my_constants.*`` name, and those are defined elsewhere in the deck, so treating
    them as errors would reject perfectly valid decks.
    """
    problems: list[str] = []
    cleaned = text.strip().strip('"')
    if not cleaned:
        return {"ok": False, "problems": ["表达式为空"], "warnings": [], "symbols": []}
    if not ALLOWED_CHARS.match(cleaned):
        bad = sorted({c for c in cleaned if not re.match(r"[A-Za-z_0-9\s+\-*/(),.**<>=!&|?:.]", c)})
        problems.append("包含非法字符: " + " ".join(bad))
    depth = 0
    for index, char in enumerate(cleaned):
        if char == "(":
            depth += 1
        elif char == ")":
            depth -= 1
            if depth < 0:
                problems.append(f"第 {index + 1} 个字符处多了一个右括号")
                break
    if depth > 0:
        problems.append(f"缺少 {depth} 个右括号")
    symbols = sorted({name for name in IDENT.findall(cleaned)})
    warnings = [s for s in symbols
                if s not in KNOWN_SYMBOLS and s not in variables and s not in known]
    if warnings:
        warnings = ["以下名字没有在 my_constants 里找到（若已在别处定义可忽略）: "
                    + ", ".join(warnings)]
    # a function name must be followed by '('
    for name in symbols:
        if name in FUNCTIONS:
            if not re.search(rf"\b{name}\s*\(", cleaned):
                problems.append(f"{name} 是函数，需要写成 {name}(…)")
    return {"ok": not problems, "problems": problems, "warnings": warnings,
            "symbols": symbols}


def is_expression_key(name: str) -> bool:
    """``warpx.Ex_external_grid_function(x,y,z)`` and friends are expression inputs."""
    return bool(re.search(r"\(x\s*,\s*y\s*,\s*z\)$", name)) or name.endswith("_function")


def is_style_key(name: str) -> bool:
    return name.endswith("_init_style")


# External-field / particle initialisation styles (from the string comparisons in
# Source/Plugins/*/Initialization): a style decides which expression fields matter next.
STYLE_OPTIONS = {
    "grid": ["none", "constant", "parse_e_ext_grid_function", "parse_b_ext_grid_function",
             "read_from_file"],
    "particle": ["none", "constant", "read_from_file", "parse_e_ext_particle_function",
                 "parse_b_ext_particle_function"],
}


def style_options(name: str) -> list[str]:
    """Dropdown values for an ``*_init_style`` key (empty when it is not one)."""
    if not name.endswith("_init_style"):
        return []
    return STYLE_OPTIONS["particle"] if "particle" in name else STYLE_OPTIONS["grid"]


def catalogue() -> dict:
    return {"field_bc": FIELD_BC, "particle_bc": PARTICLE_BC,
            "boundary_keys": BOUNDARY_KEYS, "templates": TEMPLATES,
            "functions": list(FUNCTIONS), "constants": list(CONSTANTS),
            "variables": list(VARIABLES), "style_options": STYLE_OPTIONS,
            "axes": AXES}
