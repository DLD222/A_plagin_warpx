"""Check a deck before running it, and translate WarpX's errors back to a field.

Two things are checked here that a generic form cannot know:

* **required parameters**: the source reads some keys with ``ParmParse::get`` (abort
  when missing) rather than ``query``, so the catalog marks them and the form can
  refuse to start;
* **gating**: a parameter that only takes effect under a condition is not required
  when that condition is off, so the checks follow the same rules the UI uses.

The error translation is intentionally pattern based: WarpX's messages already name
the offending key, and turning "### ERROR : algo.particle_shape must be set ..."
into "field X is required (可选值: 1 2 3 4)" is what a beginner needs.
"""
from __future__ import annotations

import re

TRUE_WORDS = {"1", "true", "t", "yes", "y", "on"}

# Parameters WarpX aborts on immediately when missing (verified by deleting each one from
# the starter deck and reading the abort message).  The catalog's `required` flag is
# broader - it also covers reads inside branches - and is metadata for tooltips only.
ALWAYS_REQUIRED = (
    # 几何三件套和网格数缺一个 WarpX 就直接 abort（"must specify the parameter
    # 'geometry.prob_lo'"）。它们必须和 particle_shape 一样当成硬性必填项，
    # 否则用户点运行才看到报错。
    "geometry.dims",
    "geometry.prob_lo",
    "geometry.prob_hi",
    "amr.n_cell",
    "amr.max_level",          # WarpX 用 getval 读它，缺了直接 abort（实测）

    # charge/mass 不在这里：设了 species_type 时由它推导（WarpX 实测允许）

    "<diag_name>.diag_type",
)

# Structural keys are documented by AMReX, not in parameters.rst, so the catalog has no
# type for them; without a type the form cannot check them.
STRUCTURAL_TYPES = {
    "amr.n_cell": "list of integers",
    "amr.max_level": "int",
    "amr.max_grid_size": "int",
    "amr.blocking_factor": "int",
    "max_step": "int",
    "stop_time": "float",
    "warpx.cfl": "float",
    "geometry.dims": "int",
    "geometry.prob_lo": "list of floats",
    "geometry.prob_hi": "list of floats",
}
SPACE_LIST = re.compile(r"\s+")


def _value(deck, name: str):
    if name in deck.params:
        return deck.params[name]
    template, _, sub = name.partition(".")
    for values in deck.groups.get(template, {}).values():
        if sub in values:
            return values[sub]
    return None


def _rule_ok(deck, rule: dict, instance: str | None) -> bool:
    param = rule.get("param")
    if instance and param.startswith("<"):
        param = param.split(">", 1)[0] + ">" + param.split(">", 1)[1]
        prefix, _, sub = param.partition(".")
        values = deck.groups.get(prefix, {}).get(instance, {})
        raw = values.get(sub, "")
    else:
        raw = _value(deck, param) or ""
    text = str(raw).strip().strip('"').lower()
    op = rule.get("op")
    if op == "truthy":
        return text in TRUE_WORDS
    if op == "falsy":
        return text not in TRUE_WORDS
    if op == "nonempty":
        return text not in ("", "none")
    if op == "empty":
        return text == ""
    if op == "notequals":
        return text != str(rule.get("value", "")).lower()
    if op == "contains":
        return str(rule.get("value", "")).lower() in text
    if op == "in":
        allowed = str(rule.get("value", "")).lower().split()
        return text in allowed
    if op == "member":
        if not instance:
            return False
        listed = str(_value(deck, param) or "").replace('"', "").split()
        return str(instance) in listed
    return text == str(rule.get("value", "")).lower()


def visible(name: str, catalog: dict, deck, instance: str | None = None) -> bool:
    rules = catalog.get("gating_rules", {}).get(name)
    if not rules:
        return True
    # 一条规则可能是"与"组（list）：组内全满足才算一条成立；组之间取"或"
    return any((all(_rule_ok(deck, x, instance) for x in rule) if isinstance(rule, list)
                else _rule_ok(deck, rule, instance)) for rule in rules)


