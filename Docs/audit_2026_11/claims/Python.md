# Python 工作日志

claim: Python F2,F6,F9 @lead 2026-11-15T19:50

F2 invalid（以 26.11 为基准）: 上游自己在 26.10→26.11 之间删掉了这个 API ——
  `git log -S deposit_charge_density 26.10..26.11 -- Python/pywarpx/particle_containers.py`
  → `dbea38aa8 Remove the Python function deposit_charge_density (#7439)`；
  `git show 26.10:... | grep -c` = 1，`git show 26.11:... | grep -c` = 0。fork 与 26.11 一致。
  （C++ 侧的 `deposit_charge` 绑定仍在 `PicEm/Modules/Python/Particles/WarpXParticleContainer.cpp:120`，
   需要该功能的用户仍可走绑定的低层接口。）
F6 fixed: pydantic 重构把一批"上游是普通属性"的状态改成了 `PrivateAttr`，而只给 `species`/`laser`/
  `diagnostic` 留了同名 property。现在补回只读 property：`Species.species_type`/`element`/`species_number`、
  `GaussianLaser.laser_number`、`AnalyticLaser.laser_number`/`mangle_dict`（带 setter）、
  `Simulation.inputs_initialized`/`warpx_initialized`/`finalized`。
  验证: 装好 pywarpx 后 `Species(particle_type='proton').species_type == 'proton'`、
  `Simulation().inputs_initialized is False`、`GaussianLaser(...).laser_number is None`（初始化前）——均不再
  AttributeError（此前第三方脚本会挂）。
F9 fixed: 三点分别处理——`python_requires=">=3.11"` 保留但加注释说明它来自 `typing.Self` 的真实需求
  （并提示下游发行版可见）；`dependencies.json` 增加 `_note_picmi` 说明 `version_picmi` 才是
  `install_requires` 实际安装的 pin、`commit_picmi` 只是该版本的来源提交（两者不会在安装时冲突）；
  `Python/pywarpx.egg-info/` 已确认**未被跟踪**，且 `.gitignore:29` 已排除。
