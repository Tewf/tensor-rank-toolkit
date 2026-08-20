/* One run, as it goes and once it has ended.

   Every card leads with how the run ended, in the words `outcome.py` chose, and
   carries the command that produced it. A tool that refused says why in its own
   words, and those words are promoted to where the answer goes rather than left
   at the bottom of a stream nobody scrolls to. */

async function run() {
  $("run").disabled = true;
  try {
    const started = await ask("POST", "/api/runs", request());
    state.notes[started.id] = {field: readHeader($("map").value).field};
    const card = el("section", {class: "result", id: "run-" + started.id});
    const results = $("results");
    if (results.querySelector(".empty")) results.textContent = "";
    results.prepend(card);
    follow(started.id, card);
  } catch (why) {
    $("preview").textContent = String(why.message);
    $("preview").className = "command loose";
  }
  $("run").disabled = false;
}

async function drawEarlierRuns() {
  /* The runs belong to the server, so a page reloaded mid-search finds them
     again instead of an empty column. */
  const cards = await ask("GET", "/api/runs");
  if (!cards.length) return;
  const results = $("results");
  results.textContent = "";
  for (const card of cards) {
    const node = el("section", {class: "result", id: "run-" + card.id});
    results.appendChild(node);
    if (card.running) follow(card.id, node); else draw(node, card);
  }
}

function follow(identifier, node) {
  const tick = async () => {
    try {
      const card = await ask("GET", "/api/runs/" + identifier);
      draw(node, card);
      if (card.running) setTimeout(tick, 500);
    } catch (why) {
      // The run is still whatever it was; this only lost sight of it, and a card
      // frozen mid-run with no explanation is the one thing worse than saying so.
      node.appendChild(el("p", {class: "standing", text:
        "Lost contact with the console while this was running (" + why.message +
        "). The run itself is unaffected. Reload the page to find it again."}));
    }
  };
  tick();
}

function draw(node, card) {
  const ending = card.outcome;
  // Two classes, and they are two questions. `v-` is the ending's own name, of
  // which there are nine; `d-` is the three-way one underneath it, so that
  // proved, found and nothing-proved are three colours a reader separates
  // without reading. A screen that gives a refutation and an exhausted budget
  // the same colour has merged them whatever the sentence beneath says.
  node.className = "result" + (ending ? " v-" + ending.verdict +
                                        " d-" + ending.decides.split(" ")[0] : "");
  node.textContent = "";

  const badge = el("span", {class: "badge",
                            text: ending ? ending.badge : "running"});
  const decides = ending ? el("span", {class: "decides", text: ending.decides}) : null;
  const clock = el("span", {class: "clock",
    text: card.seconds + " s of a " + card.wall_clock_seconds + " s wall clock"});
  const header = el("header", {},
                    [el("h3", {text: card.tool}), badge, decides, clock]);
  if (card.running) {
    const stop = el("button", {type: "button", text: "stop"});
    stop.onclick = () => ask("POST", "/api/runs/" + card.id + "/stop");
    header.appendChild(stop);
  }
  node.appendChild(header);

  node.appendChild(commandBlock(card.command));
  drawPlan(node, card);
  if (ending) {
    // A tool that refused says why in its own words, and those words name the
    // line and the file. They are the answer, so they go where the answer goes
    // rather than at the bottom of a stream nobody scrolls to.
    const refusal = ending.verdict === "error" || ending.verdict === "usage"
      ? spoken(card.commentary) : "";
    // A negative status is a signal number in Python's spelling and not one of
    // the six codes, so it is not shown as though it were one. What ended the
    // run is the sentence underneath.
    const code = card.exit_status >= 0 ? "exit " + card.exit_status + ": " : "";
    node.appendChild(el("p", {class: "means", text: code + (refusal || ending.means)}));
    // Where the tool declares no reading of its own the two are the same
    // sentence, and printing it twice reads as two claims rather than one.
    if (ending.show_standing !== false && ending.standing !== ending.means) {
      node.appendChild(el("p", {class: "standing", text: ending.standing}));
    }
  }

  if (card.result.trim()) {
    node.appendChild(el("p", {class: "stream-label", text: "result (stdout)"}));
    node.appendChild(el("pre", {class: "stream", text: card.result}));
  }
  if (card.commentary.trim()) {
    node.appendChild(el("p", {class: "stream-label", text: "commentary (stderr)"}));
    node.appendChild(el("pre", {class: "stream commentary", text: card.commentary}));
  }
  drawFiles(node, card);
  node.appendChild(el("p", {class: "hint",
    text: "Kept in " + card.directory + ". The seconds above are a wall clock on " +
          "a machine that is also running a browser: MEASURING.md is the protocol " +
          "a published timing is taken under, and this is not it."}));
}

function drawPlan(node, card) {
  /* How the tool was going to answer, in its own words, above the verdict
     instead of four screens under it. `plan_lines.py` chooses which lines these
     are and copies them; every one of them is still in the stream below, so
     nothing here is the only place a reader can see it. */
  if (!card.plan || !card.plan.length) return;
  const strip = el("ul", {class: "plan"});
  for (const line of card.plan) strip.appendChild(el("li", {text: line}));
  node.appendChild(el("p", {class: "stream-label", text: "the plan it printed"}));
  node.appendChild(strip);
}

function spoken(commentary) {
  /* The commentary with its comment character taken off, which is what the tool
     would have said on a terminal. cli/report.h puts the `#` there so a run's
     stdout can be piped straight into a reader; it is not part of the sentence. */
  return commentary.split("\n").map((line) => line.replace(/^#\s?/, ""))
                   .filter((line) => line.trim()).join("\n");
}

function drawFiles(node, card) {
  if (!card.files.length) return;
  const row = el("p", {class: "files"}, [
    el("span", {class: "stream-label", text: "wrote "})]);
  for (const name of card.files) {
    row.appendChild(el("a", {href: "/api/runs/" + card.id + "/files/" + encodeURIComponent(name),
                             download: name, text: name}));
  }
  node.appendChild(row);
  const operators = card.files.filter((name) => /_[LRP]\.sms$/.test(name));
  if (operators.length === 3) node.appendChild(plinoptNote(card));
}

function plinoptNote(card) {
  const field = (state.notes[card.id] || {}).field || "<the map's p>";
  const stem = card.files.find((name) => /_L\.sms$/.test(name)).replace(/_L\.sms$/, "");
  return el("p", {class: "hint", text:
    "PLinOpt reads these as they are. Check them with " +
    "PMchecker " + stem + "_{L,R,P}.sms -q " + field + " for polynomial " +
    "multiplication, or MMchecker for matrix multiplication: handing one to the " +
    "other fails exactly as a bad operator would, and -q with no modulus makes a " +
    "correct GF(" + field + ") algorithm fail too. " +
    "formats/plinopt_interoperability/four-false-failures.md."});
}
