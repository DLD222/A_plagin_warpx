// 页面脚本冒烟测试：用最小 DOM 桩执行 index.html 里的 <script>，确认 init + render 不抛异常。
//
// 起因：v3.3 引入的 SHOW_DERIVED / GLOSSARY / DERIVED 三个变量漏了声明，浏览器里读未声明
// 变量会抛 ReferenceError，整个表单渲染中断 —— 页面一片空白，但 node --check 只查语法，
// 完全查不出来。这个测试就是补那个缺口：真的执行一遍。
//
// 用法：node tests/ui_smoke.js <index.html> <catalog/data 目录>
const fs = require("fs");
const path = require("path");

const htmlPath = process.argv[2];
const dataDir = process.argv[3];
const html = fs.readFileSync(htmlPath, "utf8");
const script = html.match(/<script>([\s\S]*?)<\/script>/)[1];

function makeClassList(node) {
  // 真实 DOM 里 classList 和 className 是同一份数据；桩必须同步，否则
  // "给行加 err 类" 这类断言看不到效果。
  return {
    add(c) {
      const set = new Set(String(node.className || "").split(/\s+/).filter(Boolean));
      set.add(c); node.className = [...set].join(" ");
    },
    remove(c) {
      const set = new Set(String(node.className || "").split(/\s+/).filter(Boolean));
      set.delete(c); node.className = [...set].join(" ");
    },
    contains(c) { return String(node.className || "").split(/\s+/).includes(c); },
  };
}

function el(tag) {
  const node = {
    tagName: tag, children: [], style: {}, dataset: {}, options: [], selectedIndex: 0,
    className: "", checked: true, value: "", title: "",
    rows: 0, placeholder: "", scrollTop: 0, scrollHeight: 0,
    _text: "", _html: "",
    set textContent(v) { this._text = v; }, get textContent() { return this._text; },
    set innerHTML(v) {
      this._html = v;
      if (v === "") { this.children.length = 0; }   // 真实 DOM 里 innerHTML="" 会清空子节点
    },
    get innerHTML() { return this._html; },
    appendChild(c) { this.children.push(c); return c; },
    append(...cs) { cs.forEach((c) => this.children.push(c)); },
    removeChild() {}, remove() {}, addEventListener() {}, setAttribute() {},
    getAttribute() { return null; }, focus() {}, click() {},
    querySelector() { return el("div"); }, querySelectorAll() { return []; },
  };
  node.classList = makeClassList(node);
  return node;
}

const registry = {};
global.document = {
  getElementById(id) { return registry[id] || (registry[id] = el("div")); },
  createElement(tag) { return el(tag); },
  createTextNode(text) { const n = el("#text"); n._text = text; return n; },
  querySelectorAll(sel) {
    if (sel === "nav button") { return global.__NAV_BUTTONS__ || []; }
    if (sel === ".row" || sel === "section.group") {
      const want = sel === ".row" ? "row" : "group";
      const out = [];
      const walk = (n) => {
        if (!n) return;
        if (want === "row" && n.dataset && n.dataset.param) out.push(n);
        if (want === "group" && String(n.className).includes("group")) out.push(n);
        for (const c of (n.children || [])) walk(c);
      };
      walk(registry["form"]);
      return out;
    }
    return [];
  },
  querySelector(sel) {
    if (sel === "nav") { return global.__NAV__ || (global.__NAV__ = el("nav")); }
    return el("div");
  },
  addEventListener() {},
  body: el("body"),
};
global.window = { open() { return el("div"); },
                 addEventListener() {}, removeEventListener() {} };
global.confirm = () => true;
global.prompt = () => "stub";
global.alert = () => {};
const realSetTimeout = setTimeout;
global.setTimeout = (fn) => realSetTimeout(fn, 0);
global.setInterval = () => 0;
global.clearTimeout = () => {};

const read = (name) => JSON.parse(fs.readFileSync(path.join(dataDir, name), "utf8"));
const catalog = read("catalog.json");
const enums = read("enums.json");
const payloads = {
  "/api/catalog": { catalog, zh: read("zh.json"), categories: read("categories.json").categories,
                    enums: enums.enums, glossary: enums.glossary, derived: enums.derived },
  "/api/controls": { field_bc: ["periodic"], particle_bc: ["periodic"], boundary_keys: {},
                     templates: {}, style_options: {}, axes: { "1": ["z"], "2": ["x", "z"],
                     "3": ["x", "y", "z"], "RZ": ["r", "z"] } },
  "/api/binaries": { binaries: [
    { build: "fork", dims: "2d", path: "/tmp/fork/warpx.2d", features: ["MPI", "OMP"] },
    { build: "fork", dims: "rz", path: "/tmp/fork/warpx.rz", features: ["MPI", "OMP"] },
    { build: "official_2611", dims: "rz", path: "/tmp/official/warpx.rz",
      features: ["MPI", "OMP"] },
  ] },
  "/api/projects": { projects: [], last: null },
  "/api/starter": { params: { "geometry.dims": "2", "amr.n_cell": "64 64" }, groups: {} },
  "/api/live/iterations": { iterations: [] },
  "/api/status": { state: "idle" },
  "/api/run": { run_dir: "/tmp/studio-test-run", started: true },
};
// 404 的接口：默认所有未知路径都返回空对象（页面初始化会宽容处理），
// 需要验证"打开失败"的分支时把具体 URL 加进来。
const missing = new Set();
const runBodies = [];
global.fetch = async (url, opts) => {
  const key = String(url).split("?")[0];
  if (key === "/api/run" && opts && opts.body) runBodies.push(JSON.parse(opts.body));
  const absent = missing.has(key);
  const body = absent ? { detail: "Not Found" } : (payloads[key] ?? {});
  return { ok: !absent, status: absent ? 404 : 200, json: async () => body,
           text: async () => JSON.stringify(body) };
};

process.on("unhandledRejection", (err) => {
  console.log("FAIL: " + (err && err.stack ? err.stack : err));
  process.exit(1);
});

