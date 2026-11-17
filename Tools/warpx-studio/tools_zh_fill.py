"""给缺少中文说明的参数补上汉语注释（不翻译物理量、取值与代码标识符）。

优先用人工词条 ZH_EXTRA；剩下的按参数名最后一段的中文词典 GLOSS 组句，
例如 `<diag_name>.format` → "诊断输出格式：plotfile / openpmd / checkpoint …"。
"""
from __future__ import annotations

import json
import pathlib
import re

DATA = pathlib.Path(__file__).parent / "warpx_studio/catalog/data"

ZH_EXTRA: dict[str, str] = {
    # 诊断
    "<diag_name>.format": "诊断输出格式：plotfile（AMReX 自带，ParaView 可直开）/ openpmd（HDF5，通用）/ checkpoint（续算用）",
    "<diag_name>.diag_type": "诊断类型：Full 全量输出 / TimeAveraged 时间平均 / BackTransformed 背变换（boosted frame）",
    "<diag_name>.species": "这个诊断要输出哪些粒子的数据（物种名列表，空格分隔）",
    "<diag_name>.write_species": "1 = 同时输出粒子数据，0 = 只输出场",
    "<diag_name>.file_min_digits": "文件名里迭代编号至少占几位（补零），例如 5 → 00042",
    "<diag_name>.openpmd_backend": "openPMD 后端：h5（HDF5，单机最常用）/ adios2",
    "<diag_name>.openpmd_encoding": "openPMD 写出方式：group-based（一份文件一帧）/ variable-based（每变量一文件）",
    "<diag_name>.coarsening_ratio": "诊断时对网格做粗化的倍数（>1 表示降采样输出）",
    "<diag_name>.diag_lo": "诊断输出区域的低端角点坐标（米）；留空 = 整个计算域",
    "<diag_name>.diag_hi": "诊断输出区域的高端角点坐标（米）；留空 = 整个计算域",
    "<diag_name>.particle_fields_to_plot": "要额外输出的粒子量（如 weight、charge、momentum）",
    "<diag_name>.particle_fields_species": "上面那些粒子量取哪些物种",
    "<species_name>.random_fraction": "每个粒子被随机保留的概率（0–1），做粒子稀疏化输出时用",
    "<species_name>.uniform_stride": "每隔多少个粒子输出一个（均匀抽样）",
    # 物种注入
    "<species_name>.density_max": "密度剖面的最大值（m^-3），配合 profile 使用",
    "<species_name>.density_min": "密度剖面的最小值（m^-3），配合 profile 使用",
    "<species_name>.profile": "空间密度剖面：constant 均匀 / parse_density_function 用函数 / 其它见文档",
    "<species_name>.flux_profile": "面注入的流量剖面：constant 常数 / parse_flux_function 用函数",
    "<species_name>.injection_sources": "注入源个数（NFluxPerCell 连续注入时可分多个源）",
    "<species_name>.do_continuous_injection": "1 = 随时间持续注入粒子（来流），0 = 只在初始化时铺一次",
    "<species_name>.species_type": "物种类型：plasma / photon（光子）/ …；一般用 plasma",
    "<species_name>.save_particles_at_xlo": "1 = 保存从低端 x 面离开的粒子（用于统计通量）",
    "<species_name>.save_particles_at_xhi": "1 = 保存从高端 x 面离开的粒子",
    "<species_name>.save_particles_at_ylo": "1 = 保存从低端 y 面离开的粒子",
    "<species_name>.save_particles_at_yhi": "1 = 保存从高端 y 面离开的粒子",
    "<species_name>.save_particles_at_zlo": "1 = 保存从低端 z 面离开的粒子",
    "<species_name>.save_particles_at_zhi": "1 = 保存从高端 z 面离开的粒子",
    "<species_name>.save_particles_at_eb": "1 = 保存撞到导体（EB）面上被吸收/反射的粒子",
    # 算法
    "algo.particle_pusher": "粒子推进器：boris（默认，适合普通等离子体）/ vay（大步长相对论更准）/ hybrid / kinetic_energy",
    "algo.field_gathering": "场收集方式：energy-conserving / momentum-conserving / direct",
    "algo.charge_deposition": "电荷沉积方式：standard / momentum-conserving",
    # 其它
    "amr.restart": "从哪个 checkpoint 续算：写文件名或 0/1（1 = 取最近一个）",
    "amr.ref_ratio_vect": "每个方向各自的加密比（比统一的 amr.ref_ratio 更灵活）",
    "collisions.correct_energy_momentum": "1 = 碰撞后强制修正能量与动量守恒",
    "collisions.energy_fraction": "碰撞能量分配比例（0–1）",
    "<diag_name>.<species_name>.random_fraction": "每个粒子被随机保留的概率（0–1）：1 = 全部输出，0.1 = 只输出十分之一",
    "<diag_name>.<species_name>.uniform_stride": "每隔多少个粒子输出一个（均匀抽样，与 random_fraction 二选一）",
    "<species_name>.ionization_initial_level": "初始电离能级：例如 0 表示中性原子，1 表示一次电离态",
    "<species_name>.resampling_min_ppc": "重采样后的目标最小每格粒子数",
    "jacobian.pc_type": "雅可比矩阵的预条件子类型：none / diagonal / lu",
    "particle_thermalizer.theta": "热化器表面的倾斜角（度）",
    "<diag_name>.adios2_engine.parameters.*": "ADIOS2 引擎的参数（键值对，原样传给底层引擎，如 Threads=… ）",
    "<diag_name>.adios2_operator.parameters.*": "ADIOS2 压缩算子的参数（键值对，如 accuracy=… ）",
    "<diag_name>.intervals": "每隔多少步输出一次（整数）；写 0 表示不输出",
    "<diag_name>.time_average_mode": "时间平均的取样方式：fixed_start 从固定步开始 / dynamic_start 按需自动开始",
    "external_vector_potential.<field_name>.read_from_file": "1 = 该矢势分量从文件读，而不是用解析函数",
    "warpx.E_ext_grid_init_style": "外部电场初始化方式：constant 固定值（配 warpx.E_external_grid）/ parse_E_ext_grid_function 用解析函数",
    "warpx.B_ext_grid_init_style": "外部磁场初始化方式：constant 固定值（配 warpx.B_external_grid）/ parse_B_ext_grid_function 用解析函数",
    # ===== 第二批：控制 / 几何 / 求解器 / 边界 / 物种 / 诊断 =====
    "max_step": "仿真跑多少步（整数）。与 stop_time 谁先到就停",
    "stop_time": "仿真的物理时间上限（秒）。可与 max_step 二选一；两个都给时谁先到停谁",
    "warpx.cfl": "实际时间步相对 CFL 极限的比例：dt = cfl·min(dx,dz)/c，通常 0.5–0.9",
    "amr.n_cell": "最粗层的每方向网格数（二维写 nx nz，对应 x、z）",
    "geometry.prob_hi": "计算域上界角点坐标（米）：二维写 x z 两个数，三维写 x y z",
    "algo.maxwell_solver": "麦克斯韦场求解器：yee 标准 FDTD / ckc 色散修正 / psatd 谱方法 / ect 电荷守恒 Yee / hybrid 流体电子",
    "algo.current_deposition": "电流沉积算法：direct 直接法（快但不守恒）/ esirkepov 电荷守恒 / villasenor / vay",
    "boundary.field_lo": "场在低端（x、z 负向）的边界条件，按方向分别给：periodic / pec 导体壁 / pml / open / absorbing",
    "boundary.field_hi": "场在高端（x、z 正向）的边界条件，取值同上",
    "boundary.particle_lo": "粒子在低端边界的条件，按方向分别给：periodic / absorbing 吸收 / reflecting 反射 / thermal 热化",
    "boundary.particle_hi": "粒子在高端边界的条件，按方向分别给：periodic / absorbing / reflecting / thermal",
    "particles.species_names": "参与模拟的粒子种类名列表（空格分隔）；每个名字对应一组 名字.charge、名字.mass 等参数",
    "<species_name>.injection_style": "粒子怎么放进模拟：none 不放 / NUniformPerCell 每格均匀放 / NFluxPerCell 从面持续注入（来流）",
    "<species_name>.momentum_distribution_type": "初始动量分布：at_rest 静止 / constant 单能 / gaussian 高斯 / maxwellian 麦克斯韦 / uniform",
    "<species_name>.xmin": "粒子初始填充区域的 x 下界（米）；与 xmax 一起限定范围",
    "<species_name>.xmax": "粒子初始填充区域的 x 上界（米）",
    "<species_name>.ymin": "粒子初始填充区域的 y 下界（米）；二维没有 y，写 0 或省略",
    "<species_name>.ymax": "粒子初始填充区域的 y 上界（米）；二维忽略",
    "<species_name>.zmin": "粒子初始填充区域的 z 下界（米）",
    "<species_name>.zmax": "粒子初始填充区域的 z 上界（米）",
    "<diag_name>.fields_to_plot": "要输出的场：标量 rho / part_per_cell / divE / divB 等，矢量写分量 Ex Ey Ez Bx By Bz jx jy jz",
    "<diag_name>.file_prefix": "输出文件名前缀（可含子目录，如 diags/fields）",
    "<diag_name>.additional_fields_to_plot": "在 fields_to_plot 之外额外输出的场",
    "<diag_name>.average_start_step": "时间平均从第几步开始（仅在 fixed_start 模式下有效）",
    "<diag_name>.buffer_flush_limit_btd": "ADIOS 后端每累计多少个缓冲就写盘一次",
    "<diag_name>.buffer_size": "背变换诊断（BackTransformed）每次缓冲多少帧",
    "<diag_name>.dump_last_timestep": "1 = 最后一步无论是否到间隔都输出一次",
    "<diag_name>.dump_rz_modes": "RZ 下是否输出所有方位角模式：1 是 / 0 否",
    "<diag_name>.dz_snapshots_lab": "背变换诊断在实验室系快照之间的 z 间距（米）",
    "<diag_name>.particle_fields.<field_name>(x,y,z,ux,uy,uz)": "按粒子位置/动量求值的解析函数，结果按格子平均后输出",
    "<diag_name>.particle_fields.<field_name>.filter(x,y,z,ux,uy,uz)": "返回真假的过滤函数：只有为真的粒子参与上面的平均",
    "<diag_name>.<species_name>.additional_variables": "额外输出的粒子量（如 weight、charge、momentum）",
    "<diag_name>.<species_name>.plot_filter_function(t,x,y,z,ux,uy,uz)": "返回真/假的筛选函数：决定某个粒子是否被写入输出",
    "<diag_name>.<species_name>.variables": "要输出的粒子量列表：x y z（位置）、ux uy uz（动量）、w（权重）等",
    "<diag_name>.plot_raw_fields": "1 = 输出未做格心平均的原始（交错）场分量",
    "<diag_name>.plot_raw_fields_guards": "配合 plot_raw_fields=1：是否把 ghost 格也写出来",
    "<diag_name>.sensei_config": "SENSEI 后端的配置文件路径（仅 format = sensei 时读）",
    "<diag_name>.sensei_pin_mesh": "1 = 把网格左下角钉在原点（仅 SENSEI 后端）",
    "psatd.periodic_single_box_fft": "1 = 做 FFT 时不把 ghost 格算进去（要求整个域是单块周期网格）",
    "psatd.current_correction": "1 = 在傅里叶空间做电流修正以保证电荷守恒（PSATD 默认开）",
    "psatd.do_time_averaging": "1 = 使用时间平均的 Galilean PSATD（可压低 NCI 噪声）",
    # ===== 第三批：warpx.* 内部开关 / 粒子 / 诊断 / PSATD / QED / hybrid =====
    "warpx.do_electrostatic": "静电模式：none 纯电磁 / labframe 逐求解泊松 / labframe_... 其它；开了就没有电磁波",
    "warpx.const_dt": "固定时间步长（秒）；设置后覆盖 CFL 自动步长",
    "warpx.grid_type": "网格布局：collocated 共位 / nodal 节点 / staggered 交错（Yee 默认）",
    "warpx.do_moving_window": "1 = 开启移动窗口，让计算域跟着束流一起走（省内存）",
    "warpx.moving_window_dir": "移动窗口沿哪个方向移动：x / y / z",
    "warpx.moving_window_v": "移动窗口的速度（以 c 为单位，通常取接近 1）",
    "warpx.start_moving_window_step": "从第几步开始启动移动窗口",
    "warpx.end_moving_window_step": "到第几步停止移动窗口",
    "warpx.num_mirrors": "移动窗口过程中被镜像的格数（通常与窗口宽度一致）",
    "warpx.mirror_z": "1 = 在 z 方向做镜像（配合移动窗口）",
    "warpx.mirror_z_width": "镜像区域宽度（米）",
    "warpx.mirror_z_npoints": "镜像区域内采样点数",
    "warpx.do_divb_cleaning": "1 = 开启 div(B) 清理（广义拉格朗日乘子法）",
    "warpx.do_dive_cleaning": "1 = 开启 div(E) 清理",
    "warpx.do_initial_div_cleaning": "1 = 初始化后立即做一次 div(B)/div(E) 清理",
    "warpx.do_pml_divb_cleaning": "1 = 在 PML 区域也做 div(B) 清理",
    "warpx.do_pml_dive_cleaning": "1 = 在 PML 区域也做 div(E) 清理",
    "warpx.do_pml_j_damping": "1 = 在 PML 内对电流做阻尼，抑制吸收层反射",
    "warpx.do_similar_dm_pml": "1 = 让 PML 的阻尼系数与网格间距匹配（默认开）",
    "warpx.pml_has_particles": "1 = PML 区域里也保留粒子（默认 0，粒子进入 PML 会被删掉）",
    "warpx.do_single_precision_comms": "1 = 网格间通信用单精度（省带宽，精度略降）",
    "warpx.do_shared_mem_charge_deposition": "1 = 电荷沉积走共享内存（多线程更快）",
    "warpx.do_shared_mem_current_deposition": "1 = 电流沉积走共享内存",
    "warpx.shared_mem_current_tpb": "共享内存电流沉积每个线程块的线程数",
    "warpx.shared_tilesize": "共享内存分块的格数上限",
    "warpx.do_current_centering": "1 = 把电流从交错位置平移到格心再算场（压 NCI）",
    "warpx.current_centering_nox": "电流平移在 x 方向用的模板宽度（格数）",
    "warpx.current_centering_noy": "电流平移在 y 方向用的模板宽度",
    "warpx.current_centering_noz": "电流平移在 z 方向用的模板宽度",
    "warpx.field_centering_nox": "场平移在 x 方向用的模板宽度",
    "warpx.field_centering_noy": "场平移在 y 方向用的模板宽度",
    "warpx.field_centering_noz": "场平移在 z 方向用的模板宽度",
    "warpx.use_filter_compensation": "1 = 对滤波造成的色散做补偿（与 warpx.use_filter 配合）",
    "warpx.filter_npass_each_dir": "每个方向做几次滤波（默认 1 1 1）",
    "warpx.n_rz_azimuthal_modes": "RZ 几何保留多少个方位角傅里叶模式；模式越多越准越慢",
    "warpx.n_current_deposition_buffer": "电流沉积缓冲区格数（用于负载均衡时的边界交换）",
    "warpx.n_field_gather_buffer": "场收集缓冲区格数",
    "warpx.numprocs": "每个方向划分多少个 MPI 进程（如 2 2 表示 x、z 各 2 个）",
    "warpx.roundrobin_sfc": "1 = 用轮转法分配空间填充曲线上的块（负载均衡更均匀）",
    "warpx.sort_particles_for_deposition": "1 = 沉积前先按格排序粒子，提高缓存命中率",
    "warpx.sort_bin_size": "排序时每个桶的格数",
    "warpx.sort_idx_type": "排序索引的整数类型：int / long / uint32_t 等",
    "warpx.split_high_density_boxes": "1 = 把粒子过多的高密度网格块再拆分",
    "warpx.split_high_density_boxes_threshold": "触发拆分的粒子数阈值",
    "warpx.split_high_density_boxes_min_box_size": "拆分后允许的最小块尺寸（格数）",
    "warpx.load_balance_intervals": "每隔多少步做一次负载均衡；0 = 不做",
    "warpx.do_dynamic_scheduling": "1 = 用 OpenMP 动态调度（负载不均时更稳）",
    "warpx.refine_plasma": "1 = 按等离子体密度自动加密网格",
    "warpx.refine_start_time": "等离子体自适应加密的开始时间（秒）",
    "warpx.refine_stop_time": "等离子体自适应加密的结束时间（秒）",
    "warpx.refine_region_change_time": "自适应加密区域更新的周期（秒）",
    "warpx.ref_patch_function(x,y,z)": "返回真/假的函数：标记需要加密的区域",
    "warpx.fine_tag_lo": "加密区域的下界角点（米）",
    "warpx.fine_tag_hi": "加密区域的上界角点（米）",
    "warpx.maxlevel_extEMfield_init": "外部电磁场初始化用到的最粗层编号",
    "warpx.projection_div_cleaner.atol": "投影式 div 清理的绝对容差",
    "warpx.projection_div_cleaner.rtol": "投影式 div 清理的相对容差",
    "warpx.field_io_nfiles": "场输出时每个文件包含多少块（仅部分格式）",
    "warpx.particle_io_nfiles": "粒子输出时每个文件包含多少块",
    "warpx.mffile_nstreams": "写 MultiFab 文件时的并行流数目",
    "warpx.dt_update_interval": "每隔多少步检查一次是否要调整时间步",
    "warpx.dt_update_diagnostic_file": "时间步调整诊断的输出文件名",
    "warpx.dt_update_write_interval": "时间步诊断每隔多少步写一次",
    "warpx.override_sync_intervals": "1 = 用固定间隔做同步（而不是按需同步）",
    "warpx.do_device_synchronize": "1 = 每步做设备同步（调试 GPU 竞态用，慢）",
    "warpx.quantum_xi": "量子参数 ξ（强场 QED 用）",
    "warpx.reduced_diags_names": "缩减诊断的名称列表",
    "warpx.synchronize_velocity_for_diagnostics": "1 = 输出诊断前同步粒子速度（GPU 上保证一致）",
    "warpx.eb_implicit_function": "导体几何的隐函数：F>0 是实体内部，F<0 是流体区，F=0 是表面",
    "particles.deposit_on_main_grid":
        "只在用网格加密（AMR）时有意义：把物种名写在这里（空格分隔，如 `ions electrons`），"
        "这些物种的电荷/电流就直接沉积到**主网格（最粗层）**，即使粒子位于加密区内部。"
        "留空 = 按粒子所在层正常沉积。注意这里填的是**物种名**，不是 0/1",
    "particles.gather_from_main_grid":
        "只在用网格加密（AMR）时有意义：写在这里的物种，取场时只从**主网格（最粗层）**取，"
        "即使粒子位于加密区内部。留空 = 按粒子所在层取场。填的是**物种名**，不是 0/1",
    "particles.do_tiling": "1 = 用 tile 并行（小块并行，适合多核）",
    "particles.use_fdtd_nci_corr": "1 = 开启 FDTD 的 NCI 修正滤波器",
    "particles.max_grid_crossings": "一个粒子一步内最多跨几个网格（超出则拆步）",
    "particles.crop_on_PEC_boundary": "1 = 把落在 PEC 边界外的粒子剔掉",
    "particles.photon_species": "光子物种名（用于 QED 光子追踪）",
    "particles.read_fields_from_path": "从文件读取粒子初始所在场数据的路径",
    "particles.rigid_injected_species": "注入后不做推近的物种（保持刚性轨道）",
    "particles.repeated_plasma_lens_period": "重复等离子体透镜的周期（米）",
    "particles.E_ext_particle_init_style": "粒子的外部电场初始化方式：constant / parse_E_ext_particle_function / read_from_file",
    "particles.B_ext_particle_init_style": "粒子的外部磁场初始化方式，取值同上",
    "external_vector_potential.fields": "外部矢势包含哪些分量（如 A_z），逗号分隔",
    "external_vector_potential.read_from_file": "1 = 矢势从文件读而不是用解析函数",
    "external_vector_potential.do_diva_cleaning": "1 = 对矢势做 div(A) 清理",
    "external_vector_potential.<field_name>.A[x,y,z]_external_grid_function(x,y,z)": "外部矢势分量的解析表达式（空间相关）",
    "external_vector_potential.<field_name>.A_time_external_grid_function(t)": "外部矢势的时间因子（时间相关必须写在这里）",
    "hybrid_pic_model.add_external_fields": "1 = 在 hybrid 模型里叠加外部电磁场（来自矢势）",
    "hybrid_pic_model.gamma": "电子绝热指数 γ（通常 1 或 5/3）",
    "hybrid_pic_model.include_joule_heating": "1 = 把焦耳加热计入电子能量方程",
    "hybrid_pic_model.joule_redirect_Te_threshold": "焦耳加热重定向的电子温度阈值（eV）",
    "hybrid_pic_model.solve_electron_energy_equation": "1 = 解电子能量方程（否则电子温度取常数）",
    "hybrid_pic_model.holmstrom_vacuum_region": "1 = 在真空区用 Holmström 的真空分支（避免除零）",
    "hybrid_pic_model.vacuum_seam_switch_mode": "真空/等离子体交界处的开关策略",
    "hybrid_pic_model.max_substep_attempts": "自适应子步允许的最大尝试次数（超过就 abort）",
    "hybrid_pic_model.substep_rtol": "子步自适应控制的相对容差",
    "hybrid_pic_model.substep_atol": "子步自适应控制的绝对容差",
    "hybrid_pic_model.substep_safety": "子步安全系数（越小越保守）",
    "hybrid_pic_model.substep_max_growth": "单次允许的子步最大增长倍数",
    "hybrid_pic_model.plasma_resistivity(rho,J,t)": "等离子体电阻率（Ω·m），可为常数或表达式",
    "hybrid_pic_model.plasma_hyper_resistivity(rho,B)": "等离子体超电阻率（Ω·m³），用于抑制网格尺度噪声",
    "hybrid_pic_model.plasma_resistivity_<species>(rho_s,rho,Te,J,J_s,B,t)": "按物种给的电阻率贡献",
    "hybrid_pic_model.electron_ion_relaxation_rate(rho,Te,Ti,t)": "电子-离子弛豫速率（1/s），可为表达式",
    "hybrid_pic_model.Jx_external_grid_function(x,y,z,t)": "外部电流密度的 x 分量解析式（A/m²）",
    "hybrid_pic_model.Jy_external_grid_function(x,y,z,t)": "外部电流密度的 y 分量解析式",
    "hybrid_pic_model.Jz_external_grid_function(x,y,z,t)": "外部电流密度的 z 分量解析式",
    "macroscopic.epsilon": "介质 1 的介电常数（写 numeric*epsilon0）",
    "macroscopic.epsilon2": "介质 2 的介电常数（界面另一侧）",
    "macroscopic.sigma": "介质 1 的电导率（S/m）；0 = 无耗散介质",
    "macroscopic.epsilon_function(x,y,z)": "用解析函数给介电常数分布（优先于常数）",
    "macroscopic.sigma_function(x,y,z)": "用电导率的解析函数分布",
    "macroscopic.mu_function(x,y,z)": "用磁导率的解析函数分布",
    "algo.macroscopic_subpixel": "1 = 开启亚网格（subpixel）界面平滑，界面不在格点上时更准",
    "algo.macroscopic_sigma_method": "界面电导率的平均方式：harmonic 调和 / arithmetic 算术",
    "algo.load_balance_intervals": "每隔多少步做一次负载均衡",
    "algo.load_balance_costs_update": "负载代价的更新方式：heuristic / gpuclock / cpuclock",
    "algo.costs_heuristic_particles_wt": "启发式代价里每个粒子的权重",
    "algo.costs_heuristic_cells_wt": "启发式代价里每个网格的权重",
    "algo.load_balance_with_sfc": "1 = 用空间填充曲线做负载划分",
    "algo.load_balance_knapsack_factor": "背包算法的负载上限系数",
    "algo.load_balance_efficiency_ratio_threshold": "触发重新划分的效率比阈值",
    "collisions.shuffling_method": "碰撞配对前的打乱方式：percell / global",
    "collisions.split_momentum_push": "1 = 把动量推近拆成两步（部分碰撞器用）",
    "collisions.np_warning_threshold": "每格粒子数低于该值时给出警告",
    "diagnostics.enable": "1 = 启用该诊断 / 0 = 关闭",
    "reduced_diags.precision": "缩减诊断输出的小数位数",
    "reduced_diags.separator": "缩减诊断 CSV 的分隔符（如 , 或 空格）",
    "gmres.max_iterations": "GMRES 最大迭代次数",
    "gmres.absolute_tolerance": "GMRES 绝对收敛容差",
    "gmres.relative_tolerance": "GMRES 相对收敛容差",
    "gmres.verbose_int": "GMRES 输出详细程度（0 静默）",
    "newton.linear_solver": "牛顿迭代里用哪个线性求解器",
    "newton.pc_update_newton_interval": "每隔多少次牛顿迭代更新一次预条件子",
    "newton.pc_update_time_step_interval": "每隔多少个时间步更新一次预条件子",
    "jacobian.pc_type": "预条件子类型",
    "implicit_evolve.theta": "隐式时间推进的 theta（1 = 全隐式，0.5 = Crank-Nicolson）",
    "interpolation.galerkin_scheme": "1 = 用 Galerkin 插值（粒子形状与场插值一致）",
    "precon_mat.verbose": "预条件矩阵的调试输出级别",
    "psatd.JRhom": "1 = 电流修正里把 rho 也纳入（JRho 算法）",
    "psatd.update_with_rho": "1 = 用 rho 更新 E（而不是只靠电流）",
    "psatd.v_galilean": "Galilean 系速度（以 c 为单位），通常 0 0 0",
    "psatd.v_comoving": "共动系速度（以 c 为单位）",
    "psatd.use_default_v_galilean": "1 = 自动按 CFL 选 Galilean 速度",
    "psatd.use_default_v_comoving": "1 = 自动选共动系速度",
    "psatd.nx_guard": "PSATD 在 x 方向需要的 ghost 格数",
    "psatd.ny_guard": "PSATD 在 y 方向需要的 ghost 格数",
    "psatd.nz_guard": "PSATD 在 z 方向需要的 ghost 格数",
    "qed_bw.lookup_table_mode": "Breit-Wheeler 查表方式：builtin / generate / load",
    "qed_qs.lookup_table_mode": "量子同步辐射查表方式：builtin / generate / load",
    "qed_qs.photon_creation_energy_threshold": "产生光子所需的最小能量阈值（MeV）",
    "qed_schwinger.xmin": "Schwinger 场作用区的 x 下界（米）",
    "qed_schwinger.xmax": "Schwinger 场作用区的 x 上界（米）",
    "qed_schwinger.ymin": "Schwinger 场作用区的 y 下界（米）",
    "qed_schwinger.ymax": "Schwinger 场作用区的 y 上界（米）",
    "qed_schwinger.zmin": "Schwinger 场作用区的 z 下界（米）",
    "qed_schwinger.zmax": "Schwinger 场作用区的 z 上界（米）",
    "qed_schwinger.y_size": "Schwinger 场在 y 方向的厚度（米）",
    "qed_schwinger.threshold_poisson_gaussian": "高斯泊松抽样阈值",
    "qed_schwinger.ele_product_species": "Schwinger 产生的电子物种名",
    "qed_schwinger.pos_product_species": "Schwinger 产生的正电子物种名",
    "lasers.deposit_on_main_grid": "1 = 激光的电流只沉积到最粗层",
    "lattice.elements": "晶格外场包含哪些元件（逗号分隔）",
    "lattice.reverse": "1 = 晶格元件顺序反过来",
    "particle_thermalizer.momentum_threshold": "热化器触发的动量阈值",
    "particle_thermalizer.normal": "热化表面的法向（如 0 0 1）",
    "particle_thermalizer.start": "热化器作用的起始位置",
    "particle_thermalizer.theta": "热化器表面倾角",
    "eb2.geom_type": "EB 几何描述方式：all_regular 无实体 / implicit_function 隐函数 / stl 文件",
    "<species>.qed_virtual_photons_do_beam_size_effect": "1 = 虚光子谱里计入束斑尺寸效应",
    "<species_name>.addIntegerAttributes": "给粒子加整数属性（名字列表）",
    "<species_name>.addRealAttributes": "给粒子加实数属性（名字列表）",
    "<species_name>.do_adk_correction": "1 = 场致电离用 ADK 修正（考虑电离抑制）",
    "<species_name>.do_backward_propagation": "1 = 该物种反向传播（计算逆过程）",
    "<species_name>.do_classical_radiation_reaction": "1 = 计入经典辐射反作用",
    "<species_name>.do_not_deposit": "1 = 该物种不沉积电荷/电流",
    "<species_name>.do_not_gather": "1 = 该物种不从网格取场（只受外场）",
    "<species_name>.do_not_push": "1 = 该物种不推近（位置动量保持不动）",
    "<species_name>.initialize_self_fields": "1 = 初始化时解一次自场（避免初始不满足泊松方程）",
    "<species_name>.physical_element": "物理元素名（如 Al、H），用于查电离与热化参数",
    "<species_name>.radial_numpercell_power": "RZ 下粒子数沿半径的幂律分布指数",
    "<species_name>.random_theta": "1 = RZ 下方位角初始化为随机（否则取 0）",
    "<species_name>.resampling_algorithm": "粒子重采样算法：leveling_thinning / momentum_correction",
    "<species_name>.resampling_trigger_intervals": "每隔多少步检查一次重采样条件",
    "<species_name>.resampling_trigger_max_avg_ppc": "每格平均粒子数超过该值时触发重采样",
    "<species_name>.do_not_deposit":
        "1 = 该物种不把电荷/电流沉积到网格（例如只当示踪粒子，或电荷由别的模块提供）；0 = 正常沉积（默认）",
    "<species_name>.rigid_advance":
        "1 = 一步内直接把粒子推到终点而不按场推近（大权重/注入粒子用，省时间但不受电磁力）；0 = 正常推近（默认）",
    "<species_name>.self_fields_absolute_tolerance": "该物种自场泊松求解的绝对容差",
    "<species_name>.self_fields_required_precision": "该物种自场求解需要的相对精度",
    "<species_name>.zinject_plane": "面注入所在的 z 位置（米）",

    "algo.em_solver_medium": "介质处理方式：vacuum 真空 / constant 常数介电常数 / macroscopic 宏观介质（含界面与张量）",

    "<diag_name>.adios2_engine.type": "ADIOS2 引擎类型（如 bp4 / bp5 / sst）",
    "<diag_name>.adios2_operator.type": "ADIOS2 压缩算子类型（如 blosc / zfp）",
    "<species_name>.split_type": "粒子分裂方式：proton-impact-ionization（质子碰撞电离）",
    "external_vector_potential.<field_name>.path": "外部矢势文件路径（从文件读取 A 时用）",
    "fluids.species_names": "参与流体模块的物种名列表",
    "lasers.names": "激光名称列表（每个名字对应一组 <名字>.* 激光参数）",
    "particle_thermalizer.species": "参与热化器（thermalizer）的物种名列表",
    "qed_bw.chi_min": "Breit-Wheeler 光子产生的最小量子参数 χ",
    "qed_qs.chi_min": "量子同步辐射的最小量子参数 χ",
    "reduced_diags.extension": "缩减诊断输出文件的扩展名（如 .txt / .csv）",
    "reduced_diags.path": "缩减诊断输出文件的路径",
    "warpx.do_subcycling": "1 = 不同网格层用不同时间步（子循环），0 = 全局同一步长",
    "warpx.max_dt": "时间步长上限（秒）；CFL 算出的 dt 超过它时会被截断",
    "warpx.max_omegac_dt": "回旋频率限制：保证 omega_c*dt 不超过该值",
    "warpx.max_omegap_dt": "等离子体频率限制：保证 omega_p*dt 不超过该值",
}

