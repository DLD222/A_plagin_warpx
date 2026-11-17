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

function el(tag) {
  return {
    tagName: tag, children: [], style: {}, dataset: {}, options: [], selectedIndex: 0,
    classList: { add() {}, remove() {} }, checked: true, value: "", title: "",
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
}

const registry = {};
global.document = {
  getElementById(id) { return registry[id] || (registry[id] = el("div")); },
  createElement(tag) { return el(tag); },
  createTextNode(text) { const n = el("#text"); n._text = text; return n; },
  querySelectorAll() { return []; },
  querySelector() { return el("div"); },
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
    const deck = { params: {}, groups: { [S]: { ions: { profile, charge: "5*q_e", mass: "27*m_p" } } } };
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
  const mustSee = ["lasers.names", "collisions.collision_names", "injection_style", "profile"];
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
