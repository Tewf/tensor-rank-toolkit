/* The console's shared state, the helpers every other file here uses, and the
   wiring that starts it.

   Five plain scripts, no modules and no build step, so the page needs nothing
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

/* A command with a button that puts it on the clipboard.

   The whole claim of this interface is that the line under an answer is the line
   that produced it, and a line you have to select by hand across a wrapped `pre`
   is a line most readers retype instead, which is where a typo becomes a
   different question. `navigator.clipboard` needs a secure context, which
   localhost is and a machine reached over `--host` is not, so the old selection
   API is kept as the fallback rather than the button failing silently there. */
function wireCopy(button, block) {
  button.onclick = async () => {
    button.textContent = (await copyText(block.textContent)) ? "copied" : "select it";
    setTimeout(() => { button.textContent = "copy"; }, 1200);
  };
  return button;
}

function commandBlock(text) {
  const block = el("pre", {class: "command", text});
  const button = wireCopy(el("button", {type: "button", class: "copy", text: "copy"}),
                          block);
  return el("div", {class: "with-copy"}, [block, button]);
}

async function copyText(text) {
  try {
    await navigator.clipboard.writeText(text);
    return true;
  } catch (why) {
    const box = el("textarea", {});
    box.value = text;
    box.style.position = "fixed";
    box.style.opacity = "0";
    document.body.appendChild(box);
    box.select();
    let copied = false;
    try { copied = document.execCommand("copy"); } catch (also) { copied = false; }
    box.remove();
    return copied;
  }
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
  const machine = "build " + state.setup.build + " | runs from " + state.setup.root;
  $("facts").textContent = machine;
  // The header is one line that never wraps, so the two paths are elided when
  // they do not fit and the whole of both is on the hover.
  $("facts").title = machine;
  $("wall-clock").value = state.setup.wall_clock_seconds;

  // What the run is bounded by, from `show-limits` and not from anything this
  // page worked out: the flags on the left move a run, and these are what they
  // move from. Shown here rather than in the header because this is where it is
  // read, a moment before Run.
  const bounds = state.setup.limits || {command: "", text: ""};
  $("limits-command").textContent = bounds.command;
  $("limits-text").textContent = bounds.text;

  const menu = $("fixture");
  for (const [kind, names] of Object.entries(state.setup.fixtures)) {
    // The kind is on the group so that choosing a tool can grey out the
    // fixtures it cannot read. `sparsify-operator` takes one operator and the
    // rest take a map, and offering all of them to both is how a reader spends
    // a run finding that out.
    const group = el("optgroup", {label: kind, "data-kind": kind});
    for (const name of names) group.appendChild(el("option", {value: name, text: name}));
    menu.appendChild(group);
  }
  menu.onchange = () => loadFixture(menu.value);
  wireCopy($("copy-preview"), $("preview"));

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
  drawer("starters");

  renderTools();
  renderExamples();
  buildPanel();
  drawEarlierRuns();
}

/* A panel that is opened, read and closed, over the four panes rather than
   inside one of them. `dialog` because the backdrop, the Escape key and giving
   the focus back afterwards are the browser's job and it already does all
   three; the two buttons are the only wiring left. */
function drawer(name) {
  $("open-" + name).onclick = () => $(name).showModal();
  $("close-" + name).onclick = () => $(name).close();
}

/* Which fixtures the chosen tool can read, greyed rather than hidden so that a
   reader can see the ones it cannot and why. `kind` is the tool's own declared
   input, from the catalogue, so this is not a guess about the file. */
function offerFixturesFor(kind) {
  for (const group of $("fixture").querySelectorAll("optgroup")) {
    group.disabled = !!kind && kind !== "none" && group.dataset.kind !== kind;
  }
  $("fixture-note").textContent = !kind || kind === "none"
    ? "" : "This tool reads one " + (kind === "tensor" ? "map" : "operator") +
           ", so the other kind is greyed out above.";
}

document.addEventListener("DOMContentLoaded", begin);
