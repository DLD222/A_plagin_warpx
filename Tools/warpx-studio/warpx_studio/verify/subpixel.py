"""电介质界面的 subpixel 平滑：面心等效介电常数。

物理要求：`E_tangential` 连续、`D_normal` 连续。subpixel 平滑（Farjadpour 2006,
Kottke 2008）不做几何切割，只按体积分数在面上做**方向性平均**：

* 法向分量：`ε_face = 1 / ( f/ε2 + (1-f)/ε1 )`   （调和平均，对应 D_n 连续）
* 切向分量：`ε_face = f·ε2 + (1-f)·ε1`            （算术平均，对应 E_t 连续）

这样既保持二阶精度，也不产生伪电荷。函数是纯算术，便于单元验证，C++ 侧只需把结果
填到面心 `eps_r`（`algo.em_solver_medium = macroscopic` 的路径）。
"""
from __future__ import annotations


def face_epsilon(volfrac: float, eps1: float, eps2: float, normal: bool) -> float:
    """界面穿过一个面时的等效 ε。

    Parameters
    ----------
    volfrac : float
        该面另一侧材料（ε2）占的面积/体积分数，0..1。
    eps1, eps2 : float
        两种材料的介电常数。
    normal : bool
        True  → 面法向分量（用调和平均，保证 D_n 连续）。
        False → 面切向分量（用算术平均，保证 E_t 连续）。
    """
    f = min(max(volfrac, 0.0), 1.0)
    if f <= 0.0:
        return eps1
    if f >= 1.0:
        return eps2
    if normal:
        return 1.0 / (f / eps2 + (1.0 - f) / eps1)
    return f * eps2 + (1.0 - f) * eps1


def reflection_coefficient(eps1: float, eps2: float, normal: bool) -> float:
    """正入射时的反射系数（用于与 Fresnel 解对照）。"""
    import math
    n1, n2 = math.sqrt(eps1), math.sqrt(eps2)
    return abs((n1 - n2) / (n1 + n2))