NUMERIC = re.compile(r"\b(int|integer|float|real|double|bool|number)s?\b", re.I)


def _check_number(value: str, kind: str, constants: set[str] | None = None) -> str | None:
    if not value or not NUMERIC.search(kind or "") or "string" in (kind or "").lower():
        return None
    text = value.strip().strip('"')
    # 纯数字（可带科学计数法、可空格分隔多个）先直接判
    parts = SPACE_LIST.split(text)
    numeric_only = True
    for part in parts:
        try:
            if "int" in kind:
                int(part)
            else:
                float(part)
        except ValueError:
            numeric_only = False
            break
    if numeric_only:
        return None

    # 其余情况：WarpX 的数值参数可以写表达式，里面能用的名字有两类——
    #   1) 内置常量：m_e, m_p, q_e, clight, pi, epsilon0, mu0 ...
    #   2) 用户自己用 my_constants.<name> 定义的常量（如 n0、delta、kx）
    # 只有在"用了既不是数字、也不是这两类名字"的时候才判非法。
    builtin = {
        "m_e", "m_p", "m_i", "m_n", "m_u", "q_e", "q_p", "clight", "c",
        "pi", "epsilon0", "mu0", "hbar", "eV", "keV", "MeV", "GeV",
        "micro", "nano", "femto", "atto", "sqrt", "sin", "cos", "tan", "exp",
        "kb", "na", "G", "c0",
        "log", "log10", "abs", "floor", "ceil", "pow", "min", "max", "if",
        "true", "false",
    }
    if constants:                 # deck 里有 my_constants 时不敢按名字判死（可能嵌套定义）
        return None
    known = builtin
    # 先把数字字面量整体去掉：否则 "1.e19" 里的 "e19" 会被当成标识符
    stripped = re.sub(r"\d*\.?\d+(?:[eEdD][+-]?\d+)?", " ", text)
    for name in re.findall(r"[A-Za-z_][A-Za-z0-9_]*", stripped):
        if name not in known:
            return (f"`{name}` 不是数值，也不是内置常量或 my_constants 里定义过的常量"
                    f"（需要的话先写 my_constants.{name} = ...）")
    return None


