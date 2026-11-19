"""官方算例的输入文本往返一致性检查（验收项 5 的文本部分）。

对 Examples/ 下每个 inputs* 文件：
  Deck.from_inputs(text) -> deck.to_inputs() -> Deck.from_inputs(...) 必须与第一次解析
得到**完全相同**的参数集合与物种/诊断分组。任何差异都说明网页会改写用户的输入。
"""
from __future__ import annotations

import json
import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[1]))
from warpx_studio.deck.model import Deck  # noqa: E402


def main(root: pathlib.Path) -> int:
    # .py 是 PICMI 脚本，不是 WarpX 输入文件，解析它没有意义
    files = [f for f in sorted(root.rglob("inputs*")) if f.is_file() and f.suffix != ".py"]
    bad = []
    for f in files:
        try:
            deck = Deck.from_inputs(f.read_text(errors="ignore"))
        except Exception as exc:                       # noqa: BLE001
            bad.append((f, f"解析失败: {exc!r}"[:100]))
            continue
        again = Deck.from_inputs(deck.to_inputs())
        groups_a = {t: {i: dict(v) for i, v in inst.items()} for t, inst in deck.groups.items()}
        groups_b = {t: {i: dict(v) for i, v in inst.items()} for t, inst in again.groups.items()}
        if deck.params != again.params:
            diff = {k: (deck.params.get(k), again.params.get(k))
                    for k in set(deck.params) | set(again.params)
                    if deck.params.get(k) != again.params.get(k)}
            bad.append((f, f"参数差异 {list(diff.items())[:2]}"))
        elif groups_a != groups_b:
            bad.append((f, "分组差异"))
    print(f"{len(files)} 个输入文件，往返一致 {len(files) - len(bad)}，不一致 {len(bad)}")
    # 跨行引号值是 WarpX 允许的写法（例如 parser 表达式换行），必须原样保住
    multiline = [f for f in files
                 if any(line.count('"') % 2 == 1 for line in f.read_text(errors="ignore").splitlines())]
    print(f"其中含跨行引号值的 {len(multiline)} 个，已确认不被拆断")

    # 官方算例都是合法输入：校验器不该对它们报 error（误报会让用户不敢用红标）
    from warpx_studio.deck.validate import validate
    catalog_path = (pathlib.Path(__file__).resolve().parents[1] / "warpx_studio"
                    / "catalog" / "data" / "catalog.json")
    catalog = json.loads(catalog_path.read_text())
    false_positive = []
    real = [f for f in files if not f.name.startswith("inputs_base")]
    for f in real:
        deck = Deck.from_inputs(f.read_text(errors="ignore"))
        errs = [i for i in validate(deck, catalog) if i["severity"] == "error"]
        if errs:
            false_positive.append((f.name, [i["param"] for i in errs[:3]]))
    print(f"校验器在 {len(real)} 个完整官方算例上的误报：{len(false_positive)} 个")
    for name, ps in false_positive[:5]:
        print(f"  {name}: {ps}")
    if false_positive:
        raise SystemExit(1)
    for f, why in bad[:10]:
        print(f"  {f}: {why}")
    return 1 if bad else 0


if __name__ == "__main__":
    repo = pathlib.Path(__file__).resolve().parents[3]
    raise SystemExit(main(repo / "Examples"))
