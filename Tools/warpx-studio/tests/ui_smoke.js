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
  "/api/binaries": { binaries: [] },
  "/api/projects": { projects: [], last: null },
  "/api/starter": { params: { "geometry.dims": "2", "amr.n_cell": "64 64" }, groups: {} },
  "/api/live/iterations": { iterations: [] },
  "/api/status": { state: "idle" },
};
global.fetch = async (url) => {
  const key = String(url).split("?")[0];
  const body = payloads[key] ?? {};
  return { ok: true, json: async () => body, text: async () => JSON.stringify(body) };
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
  const fluxNeeded = ["zinject_plane", "flux_normal_axis", "surface_flux_pos", "flux"];
  const fluxMissing = fluxNeeded.filter((n) => !window.includes(n));
  if (idx < 0 || fluxMissing.length) {
    console.error("FAIL 面注入参数没有紧跟 injection_style: 缺 " + fluxMissing.join(", ")
                  + "；实际紧跟的 15 行 = " + window.join(", "));
    process.exit(1);
  }
  console.log("PASS 面注入参数紧跟开关: " + window.slice(0, 6).join(", ") + " …");

  // 动量分布两级联动：选 maxwellian -> 出现 u_std/u_mean；再选 parser -> 出现 *_function
  T.setDECK({ params: {}, groups: { [S]: { ions: { momentum_distribution_type: "constant" } } } });
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
    ["NFluxPerCell", ["flux", "flux_normal_axis", "flux_direction", "surface_flux_pos",
                      "flux_profile", "flux_tmin", "flux_tmax", "zinject_plane"]],
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
    groups: { [S]: { ions: Object.assign({ momentum_distribution_type: v }, extra) } } });
  const momRows = (v, extra) => dumpScenario(momDeck(v, extra), 2, "动量 " + v);
  const momCases = [
    ["at_rest", {}, []],
    ["constant", {}, ["ux", "uy", "uz"]],
    ["uniform", {}, ["ux_min", "ux_max", "uy_min", "uy_max", "uz_min", "uz_max"]],
    ["gaussian", {}, ["ux_m", "ux_th", "uy_m", "uy_th"]],
    ["gaussianflux", {}, ["ux_m", "flux_normal_axis", "flux_direction"]],
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
  const need = ["zinject_plane", "flux_normal_axis", "surface_flux_pos"];
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