def validate(deck, catalog: dict) -> list[dict]:
    """Return a list of issues: {severity, param, message, options?}."""
    issues: list[dict] = []
    allowed_sigs = {
        "x,y,z", "x,y,z,t", "t,x,y,z",
        "x,y,z,ux,uy,uz", "t,x,y,z,ux,uy,uz", "t,x,y,z,ux,uy,uz,w",
    }
    sig_re = re.compile(r"_function\(([^)]*)\)$")
    def check_signature(full: str, args: str) -> None:
        # WarpX 各族的参数表不一样（激光是 (X,Y,t)、降维诊断是 (x,y,z,E,B,j...)），
        # 能确定"一定写错了"的只有一种情况：位置类函数给的参数个数不足 3 个。
        # 例如 density_function(x,y) —— 运行时报的是 "density_function(x,y,z) not found"。
        norm = ",".join(a.strip() for a in args.split(",") if a.strip())
        if norm in allowed_sigs or len(norm.split(",")) >= 3:
            return
        issues.append({"severity": "error", "param": full,
                       "message": f"函数参数表写成 `({args})`，参数个数不足；位置类函数必须写 "
                                  "(x,y,z)（用不到的坐标由 WarpX 补 0，不能省略）"})

    params = {p["name"]: p for p in catalog["params"]}

    # A parameter name containing "/" is a documentation artefact (e.g. the old
    # `geometry.prob_lo/hi` row): WarpX has no such key, so writing it produces an
    # input line that is silently ignored.  Catch it here instead of at run time.
    for key in list(deck.params):
        if "/" in key:
            issues.append({"severity": "error", "param": key,
                           "message": "不是 WarpX 参数：`/` 表示文档里并排定义的两个参数，"
                                      "应分别填写（例如 geometry.prob_lo 与 geometry.prob_hi）"})

    # 用户用 my_constants.xxx = ... 定义的常量，数值表达式里可以引用
    user_constants = {k.split("my_constants.", 1)[1] for k in deck.params
                      if k.startswith("my_constants.") and "." in k}

    def check(label: str, full: str, value) -> None:
        info = params.get(full, {})
        kind = info.get("type") or STRUCTURAL_TYPES.get(full, "")
        missing = not str(value).strip()
        if full in ALWAYS_REQUIRED and missing:
            issues.append({"severity": "error", "param": label,
                           "message": "必填项：缺了它 WarpX 会立刻 abort",
                           "options": info.get("options", [])})
            return
        problem = _check_number(str(value), kind, user_constants)
        if problem:
            issues.append({"severity": "error", "param": label, "message": problem})

    for name, value in deck.params.items():
        m_top = sig_re.search(name)
        if m_top:
            check_signature(name, m_top.group(1))
        check(name, name, value)
    # 从 checkpoint 续算时几何/网格来自重启文件；FILE = 其他输入文件时参数可能继承自它
    req = list(ALWAYS_REQUIRED)
    if str(deck.params.get("FILE", "") or "").strip():
        req = []
    if str(deck.params.get("amr.restart", "") or "").strip():
        req = [k for k in req if not k.startswith(("geometry.", "amr.n_cell", "amr.max_level"))]
    for name in req:
        if name.startswith("<"):
            continue
        if not visible(name, catalog, deck, None):
            continue          # 当前配置下不适用（例如面注入时的 profile）
        if not str(deck.params.get(name, "")).strip():
            check(name, name, deck.params.get(name, ""))

    for template, instances in deck.groups.items():
        # a required key that is *absent* has no item to iterate, so it is checked here
        for instance in instances:
            for name in req:
                if not name.startswith(template + "."):
                    continue
                sub = name[len(template) + 1:]
                if not visible(name, catalog, deck, instance):
                    continue      # 当前配置下不适用
                if sub not in instances[instance]:
                    check(f"{instance}.{sub}", name, "")
    for template, instances in deck.groups.items():
        for instance, values in instances.items():
            for sub, value in values.items():
                full = f"{template}.{sub}"
                label = f"{instance}.{sub}"
                m_sig = sig_re.search(sub)
                if m_sig:
                    check_signature(label, m_sig.group(1))
                if not visible(full, catalog, deck, instance):
                    continue
                check(label, full, value)

    # algo.particle_shape 只在**有物种**时才被 WarpX 读取（实测：纯场算例不写也能跑）
    if deck.groups.get("<species_name>") and \
            not str(deck.params.get("algo.particle_shape", "") or "").strip():
        issues.append({"severity": "error", "param": "algo.particle_shape",
                       "message": "必填项：有粒子物种时必须给（缺了 WarpX 会立刻 abort）"})

    # parser 函数的参数表必须和 WarpX 期望的一致：位置类函数一律是 (x,y,z)，
    # 粒子类函数是 (x,y,z,ux,uy,uz) 或带时间的写法。写成 (x,y) 这种，运行时报的是
    # "density_function(x,y,z) not found in database"，很难看出是参数表写错了。
    # 每个物种：要么给 species_type（charge/mass 由它推导），要么 charge 和 mass 都写。
    # 实测：只写 species_type 的算例 WarpX 能正常跑；原来把 charge/mass 当无条件必填，
    # 那种合法算例会被误报。
    for instance, values in deck.groups.get("<species_name>", {}).items():
        if str(values.get("species_type", "") or "").strip():
            continue
        injection = str(values.get("injection_style", "") or "").lower()
        if ("from_file" in injection or "openpmd" in injection
                or "external_file" in injection):
            continue                  # 电荷/质量由文件给出
        lacking = [k for k in ("charge", "mass")
                   if not str(values.get(k, "") or "").strip()]
        if lacking:
            issues.append({"severity": "error", "param": f"{instance}.{lacking[0]}",
                           "message": f"物种 `{instance}` 既没写 species_type，也没写 "
                                      + "、".join(lacking) + "；两类信息至少要给一套"})

    def _clean(value) -> str:
        """输入里的枚举值可能带引号（官方算例就写 "electron"），比较前去掉。"""
        return str(value or "").strip().strip('"').strip("'").strip().lower()

    # gaussianflux 只允许 NFluxPerCell（SpeciesUtils.cpp 里 assert）。
    for instance, values in deck.groups.get("<species_name>", {}).items():
        if str(values.get("momentum_distribution_type", "") or "").strip().lower()                 != "gaussianflux":
            continue
        style = _clean(values.get("injection_style"))
        if style != "nfluxpercell":
            issues.append({
                "severity": "error", "param": f"{instance}.momentum_distribution_type",
                "message": "gaussianflux 只能与 injection_style = NFluxPerCell 搭配"
                           "（源码 assert）；否则改用 gaussian 或改成 NFluxPerCell"})

    # QED 量子同步：缺 chi_min / 查找表 / 光子产物会在 MultiParticleContainer 里 abort。
    for instance, values in deck.groups.get("<species_name>", {}).items():
        if _clean(values.get("do_qed_quantum_sync")) not in ("1", "true", "t", "yes"):
            continue
        if not str(deck.params.get("qed_qs.chi_min", "") or "").strip():
            issues.append({"severity": "error", "param": "qed_qs.chi_min",
                           "message": "开启 do_qed_quantum_sync 时必须给 qed_qs.chi_min"})
        mode = _clean(deck.params.get("qed_qs.lookup_table_mode"))
        if mode not in ("builtin", "generate", "load"):
            issues.append({"severity": "error", "param": "qed_qs.lookup_table_mode",
                           "message": "qed_qs.lookup_table_mode 必须是 builtin / generate / load"})
        elif mode == "load" and not str(deck.params.get("qed_qs.load_table_from", "") or "").strip():
            issues.append({"severity": "error", "param": "qed_qs.load_table_from",
                           "message": "lookup_table_mode=load 时必须给 qed_qs.load_table_from"})
        prod = _clean(values.get("qed_quantum_sync_phot_product_species"))
        names = [n.strip().strip('"').strip("'") for n in
                 str(deck.params.get("particles.species_names", "") or "").split()]
        if not prod or prod not in names:
            issues.append({"severity": "error",
                           "param": f"{instance}.qed_quantum_sync_phot_product_species",
                           "message": f"光子产物物种 {prod or '未指定'} 不在 particles.species_names 中"})
        else:
            pvals = deck.groups.get("<species_name>", {}).get(prod, {})
            if _clean(pvals.get("species_type")) != "photon":
                issues.append({"severity": "error",
                               "param": f"{instance}.qed_quantum_sync_phot_product_species",
                               "message": f"产物物种 {prod} 的 species_type 必须是 photon"})

    # 碰撞的 supercycle / subcycle 只能选一个（CollisionBase.cpp:34 assert）。
    for instance, values in deck.groups.get("<collision_name>", {}).items():
        if str(values.get("ndt_supercycle", "") or "").strip() and \
                str(values.get("ndt_subcycle", "") or "").strip():
            issues.append({
                "severity": "error", "param": f"{instance}.ndt_subcycle",
                "message": "ndt_supercycle 与 ndt_subcycle 只能选一个"
                           "（两个都写 CollisionBase.cpp 会 abort）；请清空其中一个"})

    # 经典辐射反作用只允许 species_type=electron/positron，且必须用 Boris
    # （PhysicalParticleContainer.cpp:262-276 的两条 ALWAYS_ASSERT）。
    _pusher = str(deck.params.get("algo.particle_pusher", "") or "boris").strip().lower()
    for instance, values in deck.groups.get("<species_name>", {}).items():
        crr = _clean(values.get("do_classical_radiation_reaction"))
        if crr not in ("1", "true", "t", "yes"):
            continue
        stype = _clean(values.get("species_type"))
        if stype not in ("electron", "positron"):
            issues.append({
                "severity": "error",
                "param": f"{instance}.do_classical_radiation_reaction",
                "message": "经典辐射反作用只支持 species_type = electron / positron；"
                           f"当前是 {stype or '未指定'}。请设置 species_type，"
                           "或关闭 do_classical_radiation_reaction（仅写 charge/mass 不够）"})
        if _pusher != "boris":
            issues.append({
                "severity": "error",
                "param": "algo.particle_pusher",
                "message": "开启经典辐射反作用时必须用 Boris 推进器；"
                           f"当前 algo.particle_pusher = {_pusher}"})

    # Maxwellian 的温度与 u_std 两条分支互斥（TemperatureProperties.cpp 里 assert）。
    # 表单切换子类型时会清掉另一支；手写文本仍可能同时写两个，这里拦下。
    for instance, values in deck.groups.get("<species_name>", {}).items():
        if _clean(values.get("momentum_distribution_type")) != "maxwellian":
            continue
        has_ustd = bool(str(values.get("maxwellian_u_std_distribution_type", "") or "").strip())
        has_temp = bool(str(values.get(
            "maxwellian_temperature_in_eV_distribution_type", "") or "").strip())
        if has_ustd and has_temp:
            issues.append({
                "severity": "error",
                "param": f"{instance}.maxwellian_temperature_in_eV_distribution_type",
                "message": "maxwellian_u_std_distribution_type 与 "
                           "maxwellian_temperature_in_eV_distribution_type 只能二选一"
                           "（两个都写 WarpX 会 abort）；请清空其中一个及其参数"})

    # amr.restart must be a checkpoint *path*, not an index.  A bare number becomes
    # "./1/WarpXHeader" at run time and WarpX aborts with "Couldn't open file".  Catch it here
    # instead of after a 30 s start-up (the 2026-11-18 report: the tooltip said "0/1").
    restart = str(deck.params.get("amr.restart", "") or "").strip().strip('"')
    if restart:
        import os as _os
        looks_numeric = restart.isdigit()
        has_header = _os.path.exists(_os.path.join(restart, "WarpXHeader")) or \
            any(_os.path.exists(_os.path.join(restart + str(n), "WarpXHeader"))
                for n in range(0, 10))
        if looks_numeric:
            issues.append({"severity": "error", "param": "amr.restart",
                           "message": f"`{restart}` 是数字，会被当成目录名：应填 checkpoint "
                                      "目录前缀（如 diag1/checkpoint0001），否则 WarpX 报 "
                                      f"`Couldn't open file: {restart}/WarpXHeader`"})
        elif not has_header:
            issues.append({"severity": "warn", "param": "amr.restart",
                           "message": f"在当前目录找不到 `{restart}/WarpXHeader`；"
                                      "checkpoint 路径是相对运行目录解析的，确认它存在"})

    # resource estimate
    try:
        cells = 1
        for part in SPACE_LIST.split(str(deck.params.get("amr.n_cell", "")).strip()):
            number = int(float(part))
            if number > 0:
                cells *= number
        if cells > 4_000_000:
            issues.append({"severity": "warn", "param": "amr.n_cell",
                           "message": f"基础网格 {cells:,} 点；场数据内存约 "
                                      f"{cells * 8 * 10 / 1e9:.1f} GB 量级"})
        levels = int(float(str(deck.params.get("amr.max_level", "0")) or 0))
        if levels > 0:
            issues.append({"severity": "info", "param": "amr.max_level",
                           "message": f"启用 {levels} 层自适应加密，实际网格数会更大"})
    except (ValueError, TypeError):
        pass

    # the live view needs a periodic openPMD diagnostic
    diags = deck.groups.get("<diag_name>", {})
    if diags and not any(str(v.get("format", "")).lower() == "openpmd" for v in diags.values()):
        issues.append({"severity": "warn", "param": "<diag_name>",
                       "message": "没有 format=openpmd 的诊断，实时画面不会有数据"})
    # diag_type 决定哪些 format 合法（MultiDiagnostics / FullDiagnostics /
    # BTDiagnostics / BoundaryScrapingDiagnostics 的 assert）。
    FORMAT_BY_DIAG_TYPE = {
        "Full": {"plotfile", "openpmd", "checkpoint", "ascent", "sensei", "catalyst"},
        "TimeAveraged": {"plotfile", "openpmd", "checkpoint", "ascent", "sensei", "catalyst"},
        "BackTransformed": {"plotfile", "openpmd"},
        "BoundaryScraping": {"openpmd"},
    }
    for name, values in diags.items():
        diag_type = str(values.get("diag_type", "") or "").strip()
        fmt = str(values.get("format", "") or "plotfile").strip().lower()
        allowed = FORMAT_BY_DIAG_TYPE.get(diag_type)
        if allowed is not None and fmt not in allowed:
            issues.append({
                "severity": "error", "param": f"{name}.format",
                "message": f"diag_type={diag_type} 只支持 format = "
                           + " / ".join(sorted(allowed)) + f"；当前是 {fmt}",
                "options": sorted(allowed)})
        if diag_type == "BackTransformed" and str(
                values.get("particle_fields_to_plot", "") or "").strip():
            issues.append({
                "severity": "error", "param": f"{name}.particle_fields_to_plot",
                "message": "BackTransformed 诊断不支持粒子场输出（源码 assert）"})

    for name, values in diags.items():
        # 普通诊断和降维诊断是两套命名：把 field_energy.diag_type = ReducedDiags 写进
        # diagnostics.diags_names 时，WarpX 运行到 "diag_type must be Full, ..." 才 abort。
        # 这里提前拦下，并说明降维诊断的正确写法。
        diag_type = str(values.get("diag_type", "") or "").strip()
        if diag_type and diag_type not in {
                "Full", "TimeAveraged", "BackTransformed", "BoundaryScraping"}:
            issues.append({
                "severity": "error", "param": f"{name}.diag_type",
                "message": f"普通诊断的 diag_type 不能是 `{diag_type}`；只接受 Full / "
                           "TimeAveraged / BackTransformed / BoundaryScraping。"
                           f"如果这是降维诊断，请改用 warpx.reduced_diags_names = {name}，"
                           f"再写 {name}.type = FieldEnergy",
                "options": ["Full", "TimeAveraged", "BackTransformed", "BoundaryScraping"]})
        if str(values.get("format", "")).lower() == "openpmd" and not (
                values.get("intervals") or values.get("period")):
            issues.append({"severity": "warn", "param": f"{name}.intervals",
                           "message": "openPMD 诊断没有设置 intervals，计算过程中不会写新迭代"})
    return issues


