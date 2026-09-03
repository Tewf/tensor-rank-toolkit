/* The seven starters, and what pressing one does.

   A starter fills the panes behind the drawer: the map, the question, its
   flags. It then stops, and does not run anything. That is deliberate: the
   point of this console is that a command is shown before it is run, and a
   button that skipped straight to an answer would teach the opposite of what
   the interface is for. So the drawer closes onto the filled-in command, and
   Run is one more press.

   The last names a flow rather than a tool, which fills the same panes with a
   pipeline instead of one command. It stops in the same place, for the same
   reason: the first of its lines is on the screen before anything runs. */

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
  const chosen = example.flow
    ? state.setup.flows.find((entry) => entry.name === example.flow)
    : state.setup.tools.find((entry) => entry.name === example.tool);
  if (!chosen) return;
  await loadFixture(example.fixture);
  $("fixture").value = example.fixture;
  if (example.flow) {
    // A flow carries its own flags, in `flows.py`, so there is nothing for a
    // starter to put on top of them.
    chooseFlow(chosen);
  } else {
    chooseTool(chosen);
    // After `chooseTool`, which resets the flags and puts this console's wall
    // clock into any flag that carries one. The example's own flags go on top of
    // that, so a starter never silently drops the clock a tool would otherwise
    // have been given.
    Object.assign(state.chosen, example.options);
    renderOptions();
    preview();
  }
  $("starters").close();
}
