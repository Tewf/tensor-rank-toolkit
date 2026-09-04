/* The question: which of the tools, with which flags, and the line that asks it.

   The preview is asked of the server rather than assembled here. That costs a
   round trip on the loopback address and buys the only thing that matters: the
   line shown before a run is produced by the same function that produces the
   line shown after it, so the two cannot drift. A preview built here would be a
   second implementation of `command_line.py`, and the first time it disagreed it
   would be a paper's method section disagreeing with the run. */

const GROUPS = [
  ["tensor", "on a map"],
  ["operator", "on one operator"],
  ["none", "on nothing but its own arguments"],
];

function renderTools() {
  const holder = $("tools");
  holder.textContent = "";
  for (const [kind, heading] of GROUPS) {
    const tools = state.setup.tools.filter(
      (tool) => tool.input === kind && !tool.produces);
    if (!tools.length) continue;
    holder.appendChild(el("p", {class: "hint", text: heading}));
    for (const tool of tools) {
      const button = el("button", {class: "tool", type: "button",
                                   "aria-pressed": String(state.tool === tool)}, [
        el("span", {class: "name", text: tool.name}),
        el("span", {class: "asks", text: tool.asks}),
      ]);
      button.onclick = () => chooseTool(tool);
      holder.appendChild(button);
    }
  }
  renderFlows(holder);
}

/* The flows, under the tools and named as what they are: more than one tool, in
   the order a pipeline needs them. Choosing one is still choosing a question;
   what it is not is a thirteenth tool, so it is grouped apart from the twelve
   and `flows.py` rather than `catalogue.py` is where it is declared. */
function renderFlows(holder) {
  const offered = state.setup.flows || [];
  if (!offered.length) return;
  holder.appendChild(el("p", {class: "hint",
    text: "on a map, one tool after another"}));
  for (const flow of offered) {
    const button = el("button", {class: "tool flow", type: "button",
                                 "aria-pressed": String(state.flow === flow)}, [
      el("span", {class: "name", text: flow.name}),
      el("span", {class: "asks", text: flow.asks}),
    ]);
    button.onclick = () => chooseFlow(flow);
    holder.appendChild(button);
  }
}

function chooseFlow(flow) {
  state.flow = flow;
  state.tool = null;
  state.chosen = {};
  renderTools();
  renderOptions();
  offerFixturesFor(flow.input);
  $("no-question").hidden = true;
  $("options-panel").hidden = false;
  $("run-panel").hidden = false;
  $("answers").textContent = flow.answers;
  preview();
}

function chooseTool(tool) {
  state.tool = tool;
  state.flow = null;
  state.chosen = {};
  for (const option of tool.options) {
    // A tool with a clock of its own is given this console's clock, in its own
    // flag, so the limit is named in the command a reader retypes instead of
    // being imposed from outside where nothing records it.
    if (option.carries_wall_clock) state.chosen[option.flag] = $("wall-clock").value;
  }
  renderTools();
  renderOptions();
  offerFixturesFor(tool.input);
  $("no-question").hidden = true;
  $("options-panel").hidden = false;
  $("run-panel").hidden = false;
  $("answers").textContent = tool.answers;
  preview();
}

function control(option) {
  const set = (value) => { state.chosen[option.flag] = value; preview(); };
  const current = state.chosen[option.flag];

  if (option.kind === "switch" || option.kind === "emit_operators" ||
      option.kind === "emit_file") {
    const box = el("input", {type: "checkbox"});
    box.checked = !!current;
    box.onchange = () => set(box.checked);
    return el("label", {}, [box, document.createTextNode(" " + option.label + " "),
                             el("span", {class: "flag", text: option.flag})]);
  }
  if (option.kind === "choice" || option.kind === "symmetry") {
    return option.kind === "symmetry" ? symmetryControl(option, set)
                                      : choiceControl(option, set, current);
  }
  const box = el("input", {size: 12, value: current == null ? "" : current,
                           placeholder: option.kind === "points" ? "1:5 2:3" : ""});
  box.oninput = () => set(box.value);
  return labelled(option, box);
}

function labelled(option, ...controls) {
  return el("label", {}, [
    document.createTextNode(option.label + " "),
    el("span", {class: "flag", text: option.flag}),
    el("br"), ...controls,
  ]);
}