ERROR_LINE = re.compile(
    r"(?:###\s*ERROR\s*:\s*"
    r"|ParmParse::\w+(?:\(\))?\s*:\s*"      # ParmParse::getval(): / ParmParse::sgetarr:
    r"|\bnot found in database\b\s*(\S+)?)"  # 兜底：xxx not found in database
    r"(.+)")
PARAM_HINT = re.compile(r"([a-z_][a-z0-9_]*(?:\.[a-z0-9_]+)+)")
MUST_BE_SET = re.compile(r"([a-z_][a-z0-9_.]+)\s+must be set")
NOT_RECOGNIZED = re.compile(r"string\s+'([^']+)'\s+not recognized")
# WarpX messages that name the parameter only in English prose.  Conservative: only the
# ones whose parameter is unambiguous are mapped; anything else is reported without a field.
MESSAGE_TO_PARAM = {
    "injection style": "<species_name>.injection_style",
    "density profile type": "<species_name>.profile",
    "momentum distribution type": "<species_name>.momentum_distribution_type",
    "particle boundary condition": "boundary.particle_lo",
    "position initialization strategy": "<species_name>.position_initialization",
    "momentum initialization strategy": "<species_name>.momentum_initialization",
    "diag_type must be": "<diag_name>.diag_type",
}
NOT_FOUND = re.compile(r"([A-Za-z_][\w.]*\.{1,2}\w+)\s+not found")


