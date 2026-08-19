/* The console's shared state, the three helpers every other file here uses, and
   the wiring that starts it.

   Four plain scripts, no modules and no build step, so the page needs nothing
   the toolkit did not already need. They are loaded in any order and everything
   below runs from `DOMContentLoaded`, so no file has to be after another. */

const state = {setup: null, tool: null, chosen: {}, mode: null,
               fixture: null, notes: {}};

const $ = (id) => document.getElementById(id);

function el(tag, attributes, children) {
  const node = document.createElement(tag);
  for (const [key, value] of Object.entries(attributes || {})) {
    if (key === "text") node.textContent = value;
    else if (key === "class") node.className = value;
    else if (value === true) node.setAttribute(key, "");
    else if (value !== false && value != null) node.setAttribute(key, value);
  }
  for (const child of children || []) if (child) node.appendChild(child);
  return node;
}

async function ask(method, path, body) {
  const answer = await fetch(path, {
    method,
    headers: body ? {"Content-Type": "application/json"} : {},
    body: body ? JSON.stringify(body) : undefined,
  });
  const parsed = await answer.json();
  if (!answer.ok) throw new Error(parsed.refused || answer.statusText);
  return parsed;
}

async function begin() {
  state.setup = await ask("GET", "/api/setup");
  $("facts").textContent =
    "build " + state.setup.build + " | runs from " + state.setup.root;
  $("wall-clock").value = state.setup.wall_clock_seconds;

  const menu = $("fixture");
  for (const [kind, names] of Object.entries(state.setup.fixtures)) {
    const group = el("optgroup", {label: kind});
    for (const name of names) group.appendChild(el("option", {value: name, text: name}));
    menu.appendChild(group);
  }
  menu.onchange = () => loadFixture(menu.value);

  $("map").oninput = () => { state.fixture = null; showReading(); preview(); };
  $("wall-clock").oninput = () => {
    if (!state.tool) return;
    for (const option of state.tool.options) {
      if (option.carries_wall_clock) state.chosen[option.flag] = $("wall-clock").value;
    }
    renderOptions();
    preview();
  };
  $("upload-button").onclick = () => $("upload").click();
  $("upload").onchange = () => {
    const file = $("upload").files[0];
    if (!file) return;
    file.text().then((text) => {
      $("map").value = text;
      state.fixture = null;
      showReading();
      preview();
    });
  };
  $("build-button").onclick = () => {
    $("builder").hidden = !$("builder").hidden;
  };
  $("run").onclick = run;

  renderTools();
  buildPanel();
  drawEarlierRuns();
}

document.addEventListener("DOMContentLoaded", begin);