function choiceControl(option, set, current) {
  const menu = el("select", {}, [el("option", {value: "", text: "(the default)"})]);
  for (const value of option.values) {
    menu.appendChild(el("option", {value, text: value,
                                   selected: current === value}));
  }
  menu.onchange = () => set(menu.value);
  return labelled(option, menu);
}

function symmetryControl(option, set) {
  // Seeded from what is already chosen rather than from nothing, so that
  // redrawing the panel does not reset a quotient somebody asked for. A worked
  // example sets `-s` and then redraws, which is exactly that case.
  const held = state.chosen[option.flag];
  const start = held && typeof held === "object" ? held : {mode: "none"};
  const shape = {n: start.n || "", m: start.m || "", k: start.k || ""};
  const boxes = ["n", "m", "k"].map((part) => {
    const box = el("input", {size: 2, placeholder: part, value: shape[part]});
    box.oninput = () => { shape[part] = box.value; push(); };
    return box;
  });
  const menu = el("select", {}, ["none", "auto", "matmul"].map(
    (mode) => el("option", {value: mode, text: mode,
                            selected: mode === start.mode})));
  const push = () => {
    for (const box of boxes) box.hidden = menu.value !== "matmul";
    set(Object.assign({mode: menu.value}, shape));
  };
  menu.onchange = push;
  push();
  return labelled(option, el("span", {class: "row"}, [menu, ...boxes]));
}

function renderOptions() {
  const holder = $("options");
  holder.textContent = "";
  if (state.flow) {
    // A flow's flags belong to the flow: they are what makes its steps fit
    // together, and a panel offering to retune one halfway through would be
    // offering a different pipeline under the same name.
    holder.appendChild(el("p", {class: "hint", text:
      "The steps are declared with the flow, so there is nothing to tune here: " +
      "each runs the flags flows.py names beside it. The wall clock below " +
      "bounds every step of it."}));
    return;
  }
  for (const option of state.tool.options) {
    holder.appendChild(control(option));
    // The note is the measurement behind the default, and it is a paragraph.
    // Eight of them at once is a wall, so each waits behind its own summary;
    // nothing is lost, only deferred to the moment a reader asks.
    if (option.note) {
      const why = el("details", {class: "why"}, [
        el("summary", {text: "why this default"}),
        el("p", {class: "hint", text: option.note}),
      ]);
      holder.appendChild(why);
    }
  }
  if (!state.tool.options.length) {
    holder.appendChild(el("p", {class: "hint",
      text: "There is nothing to tune: this tool takes no options."}));
  }
}

/* ---- the command -------------------------------------------------------- */

function request() {
  const box = {
    name: state.fixture || "map",
    wall_clock_seconds: $("wall-clock").value,
    input: {text: $("map").value, name: state.fixture || "map",
            fixture: state.fixture},
  };
  if (state.flow) return Object.assign({flow: state.flow.name}, box);
  return Object.assign({tool: state.tool.name, options: state.chosen,
                        mode: state.mode}, box);
}

let pending = null;
function preview() {
  clearTimeout(pending);
  pending = setTimeout(async () => {
    if (!state.tool && !state.flow) return;
    try {
      const seen = await ask("POST", "/api/preview", request());
      $("preview").textContent = seen.command;
      $("run").disabled = false;
      $("run-note").textContent = seen.reader
        ? "read by the " + seen.reader + " reader" : "";
      warnings(seen.warnings || []);
      whatFollows(seen.then || []);
    } catch (why) {
      $("preview").textContent = String(why.message);
      $("preview").className = "command loose";
      $("run").disabled = true;
      $("run-note").textContent = "";
      warnings([]);
      whatFollows([]);
      return;
    }
    $("preview").className = "command";
  }, 150);
}

/* What this run would leave behind, said before it is started rather than in
   `not-production-ready-yet.md` where nobody about to press Run is looking. */
function warnings(said) {
  const holder = $("preview-warnings");
  holder.textContent = "";
  holder.hidden = !said.length;
  for (const line of said) holder.appendChild(el("p", {class: "warning", text: line}));
}

/* What runs after the line above, for a flow. The steps after the first read
   files the first has not written yet, so this says the tool and what it will
   read rather than a command, and the line each step really ran arrives on that
   step's own card. A guessed path shown as a command would be the one claim
   this interface makes and cannot keep. */
function whatFollows(said) {
  const holder = $("preview-then");
  holder.textContent = "";
  holder.hidden = !said.length;
  for (const line of said) holder.appendChild(el("p", {class: "hint", text: line}));
}