def _resolve_param(param: str, params: dict, deck=None) -> str:
    """Map `electrons.profile` to the template whose group actually owns `electrons`."""
    if param in params:
        return param
    instance, _, sub = param.partition(".")
    candidates = [t for t in params if t.startswith("<") and t.endswith("." + sub)]
    if not candidates:
        return param
    if deck is not None:
        for template in candidates:
            group = template.split(".", 1)[0]          # `<species_name>.profile` -> `<species_name>`
            if instance in deck.groups.get(group, {}):
                return template
    return candidates[0]


def translate_error(line: str, catalog: dict, deck=None) -> dict | None:
    """Turn one WarpX error line into {param, message, hint, options}."""
    params = {p["name"]: p for p in catalog["params"]}
    match = ERROR_LINE.search(line)
    if not match:
        return None
    message = (match.group(2) or match.group(1) or "").strip()
    param = None
    for pattern in (MUST_BE_SET, NOT_FOUND):
        found = pattern.search(message) or pattern.search(line)
        if found:
            param = found.group(1).strip('"')
            break
    if param is None:
        found = PARAM_HINT.search(message)
        if found:
            param = found.group(1)
    if param is None:
        for phrase, mapped in MESSAGE_TO_PARAM.items():
            if phrase in message.lower():
                param = mapped
                break
    if param:
        param = re.sub(r"\.{2,}", ".", param)      # `electrons..profile` -> `electrons.profile`
        param = _resolve_param(param, params, deck)
    info = params.get(param) if param else None
    hint = []
    if info:
        if info.get("options") and NOT_RECOGNIZED.search(message):
            hint.append("可选值：" + " / ".join(info["options"]))
        if info.get("doc"):
            hint.append(info["doc"][:220])
        if info.get("required"):
            hint.append("这是必填项（源码里用 get 读取）")
    # "X(x,y,z) not found in database"：最典型的原因是函数参数表写少了坐标，
    # 或者参数名拼错。这句提示对用户没有可操作性，这里翻译成人话。
    # 注意：WarpX 报的是它**期望**的名字（如 density_function(x,y,z)），用户真正
    # 写错的键（density_function(x,y)）不在报错行里，只能回 deck 里找同前缀的键。
    sig = re.search(r"([A-Za-z_][\w.]*)\(([^)]*)\)\s+not found in database", message)
    if sig:
        name = re.sub(r"\.{2,}", ".", sig.group(1))      # electrons..density_function
        raw_args = sig.group(2)
        args = ",".join(a.strip() for a in raw_args.split(",") if a.strip())
        actual = None
        instance, _, base = name.rpartition(".")
        if instance and getattr(deck, "groups", None):
            values = deck.groups.get("<species_name>", {}).get(instance, {})
            candidate = next((k for k in values if k.startswith(base + "(")), None)
            if candidate and candidate != f"{base}({raw_args})":
                actual = candidate
        if actual:
            param = f"{instance}.{actual}"
            hint.append(f"你写的是 `{actual}`；WarpX 要求 `{base}({raw_args})`——位置类函数的"
                        "参数表不能省略坐标（用不到的坐标由 WarpX 补 0）")
        elif len(args.split(",")) < 3:
            hint.append(f"函数参数表写成了 `({raw_args})`；WarpX 的位置类函数必须写 (x,y,z)"
                        "（用不到的坐标会自动补 0，不能省略）")
        else:
            hint.append(f"WarpX 里没有 `{name}(...)` 这一项：参数名拼错了，或该函数在当前"
                        "求解器/维度下不存在")
    # 把降维诊断写进普通诊断列表是另一类常见错误（FieldEnergy 等）：
    # 报错是 "<diag>.diag_type must be Full, TimeAveraged, ..."。
    if "diag_type must be" in message.lower():
        hint.append("普通诊断的 diag_type 只接受 Full / TimeAveraged / BackTransformed / "
                    "BoundaryScraping；FieldEnergy 这类属于降维诊断，应写 "
                    "warpx.reduced_diags_names = <name>，再写 <name>.type = FieldEnergy，"
                    "不要放进 diagnostics.diags_names")
    if not hint:
        if "must be set" in message:
            hint.append("这是必填项，请在表单里补上")
        elif "not recognized" in message:
            hint.append("取值不在允许列表里")
    return {"param": param, "message": message, "hint": " · ".join(hint)}