GLOSS: dict[str, str] = {
    "random_fraction": "随机抽样比例", "uniform_stride": "均匀抽样间隔",
    "type": "类型", "format": "格式", "species": "物种列表", "profile": "剖面",
    "path": "文件路径", "name": "名称", "names": "名称列表", "steps": "步数",
    "time": "时间", "min_iters": "最大迭代次数", "max_iters": "最大迭代次数",
    "split_type": "分裂方式", "do_splitting": "是否做粒子分裂：1 开 / 0 关",
    "do_resampling": "是否做粒子重采样：1 开 / 0 关",
    "do_field_ionization": "是否做场致电离：1 开 / 0 关",
    "do_temperature_deposition": "是否沉积温度：1 开 / 0 关",
    "ionization_initial_level": "电离初态能级",
    "ionization_product_species": "电离产生的物种名",
    "self_fields_max_iters": "自场泊松求解的最大迭代次数",
    "qed_quantum_sync_phot_product_species": "量子同步辐射产生的光子物种名",
    "qed_breit_wheeler_ele_product_species": "Breit-Wheeler 产生的电子物种名",
    "qed_breit_wheeler_pos_product_species": "Breit-Wheeler 产生的正电子物种名",
    "beta_weight_exponent": "权重因子指数（碰撞用）",
    "energy_correction_sort_by_weight": "能量修正时是否按权重排序：1 是 / 0 否",
    "do_average": "是否对该量做时间平均：1 是 / 0 否",
    "do_back_transformed_fields": "是否输出背变换（boosted frame）的场：1 是 / 0 否",
    "do_back_transformed_particles": "是否输出背变换的粒子：1 是 / 0 否",
    "num_snapshots_lab": "实验室系快照数量",
    "dt_snapshots_lab": "实验室系快照时间间隔（秒）",
    "average_period_steps": "时间平均的取样间隔（步）",
    "average_period_time": "时间平均的取样间隔（秒）",
}


