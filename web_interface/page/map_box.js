/* The map: where one comes from, and what the box says it holds.

   The reading under the box restates the two header lines and judges nothing.
   `core/formats/tensor_file.h` is the authority on whether a map is well formed, and
   its refusal is what a run shows, so a second opinion written here could only
   ever disagree with the one that counts. */

function readHeader(text) {
  let field = null, shape = null;
  for (const raw of text.split("\n")) {
    const line = raw.trim();
    if (!line || line.startsWith("#")) continue;
    const words = line.split(/\s+/);
    if (words[0] === "field" && words.length >= 2) field = words[1];
    if (words[0] === "shape" && words.length >= 4) shape = words.slice(1, 4);
    if (field && shape) break;
  }
  return {field, shape};
}

function showReading() {
  const text = $("map").value;
  if (!text.trim()) { $("reading").textContent = ""; return; }
  const {field, shape} = readHeader(text);
  if (field && shape) {
    $("reading").textContent =
      "Says field " + field + ", " + shape[0] + " slices of " + shape[1] +
      " by " + shape[2] + ". That is a restatement of two lines in the box and " +
      "not a check: core/formats/tensor_file.h is the authority, and its refusal is " +
      "what you will see if it disagrees.";
  } else {
    $("reading").textContent =
      "No field and shape header seen. If this is an operator that is expected, " +
      "and the reader will be chosen from its first line. If it is meant to be a " +
      "map, the tool will say what it could not read.";
  }
}

async function loadFixture(name) {
  if (!name) return;
  const {text} = await ask("GET", "/api/fixture?name=" + encodeURIComponent(name));
  $("map").value = text;
  state.fixture = name;
  showReading();
  preview();
}

function buildPanel() {
  const builder = state.setup.tools.find((tool) => tool.name === "make-tensor");
  const menu = $("builder-mode");
  for (const mode of builder.modes) {
    menu.appendChild(el("option", {value: mode.flag, text: mode.label}));
  }
  const describe = () => {
    const mode = builder.modes.find((entry) => entry.flag === menu.value);
    $("builder-fields").textContent =
      "make-tensor " + mode.flag + " expects: " + mode.fields.join(", ") + ".";
  };
  menu.onchange = describe;
  describe();

  $("builder-go").onclick = async () => {
    const mode = builder.modes.find((entry) => entry.flag === menu.value);
    const started = await ask("POST", "/api/runs", {
      tool: "make-tensor", options: {},
      mode: {flag: mode.flag, values: $("builder-values").value},
      name: "map", wall_clock_seconds: $("wall-clock").value, input: {},
    });
    const card = el("section", {class: "result"});
    const results = $("results");
    if (results.querySelector(".empty")) results.textContent = "";
    results.prepend(card);
    const built = await ask("GET", "/api/runs/" + started.id);
    draw(card, built);
    if (built.result.trim() && built.exit_status === 0) {
      $("map").value = built.result;
      state.fixture = null;
      showReading();
      preview();
    }
  };
}
