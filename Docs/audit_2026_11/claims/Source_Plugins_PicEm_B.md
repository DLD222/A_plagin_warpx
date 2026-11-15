# Source_Plugins_PicEm_B 工作日志

claim: Source_Plugins_PicEm_B F4 @lead-tools 2026-11-15T18:30
F4 fixed by HASH(F4); set_legacy_data 改为返回 WarpXRuntimeData::Legacy()（每次调用刷新 m_warpx），set_legacy_host 改为 thread_local std::optional<WarpXHostServices> 每次 emplace(GetInstance())（地址稳定、每次重建引用）。验证(双实例探针: 用 tests/runtime/CMakeFiles/test_runtime_snapshot_2d.dir 的 flags/link.txt 编译链接 /tmp/dbl.cpp，同一进程 GetInstance→ResetInstance→(等大小 spacer)→GetInstance，比较 legacy_data().amrCore() 与当前实例): 新代码 round0/round1 均 yes ✓；反证: 临时改回旧 static 绑定 → round1 输出 NO（适配器仍指向已析构实例），已还原 ✓