JUNK = "参数含义见右侧取值与提示"


def fill(name: str, current: str) -> str:
    """只在"没有说明"或"是兜底废话"时才补；已经有像样中文的绝不动它。"""
    cur = (current or "").strip()
    good = len(cur) >= 6 and JUNK not in cur
    if name in ZH_EXTRA:
        return ZH_EXTRA[name]
    if good:
        return current
    tail = name.split(".")[-1]
    tail = re.sub(r"\(.*\)$", "", tail)
    if tail in GLOSS:
        return GLOSS[tail]
    if current.strip() and len(current.strip()) >= 4 and not re.search(r"[A-Za-z]{4,}", current):
        return current
    # 最后兜底：用参数名分段组成"分组：末段"
    parts = [p for p in re.split(r"[.]", name) if p and not p.startswith("<")]
    if parts:
        return f"{' · '.join(parts)}（参数含义见右侧取值与提示）"
    return "参数含义见右侧取值与提示"


def main() -> int:
    zh_path = DATA / "zh.json"
    zh = json.loads(zh_path.read_text(encoding="utf-8"))
    catalog = json.loads((DATA / "catalog.json").read_text(encoding="utf-8"))
    params = zh["params"]
    filled = 0
    for p in catalog["params"]:
        name = p["name"]
        cur = params.get(name, "")
        new = fill(name, cur)
        if new != cur:
            params[name] = new
            filled += 1
    zh_path.write_text(json.dumps(zh, ensure_ascii=False, indent=1) + "\n", encoding="utf-8")
    remaining = [p["name"] for p in catalog["params"]
                 if len((params.get(p["name"]) or "").strip()) < 6]
    print(f"补充 {filled} 条；仍过短 {len(remaining)} 条")
    for n in remaining[:6]:
        print("  ", n, "->", params.get(n))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
