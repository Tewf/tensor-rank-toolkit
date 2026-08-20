/* The six starters, and what pressing one does.

   A starter fills the three panels below it — the map, the tool, its flags —
   and then stops. It does not run anything. That is deliberate: the point of
   this console is that a command is shown before it is run, and a button that
   skipped straight to an answer would teach the opposite of what the interface
   is for. So the line appears in panel 4 and Run is one more press. */

function renderExamples() {
  const holder = $("examples");
  for (const example of state.setup.examples) {
    const button = el("button", {class: "example", type: "button"}, [
      el("span", {class: "name", text: example.title}),
      el("span", {class: "asks", text: example.why}),
    ]);
    button.onclick = () => fillFrom(example);
    holder.appendChild(button);
  }
}

async function fillFrom(example) {
  const tool = state.setup.tools.find((entry) => entry.name === example.tool);
  if (!tool) return;
  await loadFixture(example.fixture);
  $("fixture").value = example.fixture;
  chooseTool(tool);
  // After `chooseTool`, which resets the flags and puts this console's wall
  // clock into any flag that carries one. The example's own flags go on top of
  // that, so a starter never silently drops the clock a tool would otherwise
  // have been given.
  Object.assign(state.chosen, example.options);
  renderOptions();
  preview();
  $("run-panel").scrollIntoView({behavior: "smooth", block: "nearest"});
}
