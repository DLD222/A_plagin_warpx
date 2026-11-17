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
    set innerHTML(v) { this._html = v; }, get innerHTML() { return this._html; },
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
realSetTimeout(() => {
  try {
    render();
    console.log("PASS: init + render() 没有异常");
  } catch (err) {
    console.log("FAIL: " + (err && err.stack ? err.stack : err));
    process.exit(1);
  }
}, 10);