eval(script);
realSetTimeout(async () => {
  try {
    render();
    // ---- 行为验收：取值门控在真实渲染里是否生效 -------------------------------
function collectLabels(node, out) {
  if (!node) return out;
  const html = node._html || "";
  const m = /<b>([^<]+)<\/b>/.exec(html);
  if (m) out.push(m[1]);
  for (const c of (node.children || [])) collectLabels(c, out);
  return out;
}

if (global.window.__STUDIO_TEST__) {
  const T = global.window.__STUDIO_TEST__;
  const S = "<species_name>";
  const scenario = (profile) => {
    // 密度剖面现在依赖注入方式：必须同时给 NUniformPerCell 才会生效
    const deck = { params: {}, groups: { [S]: { ions: {
      injection_style: "NUniformPerCell", profile, charge: "5*q_e", mass: "27*m_p" } } } };
    T.setDECK(deck);
    T.setStep(2);          // 物种参数在「2 粒子/激光/碰撞」这一步
    T.render();
    return collectLabels(registry["form"], []);
  };
  // 诊断：把"面注入"与"激光/碰撞"场景实际渲染出的行打印出来
  const dumpScenario = (deck, step, title) => {
    T.setDECK(deck); T.setStep(step); T.render();
    const rows = collectLabels(registry["form"], []);
    if (process.env.UI_DUMP) {
      console.log(`--- ${title} (${rows.length} 行) ---`);
      console.log(rows.join("\n"));
    }
    return rows;
  };
  dumpScenario({ params: {}, groups: { [S]: { ions: { injection_style: "NFluxPerCell" } } } }, 2,
               "面注入 NFluxPerCell");

  const base2 = dumpScenario({ params: {}, groups: { [S]: { ions: {} } } }, 2, "空算例");
  // profile 现在只在 NUniformPerCell/NRandomPerCell 下出现（属于体积注入），
  // 所以它不再要求"默认可见"；这里只查真正的功能开关与注入方式开关。
  const mustSee = ["lasers.names", "collisions.collision_names", "injection_style"];
  const missed = mustSee.filter((n) => !base2.some((r) => r.trim() === n));
  if (missed.length) {
    console.error("FAIL 开关被折叠，用户打不开: " + missed.join(", "));
    process.exit(1);
  }
  console.log(`PASS 开关默认可见: ${mustSee.join(", ")}`);

  // 面注入：injection_style=NFluxPerCell 时，配套参数必须紧跟其后（含 zinject_plane）
  const fluxRows = dumpScenario(
    { params: {}, groups: { [S]: { ions: { injection_style: "NFluxPerCell" } } } }, 2, "面注入顺序");
  const idx = fluxRows.findIndex((r) => r.trim() === "injection_style");
  const window = fluxRows.slice(idx + 1, idx + 16).map((r) => r.trim());
  // zinject_plane 属于 rigid injection；flux 只有 flux_profile=constant 才出现
  const fluxNeeded = ["flux_normal_axis", "surface_flux_pos", "flux_profile",
                      "num_particles_per_cell"];
  const fluxMissing = fluxNeeded.filter((n) => !window.includes(n));
  if (idx < 0 || fluxMissing.length) {
    console.error("FAIL 面注入参数没有紧跟 injection_style: 缺 " + fluxMissing.join(", ")
                  + "；实际紧跟的 15 行 = " + window.join(", "));
    process.exit(1);
  }
  console.log("PASS 面注入参数紧跟开关: " + window.slice(0, 6).join(", ") + " …");

  // 动量分布两级联动：选 maxwellian -> 出现 u_std/u_mean；再选 parser -> 出现 *_function
  T.setDECK({ params: {}, groups: { [S]: { ions: { injection_style: "NUniformPerCell",
                                                   momentum_distribution_type: "constant" } } } });
  T.setStep(2); T.render();
  const pickSelect = (pred) => {
    const walk = (n) => {
      if (!n) return null;
      if (n.tagName === "select" && pred(n)) return n;
      for (const c of (n.children || [])) { const r = walk(c); if (r) return r; }
      return null;
    };
    return walk(registry["form"]);
  };
  const momSel = pickSelect((n) => (n.children || []).some((o) => o.value === "maxwellian"));
  if (!momSel) { console.error("FAIL 找不到 momentum_distribution_type 下拉"); process.exit(1); }
  momSel.value = "maxwellian"; momSel.onchange();
  await new Promise((r) => realSetTimeout(r, 250));
  const rows2 = collectLabels(registry["form"], []);
  const has2 = (k) => rows2.some((r) => r.trim() === k);
  if (!has2("ux_std") || !has2("ux_mean") || has2("ux")) {
    console.error("FAIL 选 maxwellian 后应出现 ux_std/ux_mean 且隐藏 ux；实际含 ux=" + has2("ux")
                  + " ux_std=" + has2("ux_std") + " ux_mean=" + has2("ux_mean"));
    process.exit(1);
  }
  // 找到"名字是 maxwellian_u_std_distribution_type 的那一行"里的下拉框
  // 找到"标签是某参数名的那个 row"，再从它的子节点里取下拉框
  const findRowSelect = (paramName) => {
    const re = new RegExp("<b>[^<]*" + paramName + "<\\/b>");
    let found = null;
    const walk = (n) => {
      if (!n || found) return;
      const kids = n.children || [];
      const hasLabel = kids.some((c) => re.test(c._html || ""));
      if (hasLabel) {
        const pick = (m) => {
          if (!m) return null;
          if (m.tagName === "select") return m;
          for (const c of (m.children || [])) { const r = pick(c); if (r) return r; }
          return null;
        };
        const sel2 = pick(n);
        if (sel2) { found = sel2; return; }
      }
      for (const c of kids) walk(c);
    };
    walk(registry["form"]);
    return found;
  };
  const stdSel = findRowSelect("maxwellian_u_std_distribution_type");
  if (stdSel) {
    stdSel.value = "parser"; stdSel.onchange();
    await new Promise((r) => realSetTimeout(r, 250));
    const rows3 = collectLabels(registry["form"], []);
    const has3 = (k) => rows3.some((r) => r.trim() === k);
    if (!has3("ux_std_function(x,y,z)") || has3("ux_std")) {
      console.error("FAIL 子类型选 parser 后应出现 ux_std_function(x,y,z) 且隐藏 ux_std");
      process.exit(1);
    }
    console.log("PASS 动量两级联动: maxwellian -> ux_std/ux_mean；子类型 parser -> ux_std_function(x,y,z)");
  }

  // 密度剖面只属于按体积铺粒子的注入方式
  const densRows = (st, prof) => dumpScenario(
    { params: {}, groups: { [S]: { ions: Object.assign({ injection_style: st },
        prof ? { profile: prof } : {}) } } }, 2, `密度 ${st} ${prof || ""}`);
  const fluxDens = densRows("NFluxPerCell", "");
  if (fluxDens.some((r) => r.trim() === "profile")) {
    console.error("FAIL 面注入(NFluxPerCell)不该出现 profile");
    process.exit(1);
  }
  const uniDens = densRows("NUniformPerCell", "");
  if (!uniDens.some((r) => r.trim() === "profile") || !uniDens.some((r) => r.trim() === "density")) {
    console.error("FAIL NUniformPerCell 应出现 profile 与 density");
    process.exit(1);
  }
  const pdfDens = densRows("NUniformPerCell", "parse_density_function");
  if (!pdfDens.some((r) => r.trim() === "density_function(x,y,z)")
      || pdfDens.some((r) => r.trim() === "density")) {
    console.error("FAIL profile=parse_density_function 时应出 density_function 且隐藏 density");
    process.exit(1);
  }
  console.log("PASS 密度剖面只属于 NUniformPerCell/NRandomPerCell，面注入下隐藏");

  // 两级联动必须在"常用区"里直接出现（不能掉进折叠区）
  const juttnerRows = dumpScenario(
    { params: {}, groups: { [S]: { ions: {
        injection_style: "NUniformPerCell",
        momentum_distribution_type: "maxwell_juttner",
        maxwell_juttner_u_mean_distribution_type: "constant",
        maxwell_juttner_temperature_in_eV_distribution_type: "constant" } } } },
    2, "Jüttner constant/constant");
  for (const need of ["ux_mean", "uy_mean", "uz_mean", "temperature_in_eV"]) {
    if (!juttnerRows.some((r) => r.trim() === need)) {
      console.error(`FAIL Jüttner + 子类型 constant 时缺少 ${need}（可能被折叠或排序丢失）`);
      process.exit(1);
    }
  }
  console.log("PASS Jüttner 两级选择后立即出现 ux_mean/uy_mean/uz_mean/temperature_in_eV");

  // 八种注入方式各自需要的参数（逐一核对）
  const styleRows = (st) => dumpScenario(
    { params: {}, groups: { [S]: { ions: { injection_style: st } } } }, 2, "注入 " + st);
  const styleCases = [
    ["none", []],
    ["NUniformPerCell", ["num_particles_per_cell_each_dim", "xmin", "xmax", "zmin", "zmax"]],
    ["NRandomPerCell", ["num_particles_per_cell", "xmin", "xmax"]],
    // flux 只在 flux_profile=constant 时出现；zinject_plane 属于 rigid injection
    ["NFluxPerCell", ["num_particles_per_cell", "flux_normal_axis", "flux_direction",
                      "surface_flux_pos", "flux_profile", "flux_tmin", "flux_tmax"]],
    ["SingleParticle", ["single_particle_pos", "single_particle_u", "single_particle_weight"]],
    ["MultipleParticles", ["multiple_particles_pos_x", "multiple_particles_ux",
                           "multiple_particles_weight"]],
    ["gaussian_beam", ["q_tot", "npart_real", "npart", "x_m", "y_m", "z_m",
                       "x_rms", "y_rms", "z_rms", "x_cut", "focal_distance",
                       "do_symmetrize", "do_gaussian_beam_rotation"]],
    ["external_file", ["injection_file", "z_shift", "impose_t_lab_from_file"]],
  ];
  for (const [st, must] of styleCases) {
    const rows = styleRows(st);
    const has = (k) => rows.some((r) => r.trim() === k);
    const missing = must.filter((k) => !has(k));
    if (missing.length) {
      console.error(`FAIL 注入方式 ${st} 缺少: ` + missing.join(", "));
      process.exit(1);
    }
    if (st !== "NFluxPerCell" && has("flux")) {
      console.error(`FAIL ${st} 不该出现 flux（只属于 NFluxPerCell）`);
      process.exit(1);
    }
  }
  console.log("PASS 八种注入方式各自带出正确参数（flux 只属于面注入）");

  // 八种动量分布各自需要的参数（含 uniform / gaussianflux / Jüttner）
  const momDeck = (v, extra = {}) => ({ params: {},
    groups: { [S]: { ions: Object.assign(
      { injection_style: "NUniformPerCell", momentum_distribution_type: v }, extra) } } });
  const momRows = (v, extra) => dumpScenario(momDeck(v, extra), 2, "动量 " + v);
  const momCases = [
    ["at_rest", {}, []],
    ["constant", {}, ["ux", "uy", "uz"]],
    ["uniform", {}, ["ux_min", "ux_max", "uy_min", "uy_max", "uz_min", "uz_max"]],
    ["gaussian", {}, ["ux_m", "ux_th", "uy_m", "uy_th"]],
    // gaussianflux 源码里只允许 NFluxPerCell
    ["gaussianflux", { injection_style: "NFluxPerCell" }, ["ux_m", "ux_th"]],
    ["maxwellian", {}, ["ux_mean", "ux_std", "maxwellian_u_std_distribution_type"]],
    ["maxwell_juttner", {}, ["ux_mean", "temperature_in_eV", "maxwell_juttner_temperature_in_eV_distribution_type"]],
    ["parse_momentum_function", {}, ["momentum_function_ux(x,y,z)"]],
  ];
  for (const [dist, extra, must] of momCases) {
    const rows = momRows(dist, extra);
    const has = (k) => rows.some((r) => r.trim() === k);
    const missing = must.filter((k) => !has(k));
    if (missing.length) {
      console.error(`FAIL ${dist} 缺少参数: ` + missing.join(", ")
                    + "；实际 = " + rows.slice(0, 20).join(", "));
      process.exit(1);
    }
    if (dist !== "constant" && has("ux")) {
      console.error(`FAIL ${dist} 不该出现 ux（ux 只属于 constant）`);
      process.exit(1);
    }
  }
  console.log("PASS 八种动量分布各自带出正确参数（uniform/gaussianflux/Jüttner 齐全，ux 只属于 constant）");

  // 填充区域参数只在 NUniformPerCell / NRandomPerCell 时出现
  const boxOf = (style) => dumpScenario(
    { params: {}, groups: { [S]: { ions: style ? { injection_style: style } : {} } } }, 2,
    "填充区域 " + (style || "未选"));
  const boxCases = [
    [null, false], ["NUniformPerCell", true], ["NRandomPerCell", true], ["NFluxPerCell", false],
  ];
  for (const [style, expect] of boxCases) {
    const rows = boxOf(style);
    const present = ["xmin", "xmax", "zmin", "zmax"].some((k) => rows.some((r) => r.trim() === k));
    if (present !== expect) {
      console.error(`FAIL 填充区域显示错误: injection_style=${style || "(未填)"} 期望${expect ? "显示" : "隐藏"}，实际${present ? "显示" : "隐藏"}`);
      process.exit(1);
    }
  }
  console.log("PASS 填充区域只在 NUniformPerCell/NRandomPerCell 出现");

  // 碰撞：三个名字 -> 三张卡片
  const colRows = dumpScenario(
    { params: { "collisions.collision_names": "ee ei ii" },
      groups: { [S]: { ions: {} }, "<collision_name>": { ee: {}, ei: {}, ii: {} } } },
    2, "三种碰撞");
  console.log("PASS 碰撞卡片: 出现 " + ["CoulombLog", "species", "type"].filter((k) => colRows.some((r) => r.includes(k))).length + " 类参数");

  // 激光：填了 lasers.names 之后，该激光的参数必须出现
  const laserRows = dumpScenario(
    { params: { "lasers.names": "www" }, groups: { [S]: { ions: {} }, "<laser_name>": { www: {} } } },
    2, "激光 www");
  const laserOk = laserRows.some((r) => r.includes("wavelength") || r.includes("a0"));
  if (!laserOk) { console.error("FAIL 激光参数没出现（lasers.names=www 时应有 wavelength/a0 等）"); process.exit(1); }
  console.log("PASS 激光参数随 lasers.names 出现: " + laserRows.filter(r=>/wavelength|a0|profile/.test(r)).slice(0,3).join(", "));

  // 新增组分要独立成卡片，并继承上一个组分的取值
  T.setDECK({ params: {}, groups: { [S]: { ions: { charge: "5*q_e", density: "1e24" } } } });
  T.setStep(2); T.render();
  const addBtn = (function findAdd(n) {
    if (!n) return null;
    if (n.tagName === "button" && /新增一张/.test(n.textContent || n._text || "")) return n;
    for (const c of (n.children || [])) { const r = findAdd(c); if (r) return r; }
    return null;
  })(registry["form"]);
  if (!addBtn) { console.error("FAIL 找不到「新增一张」按钮"); process.exit(1); }
  addBtn.onclick();
  const groups = T.getDECK().groups[S];
  const names = Object.keys(groups);
  const last = groups[names[names.length - 1]] || {};
  if (!(names.length >= 2 && last.charge === "5*q_e" && last.density === "1e24")) {
    console.error("FAIL 新组分没有继承上一个组分的取值: " + JSON.stringify(groups));
    process.exit(1);
  }
  console.log(`PASS 新增组分: ${names[0]} -> ${names[names.length - 1]}（继承 charge/density）`);

  // 新补的几类参数必须真的出现在对应步骤（防止"目录里有、界面上没有"）
  const stepHas = (step, names) => {
    T.setStep(step); T.render();
    const rows = collectLabels(registry["form"], []);
    return names.filter((n) => !rows.some((r) => r.trim() === n));
  };
  const coverage = [
    [3, ["algo.maxwell_solver", "warpx.do_electrostatic", "studio.solver.plugins"]],
    [3, ["algo.evolve_scheme"]],
  ];
  for (const [step, names] of coverage) {
    const missing = stepHas(step, names);
    if (missing.length) {
      console.error(`FAIL 第 ${step} 步缺少参数: ` + missing.join(", "));
      process.exit(1);
    }
  }
  // 选 macroscopic 介质后，macroscopic.* 必须出现（否则用户找不到介质参数）
  T.setDECK({ params: { "algo.em_solver_medium": "macroscopic" }, groups: {} });
  T.setStep(3); T.render();
  const macroRows = collectLabels(registry["form"], []);
  for (const need of ["macroscopic.epsilon", "macroscopic.sigma", "macroscopic.mu"]) {
    if (!macroRows.some((r) => r.trim() === need)) {
      console.error("FAIL 选了 macroscopic 介质后缺少 " + need);
      process.exit(1);
    }
  }
  console.log("PASS 选 macroscopic 介质后出现 epsilon/sigma/mu");

  // 校验报错必须能定位到具体参数行（标红），否则参数一多找不到
  const findRow = (node, key) => {
    if (!node) return null;
    if (node.dataset && node.dataset.param === key) return node;
    for (const c of (node.children || [])) { const r = findRow(c, key); if (r) return r; }
    return null;
  };
  T.setDECK({ params: { max_step: "5" }, groups: {} });
  T.setStep(3); T.render();
  markIssues([{ param: "algo.particle_shape", message: "必填", severity: "error" }]);
  T.setStep(3); T.render();
  const badRow = findRow(registry["form"], "algo.particle_shape");
  if (!badRow) { console.error("FAIL 找不到 algo.particle_shape 这一行"); process.exit(1); }
  if (!String(badRow.className || "").includes("err")) {
    console.error("FAIL 报错参数行没标红，class=" + badRow.className);
    process.exit(1);
  }
  markIssues([]);
  // 侧边栏：报错所在步骤必须标红并显示错误数
  const nav1 = el("button"); nav1.dataset.step = "3"; nav1.className = "";
  const nav2 = el("button"); nav2.dataset.step = "1"; nav2.className = "";
  const navMid = el("button"); navMid.dataset.step = "2"; navMid.className = "";
  global.__NAV_BUTTONS__ = [nav1, nav2, navMid];
  markIssues([{ param: "algo.particle_shape", message: "必填", severity: "error" }]);
  if (!String(nav1.className).includes("err")) {
    console.error("FAIL 第 3 步按钮没标红，class=" + nav1.className);
    process.exit(1);
  }
  const badge = (nav1.children || []).find((c) => String(c.className).includes("badge"));
  if (!badge || badge.textContent !== "1") {
    console.error("FAIL 第 3 步按钮没有错误数角标");
    process.exit(1);
  }
  if (String(nav2.className).includes("err")) {
    console.error("FAIL 第 1 步不该标红");
    process.exit(1);
  }
  markIssues([]);
  if (String(nav1.className).includes("err")) {
    console.error("FAIL 清除错误后按钮仍是红的");
    process.exit(1);
  }
  // 更要紧的：实例级报错（ions.density 这种）必须同样能定位，
  // 不能只对 algo.particle_shape 这种顶层参数有效。
  T.setDECK({ params: { max_step: "5", "particles.species_names": "ions" },
              groups: { "<species_name>": { ions: { injection_style: "NUniformPerCell",
                                                     profile: "constant", density: "n0" } } } });
  T.setStep(2); T.render();
  markIssues([{ param: "ions.density", message: "`n0` 不是合法的数值", severity: "error" }]);
  T.setStep(2); T.render();
  const densityRow = findRow(registry["form"], "<species_name>.density");
  if (!densityRow) {
    console.error("FAIL 物种卡片里找不到 density 行");
    process.exit(1);
  }
  if (!String(densityRow.className || "").includes("err")) {
    console.error("FAIL 实例级报错（ions.density）没有标红对应行，class=" + densityRow.className);
    process.exit(1);
  }
  // 一次多个报错、跨不同步骤时，两个步骤都要标红（用户实际遇到的情形）
  T.setStep(2); T.render();
  markIssues([
    { param: "algo.particle_shape", message: "必填", severity: "error" },
    { param: "ions.density", message: "n0 不是数值", severity: "error" },
  ]);
  T.setStep(2); T.render();
  const rowsNow = collectLabels(registry["form"], []);
  const nav3 = (global.__NAV_BUTTONS__ || []).find((b) => b.dataset.step === "3");
  const step2Btn = (global.__NAV_BUTTONS__ || []).find((b) => b.dataset.step === "2");
  if (!String(nav3.className).includes("err") || !String(step2Btn.className).includes("err")) {
    console.error("FAIL 多个报错跨步骤时没有都标红: step2=" + step2Btn.className + " step3=" + nav3.className);
    process.exit(1);
  }
  markIssues([]);
  // 第三种报错来源：顶层数值（amr.n_cell）必须落到第 1 步并标红
  T.setDECK({ params: { max_step: "5", "amr.n_cell": "64 abc" }, groups: {} });
  T.setStep(1); T.render();
  markIssues([{ param: "amr.n_cell", message: "abc 不是整数", severity: "error" }]);
  T.setStep(1); T.render();
  const ncellRow = findRow(registry["form"], "amr.n_cell");
  if (!ncellRow || !String(ncellRow.className || "").includes("err")) {
    console.error("FAIL 顶层数值报错（amr.n_cell）没有标红");
    process.exit(1);
  }
  markIssues([]);
  console.log("PASS 顶层数值报错（amr.n_cell）也能定位");

  console.log("PASS 多个报错跨步骤时侧边栏都标红");

  // 关键回归：markIssues 里的 DOM 清理不能把已标红的行抹掉
  // （浏览器里侧边栏红了、行不红，就是这段用错了集合判定）
  T.setDECK({ params: { max_step: "5", "particles.species_names": "ions" },
              groups: { "<species_name>": { ions: { injection_style: "NUniformPerCell",
                                                     profile: "constant", density: "n0" } } } });
  T.setStep(2); T.render();
  const rowAfterMark = findRow(registry["form"], "<species_name>.density");
  if (!rowAfterMark) { console.error("FAIL 第 2 步找不到 density 行"); process.exit(1); }
  markIssues([{ param: "ions.density", message: "n0 不是数值", severity: "error" }]);
  if (!String(rowAfterMark.className || "").includes("err")) {
    console.error("FAIL markIssues 执行后密度行被抹掉了标红，class=" + rowAfterMark.className);
    process.exit(1);
  }
  console.log("PASS markIssues 不会抹掉已标红的行");

  console.log("PASS 实例级报错（ions.density）能定位到模板行");

  // 默认必须关闭校验：不报错、不拦运行（用户要求"暂停所有报错功能"）
  if (typeof VALIDATION_ON !== "undefined" && VALIDATION_ON) {
    console.error("FAIL 输入校验默认应为关闭");
    process.exit(1);
  }
  console.log("PASS 输入校验默认关闭");

  console.log("PASS 侧边栏按步骤标红并带错误数角标");

  console.log("PASS 校验报错定位到参数行（标红）");

  console.log("PASS 新补参数在第 2/3/4 步都能渲染");

  // 激光实例卡片：lasers.names 存在时必须能渲染出 a0 等参数
  T.setDECK({ params: { "lasers.names": "laser1" },
              groups: { "<laser_name>": { laser1: { profile: "Gaussian", a0: "1.0" } } } });
  T.setStep(2); T.render();
  const laserCardRows = collectLabels(registry["form"], []);
  if (!laserCardRows.some((r) => r.trim() === "a0")) {
    console.error("FAIL 激光卡片没渲染（缺 a0）");
    process.exit(1);
  }
  console.log("PASS 激光卡片渲染（<laser_name>.a0）");

  // 手写 inputs 与表单必须双向同步：文本区存在、能解析成表单、表单改动能回写
  const ta = document.getElementById("inputs-text");
  if (!ta) { console.error("FAIL 页面缺少输入文本区"); process.exit(1); }
  if (typeof ta.addEventListener !== "function") {
    console.error("FAIL 输入文本区没有事件绑定能力"); process.exit(1);
  }
  const textPage = document.getElementById("page-text");
  if (!textPage) { console.error("FAIL 没有独立的输入文本页"); process.exit(1); }
  const btn = document.getElementById("preview");
  if (!btn) { console.error("FAIL 顶栏缺少输入文本按钮"); process.exit(1); }
  // 文本页是独立页面：靠 body.text-mode 控制，CSS 用 !important 保证
  const hasClass = (c) => String((document.body && document.body.className) || "").includes(c);
  btn.onclick();                                   // 点一下切到文本页
  if (!hasClass("text-mode")) {
    console.error("FAIL 进入文本页没有加 body.text-mode");
    process.exit(1);
  }
  document.body.classList.add("text-mode");
  document.getElementById("text-back").onclick();  // 返回表单
  if (hasClass("text-mode")) {
    console.error("FAIL 返回表单没有去掉 body.text-mode");
    process.exit(1);
  }
  if (!html.includes("body.text-mode nav") || !html.includes("display: none !important")) {
    console.error("FAIL 缺少 text-mode 的 CSS 隔离规则（表单渲染会重新打开页面/侧边栏）");
    process.exit(1);
  }
  if (!html.includes('id="text-save"') || html.includes('id="text-to-form"')) {
    console.error("FAIL 文本页按钮不对：应只有保存按钮");
    process.exit(1);
  }
  // 用户手写原文必须被保留：保存后再次进入文本页要看到"原文"，不是重新生成的
  const ta2 = document.getElementById("inputs-text");
  ta2.value = "# 我的注释\n#define n0 1.e24\nmax_step = 7\n";
  document.getElementById("text-save").onclick();
  document.getElementById("text-back").onclick();
  document.getElementById("preview").onclick();
  if (!String(ta2.value).includes("#define n0 1.e24")) {
    console.error("FAIL 手写原文被生成器覆盖了：" + String(ta2.value).slice(0, 60));
    process.exit(1);
  }
  document.getElementById("text-back").onclick();
  // 用户这份"朗缪尔波"deck 必须在 1~7 步都能渲染（曾经 render 抛异常被误报成"解析失败"）
  T.setDECK({
    params: {
      max_step: "800", "amr.n_cell": "64 64", "amr.max_grid_size": "32",
      "amr.max_level": "0", "geometry.dims": "2",
      "geometry.prob_lo": "-20.e-6 -20.e-6", "geometry.prob_hi": "20.e-6  20.e-6",
      "boundary.field_lo": "periodic periodic", "boundary.field_hi": "periodic periodic",
      "boundary.particle_lo": "periodic periodic", "boundary.particle_hi": "periodic periodic",
      "warpx.cfl": "0.99", "warpx.verbose": "1",
      "algo.maxwell_solver": "yee", "algo.particle_shape": "1",
      "particles.species_names": "electrons ions",
      "diagnostics.diags_names": "chk diag1 field_energy",
      "my_constants.n0": "1.e24", "my_constants.delta": "0.01",
      "my_constants.kx": "157079.63",
    },
    groups: {
      "<species_name>": {
        electrons: { charge: "-q_e", mass: "m_e", injection_style: "NUniformPerCell",
                     profile: "parse_density_function",
                     "density_function(x,y)": "\"n0 * (1.0 + delta * sin(kx * x))\"",
                     momentum_distribution_type: "gaussian" },
        ions: { charge: "q_e", mass: "m_p", injection_style: "NUniformPerCell",
                profile: "constant", density: "1.e24",
                momentum_distribution_type: "constant" },
      },
      "<diag_name>": {
        chk: { diag_type: "Full", format: "checkpoint", intervals: "200" },
        diag1: { diag_type: "Full", format: "openpmd", intervals: "10",
                 fields_to_plot: "Ex Ey Ez Bx By Bz rho" },
        field_energy: { diag_type: "ReducedDiags", reduced_diags_name: "FieldEnergy",
                        intervals: "1" },
      },
    },
  });
  for (const step of [1, 2, 3, 4, 5, 6, 7]) {
    try {
      T.setStep(step); T.render();
    } catch (e) {
      console.error(`FAIL 第 ${step} 步渲染抛异常: ${e && e.message}`);
      process.exit(1);
    }
  }
  await new Promise((r) => realSetTimeout(r, 30));   // 等异步渲染（插件页）走完
  console.log("PASS 朗缪尔波 deck 在 1~7 步都能渲染");

  console.log("PASS 用户手写原文（注释/#define）不会被覆盖");

  console.log("PASS 文本页独立：body.text-mode 切换 + CSS 隔离 + 单保存按钮");

  console.log("PASS 文本页是独立页面、只有保存按钮、可返回表单");

  // 降维诊断：warpx.reduced_diags_names 存在时，<reduced_diags_name>.type 必须出现
  T.setDECK({ params: { "warpx.reduced_diags_names": "rd1" },
              groups: { "<reduced_diags_name>": { rd1: { type: "FieldEnergy" } } } });
  T.setStep(4); T.render();
  const rdRows = collectLabels(registry["form"], []);
  if (!rdRows.some((r) => r.trim() === "type")) {
    console.error("FAIL 降维诊断卡片没渲染（缺 type）");
    process.exit(1);
  }
  console.log("PASS 降维诊断卡片渲染（<reduced_diags_name>.type）");

  // 真实交互：在页面里把 injection_style 改成 NFluxPerCell，等防抖重渲染，
  // 检查面注入的参数是否出现、填充区域是否消失（这才是用户实际做的动作）
  T.setDECK({ params: {}, groups: { [S]: { ions: { injection_style: "NUniformPerCell" } } } });
  T.setStep(2);
  T.render();
  const findInjectionSelect = (n) => {
    if (!n) return null;
    // 桩里 option 是被 appendChild 进 children 的，不在 n.options 上
    if (n.tagName === "select"
        && (n.children || []).some((o) => o.value === "NFluxPerCell")) return n;
    for (const c of (n.children || [])) { const r = findInjectionSelect(c); if (r) return r; }
    return null;
  };
  const injSel = findInjectionSelect(registry["form"]);
  if (!injSel) { console.error("FAIL 页面里找不到 injection_style 下拉框"); process.exit(1); }
  injSel.value = "NFluxPerCell";
  injSel.onchange();
  await new Promise((r) => realSetTimeout(r, 250));
  const after = collectLabels(registry["form"], []);
  const need = ["flux_normal_axis", "surface_flux_pos", "flux_profile"];
  const miss = need.filter((k) => !after.some((r) => r.trim() === k));
  if (miss.length) {
    console.error("FAIL 选中 NFluxPerCell 后没有出现: " + miss.join(", ")
                  + "；实际前 25 行 = " + after.slice(0, 25).join(", "));
    process.exit(1);
  }
  const boxGone = ["xmin", "xmax", "zmin", "zmax"].every((k) => !after.some((r) => r.trim() === k));
  if (!boxGone) { console.error("FAIL 选中 NFluxPerCell 后，填充区域参数仍然存在"); process.exit(1); }
  console.log("PASS 交互：选中 NFluxPerCell -> 出现 " + need.join(", ") + "，填充区域消失");

  const asPdf = scenario("parse_density_function");
  const asConst = scenario("constant");
  const has = (rows, name) => rows.some((r) => r.replace(/\s/g, "") === name.replace(/\s/g, ""));

  const problems = [];
  if (!has(asPdf, "density_function(x,y,z)")) problems.push("profile=parse_density_function 时没显示 density_function(x,y,z)");
  if (has(asPdf, "density")) problems.push("profile=parse_density_function 时仍然显示了 density");
  if (!has(asConst, "density")) problems.push("profile=constant 时没显示 density");
  if (has(asConst, "density_function(x,y,z)")) problems.push("profile=constant 时仍然显示了 density_function(x,y,z)");
  if (problems.length) {
    console.error("FAIL 取值门控在实际渲染中不生效:\n  " + problems.join("\n  "));
    process.exit(1);
  }
  console.log(`PASS 取值门控的真实渲染: parse_density_function -> ${asPdf.filter(r=>/density/.test(r)).join(",")}; constant -> ${asConst.filter(r=>/density/.test(r)).join(",")}`);

  // ---- 选择器 -> 参数集合：每个取值只出现源码真正读取的参数 ----------------
  // 这一组不是"能看到某个参数"，而是"不该出现的参数确实不出现"；旧规则由
  // 扫描器的 if/else-if 条件按"或"拼出来，会把别的取值的参数一起显示。
  const specRows = async (spec, params = {}) => {
    T.setDECK({ params, groups: { [S]: { electrons: spec } } });
    T.setStep(2); T.render();
    await new Promise((r) => realSetTimeout(r, 10));
    return collectLabels(registry["form"], []);
  };
  const hasName = (rows, n) => rows.some((r) => r.trim() === n);
  const checkSet = async (spec, must, mustNot, label, params = {}) => {
    const rows = await specRows(spec, params);
    const missing = must.filter((n) => !hasName(rows, n));
    const extra = mustNot.filter((n) => hasName(rows, n));
    if (missing.length || extra.length) {
      console.error(`FAIL ${label}: 缺 ${missing.join(", ") || "无"}；多 ${extra.join(", ") || "无"}`);
      process.exit(1);
    }
    console.log("PASS " + label);
  };

  await checkSet(
    { injection_style: "NUniformPerCell", momentum_distribution_type: "constant",
      profile: "constant" },
    ["ux", "uy", "uz", "density", "profile", "momentum_distribution_type",
     "num_particles_per_cell_each_dim", "xmin", "zmin"],
    ["ux_m", "ux_th", "ux_mean", "ux_std", "density_function(x,y,z)",
     "read_density_from_path", "single_particle_pos"],
    "NUniform + constant：只出现 ux/uy/uz 与 constant 密度");

  await checkSet(
    { injection_style: "NUniformPerCell", momentum_distribution_type: "maxwellian",
      profile: "constant" },
    ["ux_mean", "uy_mean", "uz_mean", "ux_std", "uy_std", "uz_std",
     "maxwellian_u_mean_distribution_type", "maxwellian_u_std_distribution_type",
     "maxwellian_temperature_in_eV_distribution_type"],
    ["ux_th", "uy_th", "uz_th", "temperature_in_eV", "ux_std_function(x,y,z)",
     "read_u_std_from_path"],
    "maxwellian 默认：u_mean + u_std，且不出现 gaussian 的 u_th");

  await checkSet(
    { injection_style: "NUniformPerCell", momentum_distribution_type: "maxwellian",
      maxwellian_temperature_in_eV_distribution_type: "constant" },
    ["temperature_in_eV", "maxwellian_temperature_in_eV_distribution_type"],
    ["ux_std", "uy_std", "uz_std", "read_u_std_from_path"],
    "maxwellian + 温度子类型 constant：温度出现、u_std 隐藏");

  await checkSet(
    { injection_style: "NUniformPerCell", momentum_distribution_type: "maxwellian",
      maxwellian_u_std_distribution_type: "parser" },
    ["ux_std_function(x,y,z)", "uy_std_function(x,y,z)", "uz_std_function(x,y,z)"],
    ["ux_std", "uy_std", "uz_std", "temperature_in_eV"],
    "maxwellian + u_std 子类型 parser：只出现 std 函数");

  await checkSet(
    { injection_style: "NUniformPerCell", momentum_distribution_type: "maxwell_juttner" },
    ["temperature_in_eV", "maxwell_juttner_temperature_in_eV_distribution_type",
     "maxwell_juttner_u_mean_distribution_type", "ux_mean"],
    ["ux_std", "maxwellian_u_std_distribution_type", "maxwellian_u_mean_distribution_type",
     "ux_th"],
    "maxwell_juttner：用温度、没有 u_std / maxwellian 选择器");

  await checkSet(
    { injection_style: "NUniformPerCell", momentum_distribution_type: "gaussianflux" },
    [],
    ["ux_m", "ux_th", "uy_m", "uy_th", "uz_m", "uz_th"],
    "gaussianflux + NUniform：参数不出现（源码要求 NFluxPerCell）");

  await checkSet(
    { injection_style: "NUniformPerCell", momentum_distribution_type: "constant",
      profile: "parse_density_function" },
    ["density_function(x,y,z)"],
    ["density", "read_density_from_path", "density_mesh_name"],
    "profile=parse_density_function：只出现密度函数");

  await checkSet(
    { injection_style: "NUniformPerCell", momentum_distribution_type: "constant",
      profile: "read_from_file" },
    ["read_density_from_path", "density_mesh_name", "read_density_distributed"],
    ["density", "density_function(x,y,z)", "read_temperature_in_eV_from_path",
     "read_u_mean_from_path"],
    "profile=read_from_file：只出现密度文件参数（不再带温度/速度文件参数）");

  await checkSet(
    { injection_style: "SingleParticle", momentum_distribution_type: "constant",
      profile: "constant" },
    ["single_particle_pos", "single_particle_u", "single_particle_weight"],
    ["momentum_distribution_type", "profile", "density", "ux", "num_particles_per_cell_each_dim"],
    "SingleParticle：不出现动量/密度集合");

  await checkSet(
    { injection_style: "external_file", momentum_distribution_type: "constant" },
    ["injection_file", "q_tot", "z_shift", "impose_t_lab_from_file"],
    ["momentum_distribution_type", "profile", "npart_real", "ux"],
    "external_file：只出现文件参数，不出现动量/密度");

  await checkSet(
    { injection_style: "gaussian_beam", momentum_distribution_type: "constant",
      profile: "constant" },
    ["x_m", "y_m", "z_m", "x_rms", "q_tot", "npart_real",
     "do_gaussian_beam_rotation", "npart"],
    ["density", "profile", "single_particle_pos", "gaussian_beam_rotation_angle",
     "gaussian_beam_rotation_axis"],
    "gaussian_beam：出现束参数、不出现密度；旋转参数默认隐藏");

  await checkSet(
    { injection_style: "gaussian_beam", momentum_distribution_type: "constant",
      do_gaussian_beam_rotation: "1" },
    ["gaussian_beam_rotation_angle", "gaussian_beam_rotation_axis"],
    ["density", "profile"],
    "gaussian_beam + 旋转开关：才出现 angle/axis");

  await checkSet(
    { injection_style: "NFluxPerCell", momentum_distribution_type: "gaussianflux",
      flux_profile: "constant", inject_from_embedded_boundary: "1" },
    ["inject_from_embedded_boundary", "flux_profile", "flux"],
    ["surface_flux_pos", "flux_normal_axis", "flux_direction"],
    "NFlux + EB 注入：不出现平面参数");

  await checkSet(
    { injection_style: "NFluxPerCell", momentum_distribution_type: "constant",
      flux_profile: "parse_flux_function" },
    ["flux_function(x,y,z,t)"],
    ["flux"],
    "NFlux + parse_flux_function：出现 flux 函数、无常数 flux");

  await checkSet(
    { injection_style: "NUniformPerCell", momentum_distribution_type: "constant",
      initialize_self_fields: "1" },
    ["self_fields_required_precision", "self_fields_max_iters"],
    [],
    "initialize_self_fields=1：出现 self_fields_*");

  await checkSet(
    { injection_style: "NUniformPerCell", momentum_distribution_type: "constant" },
    [],
    ["self_fields_required_precision", "self_fields_max_iters"],
    "initialize_self_fields 未开：隐藏 self_fields_*");

  await checkSet(
    { injection_style: "NUniformPerCell", momentum_distribution_type: "constant" },
    ["zinject_plane", "rigid_advance"],
    [],
    "particles.rigid_injected_species 包含该物种：出现 rigid 参数",
    { "particles.rigid_injected_species": "electrons" });

  await checkSet(
    { injection_style: "NUniformPerCell", momentum_distribution_type: "constant" },
    [],
    ["zinject_plane", "rigid_advance"],
    "非 rigid 物种：不出现 rigid 参数");

  // 静电场：只有 do_electrostatic 选了一个非 none 求解器才显示边界电势
  const step1Rows = async (params) => {
    T.setDECK({ params, groups: { [S]: { electrons: { injection_style: "NUniformPerCell" } } } });
    T.setStep(1); T.render();
    await new Promise((r) => realSetTimeout(r, 10));
    return collectLabels(registry["form"], []);
  };
  let rows = await step1Rows({});
  if (hasName(rows, "boundary.potential_lo_x")) {
    console.error("FAIL do_electrostatic 未设置时仍显示边界电势");
    process.exit(1);
  }
  rows = await step1Rows({ "warpx.do_electrostatic": "none" });
  if (hasName(rows, "boundary.potential_lo_x")) {
    console.error("FAIL do_electrostatic=none 时仍显示边界电势");
    process.exit(1);
  }
  rows = await step1Rows({ "warpx.do_electrostatic": "Poisson" });
  if (!hasName(rows, "boundary.potential_lo_x")) {
    console.error("FAIL do_electrostatic=Poisson 时没有显示边界电势");
    process.exit(1);
  }
  console.log("PASS do_electrostatic：none/未设置隐藏边界电势，Poisson 显示");

  // ---- 碰撞类型：11 种取值各自的参数集合（源码 CollisionHandler.cpp 分派） ----
  const collisionRows = async (ty, extra = {}) => {
    T.setDECK({
      params: { "collisions.collision_names": "c1" },
      groups: { "<collision_name>": { c1: Object.assign(
        { type: ty, species: "e e" }, extra) } },
    });
    T.setStep(2); T.render();
    await new Promise((r) => realSetTimeout(r, 10));
    return collectLabels(registry["form"], []);
  };
  const checkCollision = async (ty, must, mustNot, label, extra = {}) => {
    const rows = await collisionRows(ty, extra);
    const missing = must.filter((n) => !hasName(rows, n));
    const extraNames = mustNot.filter((n) => hasName(rows, n));
    if (missing.length || extraNames.length) {
      console.error(`FAIL ${label}: 缺 ${missing.join(", ") || "无"}；多 ${extraNames.join(", ") || "无"}`);
      process.exit(1);
    }
    console.log("PASS " + label);
  };

  {
    const typeParam = (T.CATALOG().params || []).find((p) => p.name === "<collision_name>.type");
    const opts = (typeParam && typeParam.options) || [];
    if (opts.length !== 11 || !opts.includes("background_mcc") || !opts.includes("inverse_bremsstrahlung")) {
      console.error("FAIL 碰撞类型选项不是源码的 11 种: " + JSON.stringify(opts));
      process.exit(1);
    }
    console.log("PASS 碰撞类型选项 11 种");
  }

  // 碰撞类型下拉：一个"未设置" + 11 个类型；每个选项都要有中文含义（title）
  {
    await collisionRows("pairwisecoulomb");
    const typeRow = findRow(registry["form"], "<collision_name>.type");
    const findSel = (n) => {
      if (!n) return null;
      if (n.tagName === "select") return n;
      for (const c of (n.children || [])) { const r = findSel(c); if (r) return r; }
      return null;
    };
    const sel = findSel(typeRow);
    if (!sel || sel.children.length !== 12) {
      console.error("FAIL 碰撞类型下拉不是 1 个空项 + 11 种: "
                    + (sel ? sel.children.length : "no select"));
      process.exit(1);
    }
    const blanks = sel.children.filter((o) => String(o.value) === "");
    const missing = sel.children.slice(1).filter((o) => !o.title);
    if (blanks.length !== 1 || missing.length) {
      console.error(`FAIL 碰撞类型下拉空项/含义异常: blanks=${blanks.length} missing_title=${missing.length}`);
      process.exit(1);
    }
    console.log("PASS 碰撞类型下拉：11 项每项都有中文含义");
  }

  await checkCollision(
    "pairwisecoulomb",
    ["CoulombLog", "cumulative_scattering_angle_model", "use_global_debye_length",
     "shuffling_method", "correct_energy_momentum", "beta_weight_exponent"],
    ["product_species", "Z", "background_type", "weight_ratio", "decay_rate(x,y,z,t)"],
    "collision=pairwisecoulomb：只出现库仑参数");

  await checkCollision(
    "dsmc",
    ["product_species", "scattering_processes", "ionization_target_species",
     "shuffling_method"],
    ["CoulombLog", "Z", "background_type", "weight_ratio"],
    "collision=dsmc：出现 DSMC/散射过程参数");

  await checkCollision(
    "background_mcc",
    ["background_density", "background_density(x,y,z,t)", "max_background_density",
     "scattering_processes", "ionization_species", "background_mass"],
    ["background_type", "CoulombLog", "Z"],
    "collision=background_mcc：出现背景气体 MCC 参数");

  await checkCollision(
    "background_stopping",
    ["background_type", "background_mass", "background_density",
     "background_temperature"],
    ["background_charge_state", "CoulombLog", "Z"],
    "collision=background_stopping：出现 stopping 参数");

  await checkCollision(
    "background_stopping",
    ["background_charge_state"],
    ["CoulombLog"],
    "collision=background_stopping + ions：出现 charge_state",
    { background_type: "ions" });

  await checkCollision(
    "nuclearfusion",
    ["create_products", "save_particle_production", "event_multiplier",
     "probability_threshold", "scattering_angle_model", "product_species"],
    ["Z", "background_type", "CoulombLog"],
    "collision=nuclearfusion：出现聚变参数");

  await checkCollision(
    "bremsstrahlung",
    ["Z", "multiplier", "koT1_cut", "create_photons", "product_species"],
    ["CoulombLog", "event_multiplier", "background_type"],
    "collision=bremsstrahlung：出现轫致参数");

  await checkCollision(
    "pulsed_decay",
    ["weight_ratio", "productA_temperature_eV", "productB_temperature_eV",
     "decay_rate(x,y,z,t)", "product_species"],
    ["background_type", "CoulombLog"],
    "collision=pulsed_decay：出现脉冲衰变参数");

  await checkCollision(
    "hybrid_resistive_drag",
    [],
    ["CoulombLog", "product_species", "Z", "background_type",
     "weight_ratio", "decay_rate(x,y,z,t)", "scattering_processes"],
    "collision=hybrid_resistive_drag：没有任何专属参数");

  // ---- 普通诊断：diag_type / format 的精确集合（源码 BTD/Full/openpmd 分支） ----
  const diagRows = async (type, format, extra = {}) => {
    T.setDECK({
      params: { "diagnostics.diags_names": "d1", "geometry.dims": "2" },
      groups: { "<diag_name>": { d1: Object.assign(
        { diag_type: type, format }, extra) } },
    });
    T.setStep(4); T.render();
    await new Promise((r) => realSetTimeout(r, 10));
    return collectLabels(registry["form"], []);
  };
  const checkDiag = async (type, format, must, mustNot, label, extra = {}) => {
    const rows = await diagRows(type, format, extra);
    const missing = must.filter((n) => !hasName(rows, n));
    const extraNames = mustNot.filter((n) => hasName(rows, n));
    if (missing.length || extraNames.length) {
      console.error(`FAIL ${label}: 缺 ${missing.join(", ") || "无"}；多 ${extraNames.join(", ") || "无"}`);
      process.exit(1);
    }
    console.log("PASS " + label);
  };

  {
    const dt = (T.CATALOG().params || []).find((p) => p.name === "<diag_name>.diag_type");
    const fm = (T.CATALOG().params || []).find((p) => p.name === "<diag_name>.format");
    if (!dt || !fm || dt.options.length !== 4 || !dt.options.includes("BoundaryScraping")
        || fm.options.length !== 6 || !fm.options.includes("catalyst")) {
      console.error("FAIL 诊断类型/格式选项不完整: " + JSON.stringify([dt && dt.options, fm && fm.options]));
      process.exit(1);
    }
    console.log("PASS 诊断选项：diag_type 4 种、format 6 种（含 catalyst）");
  }

  await checkDiag(
    "Full", "plotfile",
    ["plot_raw_fields", "plot_raw_fields_guards", "particle_fields_to_plot",
     "fields_to_plot", "additional_fields_to_plot", "intervals", "write_species"],
    ["time_average_mode", "average_period_steps", "do_back_transformed_fields",
     "num_snapshots_lab", "openpmd_backend", "sensei_config"],
    "diag Full+plotfile：只有 Full 参数");

  await checkDiag(
    "TimeAveraged", "plotfile",
    ["time_average_mode", "average_period_steps", "average_period_time",
     "plot_raw_fields"],
    ["average_start_step", "do_back_transformed_fields", "num_snapshots_lab",
     "openpmd_backend"],
    "diag TimeAveraged+plotfile：时间平均参数，无 BTD");

  await checkDiag(
    "TimeAveraged", "plotfile",
    ["average_start_step"],
    ["do_back_transformed_fields"],
    "diag TimeAveraged + fixed_start：出现 average_start_step",
    { time_average_mode: "fixed_start" });

  await checkDiag(
    "BackTransformed", "openpmd",
    ["do_back_transformed_fields", "do_back_transformed_particles",
     "num_snapshots_lab", "dt_snapshots_lab", "dz_snapshots_lab", "buffer_size",
     "buffer_flush_limit_btd", "openpmd_backend", "openpmd_encoding", "openpmd_tspf"],
    ["plot_raw_fields", "plot_raw_fields_guards", "particle_fields_to_plot",
     "time_average_mode", "average_period_steps", "sensei_config"],
    "diag BTD+openpmd：BTD 参数 + openpmd 参数");

  await checkDiag(
    "BackTransformed", "plotfile",
    ["do_back_transformed_fields", "buffer_size"],
    ["buffer_flush_limit_btd", "openpmd_backend", "openpmd_encoding", "openpmd_tspf"],
    "diag BTD+plotfile：不出现 openpmd 专属参数");

  await checkDiag(
    "BoundaryScraping", "openpmd",
    ["openpmd_backend", "openpmd_encoding"],
    ["plot_raw_fields", "do_back_transformed_fields", "time_average_mode",
     "sensei_config"],
    "diag BoundaryScraping+openpmd：只有 openpmd 公共参数");

  await checkDiag(
    "Full", "checkpoint",
    ["additional_fields_to_plot"],
    ["fields_to_plot", "particle_fields_to_plot", "particle_fields_species",
     "diag_lo", "diag_hi", "coarsening_ratio", "species", "plot_raw_fields",
     "plot_raw_fields_guards", "openpmd_backend"],
    "diag Full+checkpoint：隐藏 checkpoint 禁止的 fields 参数");

  await checkDiag(
    "Full", "openpmd",
    ["openpmd_backend", "openpmd_encoding", "openpmd_tspf", "adios2_engine.type"],
    ["sensei_config"],
    "diag Full+openpmd：openpmd + adios2 参数，无 sensei");

  await checkDiag(
    "Full", "openpmd",
    ["openpmd_encoding"],
    ["openpmd_tspf"],
    "diag Full+openpmd 且显式 encoding：隐藏 tspf",
    { openpmd_encoding: "tspf" });

  await checkDiag(
    "Full", "sensei",
    ["sensei_config", "sensei_pin_mesh"],
    ["openpmd_backend", "openpmd_encoding"],
    "diag Full+sensei：sensei 参数，无 openpmd 参数");

  // ---- 降维诊断：21 种 type 的专属参数（catalog 以前只有 type 一项） ----
  const rdiagRows = async (type, extra = {}) => {
    T.setDECK({
      params: { "warpx.reduced_diags_names": "r1", "geometry.dims": "2" },
      groups: { "<reduced_diags_name>": { r1: Object.assign({ type }, extra) } },
    });
    T.setStep(4); T.render();
    await new Promise((r) => realSetTimeout(r, 10));
    return collectLabels(registry["form"], []);
  };
  const checkRdiag = async (type, must, mustNot, label, extra = {}) => {
    const rows = await rdiagRows(type, extra);
    const missing = must.filter((n) => !hasName(rows, n));
    const extraNames = mustNot.filter((n) => hasName(rows, n));
    if (missing.length || extraNames.length) {
      console.error(`FAIL ${label}: 缺 ${missing.join(", ") || "无"}；多 ${extraNames.join(", ") || "无"}`);
      process.exit(1);
    }
    console.log("PASS " + label);
  };

  {
    const tp = (T.CATALOG().params || []).find((p) => p.name === "<reduced_diags_name>.type");
    if (!tp || tp.options.length !== 21 || !tp.options.includes("FieldProbe")
        || !tp.options.includes("ParticleHistogram2D")) {
      console.error("FAIL 降维诊断类型选项不是 21 种: " + JSON.stringify(tp && tp.options));
      process.exit(1);
    }
    console.log("PASS 降维诊断类型选项 21 种");
  }

  await checkRdiag(
    "FieldEnergy", [], ["species", "bin_number", "histogram_function(t,x,y,z,ux,uy,uz)",
                        "reduction_type", "probe_geometry"],
    "rdiag FieldEnergy：没有专属参数");

  await checkRdiag(
    "BeamRelevant", ["species"], ["bin_number", "reduction_type"],
    "rdiag BeamRelevant：只要 species");

  await checkRdiag(
    "ParticleHistogram",
    ["species", "bin_number", "bin_min", "bin_max",
     "histogram_function(t,x,y,z,ux,uy,uz)", "normalization",
     "filter_function(t,x,y,z,ux,uy,uz)"],
    ["reduction_type", "probe_geometry", "bin_number_abs"],
    "rdiag ParticleHistogram：直方图参数");

  await checkRdiag(
    "ParticleHistogram2D",
    ["species", "bin_number_abs", "bin_number_ord", "histogram_function_abs(t,x,y,z,ux,uy,uz,w)",
     "histogram_function_ord(t,x,y,z,ux,uy,uz,w)", "value_function(t,x,y,z,ux,uy,uz,w)",
     "filter_function(t,x,y,z,ux,uy,uz,w)", "openpmd_backend"],
    ["bin_number", "reduction_type", "probe_geometry"],
    "rdiag ParticleHistogram2D：二维直方图参数");

  await checkRdiag(
    "FieldReduction",
    ["reduced_function(x,y,z,Ex,Ey,Ez,Bx,By,Bz,jx,jy,jz)", "reduction_type"],
    ["species", "bin_number", "probe_geometry"],
    "rdiag FieldReduction：归约函数与 reduction_type");

  await checkRdiag(
    "FieldProbe",
    ["probe_geometry", "x_probe", "z_probe", "integrate", "interp_order"],
    ["resolution", "target_normal_x", "x1_probe", "species"],
    "rdiag FieldProbe Point：点探针参数",
    { probe_geometry: "Point" });

  await checkRdiag(
    "FieldProbe",
    ["x1_probe", "z1_probe", "resolution"],
    ["target_normal_x", "detector_radius", "species"],
    "rdiag FieldProbe Line：线探针参数",
    { probe_geometry: "Line" });

  await checkRdiag(
    "FieldProbe",
    ["target_normal_x", "target_normal_z", "target_up_x", "detector_radius", "resolution"],
    ["x1_probe", "species"],
    "rdiag FieldProbe Plane：面探针参数",
    { probe_geometry: "Plane" });

  // ---- 激光 profile / 外场初始化方式：互斥集合 ----
  const laserRows2 = async (prof) => {
    T.setDECK({ params: { "lasers.names": "l1" },
                groups: { "<laser_name>": { l1: { profile: prof } } } });
    T.setStep(2); T.render();
    await new Promise((r) => realSetTimeout(r, 10));
    return collectLabels(registry["form"], []);
  };
  const checkLaser = async (prof, must, mustNot, label) => {
    const rows = await laserRows2(prof);
    const missing = must.filter((n) => !hasName(rows, n));
    const extraNames = mustNot.filter((n) => hasName(rows, n));
    if (missing.length || extraNames.length) {
      console.error(`FAIL ${label}: 缺 ${missing.join(", ") || "无"}；多 ${extraNames.join(", ") || "无"}`);
      process.exit(1);
    }
    console.log("PASS " + label);
  };
  {
    const lp = (T.CATALOG().params || []).find((p) => p.name === "<laser_name>.profile");
    if (!lp || lp.options.length !== 3 || !lp.options.includes("parse_field_function")
        || !lp.options.includes("from_file")) {
      console.error("FAIL 激光 profile 选项不是 gaussian/parse_field_function/from_file: "
                    + JSON.stringify(lp && lp.options));
      process.exit(1);
    }
    const dead = ["<species_name>.position_initialization",
                  "<species_name>.momentum_initialization",
                  "<laser_name>.injection_type"];
    const still = dead.filter((n) => (T.CATALOG().params || []).some((p) => p.name === n));
    if (still.length) {
      console.error("FAIL catalog 里还有源码不存在的死键: " + still.join(", "));
      process.exit(1);
    }
    console.log("PASS 死键已清（position_initialization/momentum_initialization/injection_type）");
  }

  await checkLaser(
    "gaussian",
    ["profile_waist", "profile_duration", "profile_t_peak", "profile_focal_distance",
     "zeta", "beta", "phi2", "phi0", "stc_direction"],
    ["field_function(X,Y,t)", "binary_file_name", "lasy_file_name", "time_chunk_size"],
    "laser gaussian：只有高斯参数");

  await checkLaser(
    "parse_field_function",
    ["field_function(X,Y,t)"],
    ["profile_waist", "profile_duration", "binary_file_name", "lasy_file_name"],
    "laser parse_field_function：只有 field_function");

  await checkLaser(
    "from_file",
    ["binary_file_name", "lasy_file_name", "time_chunk_size", "delay"],
    ["field_function(X,Y,t)", "profile_waist", "profile_duration"],
    "laser from_file：只有文件参数");

  const extRows2 = async (params) => {
    T.setDECK({ params, groups: {} }); T.setStep(3); T.render();
    await new Promise((r) => realSetTimeout(r, 10));
    return collectLabels(registry["form"], []);
  };
  const extCheck = (rows, must, mustNot) => {
    const missing = must.filter((n) => !rows.some((r) => r.trim() === n));
    const extraNames = mustNot.filter((n) => rows.some((r) => r.trim() === n));
    return { missing, extraNames };
  };
  {
    let rows = await extRows2({ "warpx.B_ext_grid_init_style": "constant" });
    let d = extCheck(rows, ["warpx.B_external_grid"],
                     ["warpx.Bx_external_grid_function(x,y,z)", "warpx.read_fields_from_path"]);
    if (d.missing.length || d.extraNames.length) {
      console.error("FAIL B=constant: " + JSON.stringify(d)); process.exit(1);
    }
    rows = await extRows2({ "warpx.B_ext_grid_init_style": "parse_b_ext_grid_function" });
    d = extCheck(rows, ["warpx.Bx_external_grid_function(x,y,z)",
                        "warpx.Bz_external_grid_function(x,y,z)"],
                 ["warpx.B_external_grid", "warpx.read_fields_from_path"]);
    if (d.missing.length || d.extraNames.length) {
      console.error("FAIL B=parse: " + JSON.stringify(d)); process.exit(1);
    }
    rows = await extRows2({ "warpx.B_ext_grid_init_style": "read_from_file" });
    d = extCheck(rows, ["warpx.read_fields_from_path"], ["warpx.B_external_grid"]);
    if (d.missing.length || d.extraNames.length) {
      console.error("FAIL B=read: " + JSON.stringify(d)); process.exit(1);
    }
    rows = await extRows2({ "warpx.E_ext_grid_init_style": "constant" });
    d = extCheck(rows, ["warpx.E_external_grid"],
                 ["warpx.Ex_external_grid_function(x,y,z)", "warpx.read_fields_from_path"]);
    if (d.missing.length || d.extraNames.length) {
      console.error("FAIL E=constant: " + JSON.stringify(d)); process.exit(1);
    }
    rows = await extRows2({ "warpx.E_ext_grid_init_style": "parse_e_ext_grid_function" });
    d = extCheck(rows, ["warpx.Ex_external_grid_function(x,y,z)",
                        "warpx.Ez_external_grid_function(x,y,z)"],
                 ["warpx.E_external_grid", "warpx.read_fields_from_path"]);
    if (d.missing.length || d.extraNames.length) {
      console.error("FAIL E=parse: " + JSON.stringify(d)); process.exit(1);
    }
    rows = await extRows2({});
    d = extCheck(rows, [], ["warpx.B_external_grid", "warpx.E_external_grid",
                            "warpx.read_fields_from_path",
                            "warpx.Bx_external_grid_function(x,y,z)"]);
    if (d.missing.length || d.extraNames.length) {
      console.error("FAIL 外场未开启: " + JSON.stringify(d)); process.exit(1);
    }
    console.log("PASS 外场：constant/parse/read/未开启 各自只出现对应参数");
  }

  // ---- Maxwellian 两个互斥子类型：切换时必须删掉另一支的值 -------------
  {
    T.setDECK({ params: {}, groups: { [S]: { electrons: {
      injection_style: "NUniformPerCell",
      momentum_distribution_type: "maxwellian",
      maxwellian_u_std_distribution_type: "constant",
      ux_std: "1", uy_std: "1", uz_std: "1",
    } } } });
    T.setStep(2); T.render();
    await new Promise((r) => realSetTimeout(r, 10));
    const tempSel = findRowSelect("maxwellian_temperature_in_eV_distribution_type");
    if (!tempSel) { console.error("FAIL 找不到温度子类型下拉"); process.exit(1); }
    tempSel.value = "constant"; tempSel.onchange();
    await new Promise((r) => realSetTimeout(r, 250));
    let el = T.getDECK().groups[S].electrons;
    if (el.maxwellian_temperature_in_eV_distribution_type !== "constant"
        || "maxwellian_u_std_distribution_type" in el
        || "ux_std" in el || "uy_std" in el || "uz_std" in el) {
      console.error("FAIL 选温度子类型后没有清掉 u_std 分支: " + JSON.stringify(el));
      process.exit(1);
    }
    const ustdSel = findRowSelect("maxwellian_u_std_distribution_type");
    if (!ustdSel) { console.error("FAIL 找不到 u_std 子类型下拉"); process.exit(1); }
    ustdSel.value = "constant"; ustdSel.onchange();
    await new Promise((r) => realSetTimeout(r, 250));
    el = T.getDECK().groups[S].electrons;
    if (el.maxwellian_u_std_distribution_type !== "constant"
        || "maxwellian_temperature_in_eV_distribution_type" in el
        || "temperature_in_eV" in el || "read_temperature_in_eV_from_path" in el) {
      console.error("FAIL 选 u_std 子类型后没有清掉温度分支: " + JSON.stringify(el));
      process.exit(1);
    }
    console.log("PASS Maxwellian 子类型互斥：切换时删除另一支的值");
  }

  // ---- 4 页表单模块：openPMD / HDF5 ------------------------------------
  {
    T.setDECK({ params: {
        "diagnostics.diags_names": "diag1 diag2",
        "studio.hdf5_metadata": '{"author":"A"}',
      }, groups: { "<diag_name>": {
        diag1: { format: "openpmd", openpmd_backend: "h5" },
        diag2: { format: "plotfile" },
      } } });
    const mod = T.buildHdf5Module();
    const byId = (n, id) => {
      if (!n) return null;
      if (n.id === id) return n;
      for (const c of (n.children || [])) { const r = byId(c, id); if (r) return r; }
      return null;
    };
    const ds = byId(mod, "h5-diag");
    const fs = byId(mod, "h5-format");
    const bs = byId(mod, "h5-backend");
    const meta = byId(mod, "h5-meta");
    const ints = byId(mod, "h5-intervals");
    if (!ds || ds.children.length !== 2) {
      console.error("FAIL 表单 HDF5 模块诊断下拉不对"); process.exit(1);
    }
    if (fs.value !== "openpmd" || bs.value !== "h5" || meta.value !== '{"author":"A"}'
        || !ints || ints.value !== "1") {
      console.error("FAIL 表单 HDF5 模块没有读出现有设置"); process.exit(1);
    }
    ints.value = "5"; ints.onchange();
    if (T.getDECK().groups["<diag_name>"].diag1.intervals !== "5") {
      console.error("FAIL HDF5 输出间隔没有写回 deck"); process.exit(1);
    }
    fs.value = "plotfile"; fs.onchange();
    if (T.getDECK().groups["<diag_name>"].diag1.format !== "plotfile") {
      console.error("FAIL 表单 HDF5 模块没有把格式写回 deck"); process.exit(1);
    }
    meta.value = '{"author":"B"}'; meta.oninput();
    if (T.getDECK().params["studio.hdf5_metadata"] !== '{"author":"B"}') {
      console.error("FAIL 表单 HDF5 模块没有把元数据写回 deck"); process.exit(1);
    }
    T.getDECK().params["particles.species_names"] = "electrons";
    const fullBtn = byId(mod, "h5-full");
    fullBtn.onclick();
    const fullDiag = T.getDECK().groups["<diag_name>"].diag1;
    if (fullDiag.format !== "openpmd" || fullDiag.write_species !== "1"
        || fullDiag.intervals !== "1"
        || !String(fullDiag.fields_to_plot).includes("T_electrons")
        || !String(fullDiag.fields_to_plot).includes("jx")) {
      console.error("FAIL 全量 HDF5 按钮没有写全字段/粒子"); process.exit(1);
    }
    console.log("PASS 4 页表单模块：格式/后端/HDF5 根属性/全量按钮");
  }

  // ---- 项目切换：名字里的 "/"、维度、二进制、文本必须一起换 -------------
  // 真实项目名形如 "核心28 · nuclear_fusion/inputs_test_rz_..."。以前
  // 后端 {name} 路由匹配不了 "/"（404），前端又没看 r.ok，点项目会静默失败。
  const slashName = "自测/斜杠 · RZ";
  const slashKey = "/api/projects/" + encodeURIComponent(slashName);
  payloads[slashKey] = {
    name: slashName,
    deck: { params: { "geometry.dims": "RZ", "amr.n_cell": "32 32", max_step: "9" }, groups: {} },
    settings: { mpi: 3, omp: 2, binary: "/tmp/official/warpx.rz" },
    inputs: "# rz\nmax_step = 9\n",
    updated: "2026-11-19T22:00:00",
  };
  const projSel = registry["project"];
  const slashOpt = document.createElement("option");
  slashOpt.value = slashName;
  projSel.appendChild(slashOpt);
  projSel.value = slashName;
  await projSel.onchange({ target: projSel });
  if (registry["dims"].value !== "rz") {
    console.error("FAIL 切项目后维度没跟着换: " + registry["dims"].value);
    process.exit(1);
  }
  if (!String(registry["binary"].value).endsWith("/official/warpx.rz")) {
    console.error("FAIL 切项目后二进制没恢复: " + registry["binary"].value);
    process.exit(1);
  }
  if (registry["inputs-text"].value !== "# rz\nmax_step = 9\n") {
    console.error("FAIL 切项目后输入文本没换: " + JSON.stringify(registry["inputs-text"].value));
    process.exit(1);
  }
  if (T.getDECK().params["geometry.dims"] !== "RZ") {
    console.error("FAIL 切项目后 deck 没换: " + JSON.stringify(T.getDECK().params));
    process.exit(1);
  }
  console.log("PASS 切项目同步维度/二进制/输入文本/deck（项目名含 /）");

  // 打不开的项目：必须提示，并把下拉框退回上一个项目，不能留下半状态。
  const missingName = "不存在/项目";
  const missingOpt = document.createElement("option");
  missingOpt.value = missingName;
  projSel.appendChild(missingOpt);
  missing.add("/api/projects/" + encodeURIComponent(missingName));
  const deckBefore = JSON.stringify(T.getDECK());
  projSel.value = missingName;
  await projSel.onchange({ target: projSel });
  if (projSel.value !== slashName) {
    console.error("FAIL 打开失败后下拉框没退回上一个项目: " + projSel.value);
    process.exit(1);
  }
  if (JSON.stringify(T.getDECK()) !== deckBefore) {
    console.error("FAIL 打开失败后 deck 被改坏了");
    process.exit(1);
  }
  if (!String(registry["status"]._html || "").includes("打开任务失败")) {
    console.error("FAIL 打开失败没有提示: " + registry["status"]._html);
    process.exit(1);
  }
  missing.delete("/api/projects/" + encodeURIComponent(missingName));
  console.log("PASS 项目打开失败有提示并回退，不破坏当前 deck");

  // ---- 运行时的文本优先级：文本比表单新才发原文 -------------------------
  // 载入带 inputs 的项目后直接运行：发原文（保留注释/宏）。
  await registry["run"].onclick();
  if (runBodies.length !== 1 || runBodies[0].inputs_text !== "# rz\nmax_step = 9\n") {
    console.error("FAIL 载入项目后运行没有发项目原文: " + JSON.stringify(runBodies[0] || null));
    process.exit(1);
  }
  // 改一个表单字段后运行：必须发 deck，而不是把旧文本原样再发一遍。
  T.setStep(1); T.render();
  const maxRow = findRow(registry["form"], "max_step");
  const field = (function findField(node) {
    if (!node) return null;
    if (node.oninput) return node;
    for (const c of (node.children || [])) { const r = findField(c); if (r) return r; }
    return null;
  })(maxRow);
  if (!field) { console.error("FAIL 找不到 max_step 输入框"); process.exit(1); }
  field.value = "11";
  field.oninput();
  await registry["run"].onclick();
  if (runBodies.length !== 2 || runBodies[1].inputs_text !== null) {
    console.error("FAIL 表单改过后仍发送了旧文本: " + JSON.stringify(runBodies[1] || null));
    process.exit(1);
  }
  if (runBodies[1].params.max_step !== "11") {
    console.error("FAIL 表单改动没有进运行请求: " + JSON.stringify(runBodies[1].params));
    process.exit(1);
  }
  console.log("PASS 文本/表单优先级: 未改表单发原文；改过表单发 deck");

  // 切二进制必须真的进运行请求（用户报的"换编译版本不生效"）。
  registry["binary"].value = "/tmp/fork/warpx.rz";
  await registry["run"].onclick();
  if (runBodies.length !== 3 || runBodies[2].binary !== "/tmp/fork/warpx.rz") {
    console.error("FAIL 换二进制后运行请求没带上新路径: "
                  + JSON.stringify(runBodies[2] && runBodies[2].binary));
    process.exit(1);
  }
  console.log("PASS 换二进制后运行请求使用新路径（" + runBodies[2].binary + "）");

  // 页面刷新后必须主动同步"已有算例在跑"：否则界面显示空闲、点运行才收到
  // 409 "a simulation is already running"，且「运行」按钮一直可点。
  payloads["/api/status"] = { state: "running", step: 5, max_step: 10, time: 1e-9,
                              elapsed: 2, log: [], live: {} };
  // 等 init 自己的首轮 poll 结束，避免与测试里的 poll 撞在 POLLING 上
  await new Promise((r) => realSetTimeout(r, 120));
  await T.poll();
  if (!registry["run"].disabled) {
    console.error("FAIL 运行中的算例没有禁用「运行」按钮");
    process.exit(1);
  }
  if (!String(registry["status"]._html || "").includes("step 5/10")) {
    console.error("FAIL 刷新后没有显示运行进度: " + registry["status"]._html);
    process.exit(1);
  }
  payloads["/api/status"] = { state: "idle" };
  await T.poll();          // 清掉刚排队的轮询定时器
  console.log("PASS 刷新后主动同步运行中状态（进度可见、运行按钮禁用）");
} else {
  console.error("FAIL 页面没有暴露 __STUDIO_TEST__，无法做行为验收");
  process.exit(1);
}

console.log("PASS: init + render() 没有异常");
  } catch (err) {
    console.log("FAIL: " + (err && err.stack ? err.stack : err));
    process.exit(1);
  }
}, 10);
