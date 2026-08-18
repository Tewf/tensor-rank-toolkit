/* Two SVG charts, drawn by hand. No library, no build step, no CDN.
   Colours come from the CSS custom properties so the charts follow the theme. */

const NS = 'http://www.w3.org/2000/svg';

function svg(tag, attrs = {}) {
  const n = document.createElementNS(NS, tag);
  for (const [k, v] of Object.entries(attrs)) n.setAttribute(k, v);
  return n;
}

function frame(w, h) {
  return svg('svg', { viewBox: `0 0 ${w} ${h}`, role: 'img', preserveAspectRatio: 'xMidYMid meet' });
}

/**
 * Paired bars: the same quantity before and after, one row per case.
 * rows: [{ label, from, to }]
 */
export function beforeAfter(host, { rows, unit = '', format = v => String(v) }) {
  const rowH = 46, padT = 10, padB = 28, gutter = 92, right = 96, width = 640;
  const height = padT + rows.length * rowH + padB;
  const top = Math.max(...rows.map(r => Math.max(r.from, r.to)));
  const plot = width - gutter - right;
  const x = v => (v / top) * plot;

  const s = frame(width, height);
  rows.forEach((r, i) => {
    const y0 = padT + i * rowH;
    const label = svg('text', { x: gutter - 10, y: y0 + 20, 'text-anchor': 'end' });
    label.textContent = r.label;
    s.append(label);

    s.append(svg('rect', { x: gutter, y: y0 + 5, width: Math.max(1, x(r.from)), height: 12, rx: 2,
      fill: 'var(--line)' }));
    s.append(svg('rect', { x: gutter, y: y0 + 21, width: Math.max(1, x(r.to)), height: 12, rx: 2,
      class: 'bar alt' }));

    const t = svg('text', { x: gutter + Math.max(x(r.from), x(r.to)) + 10, y: y0 + 22, class: 'val' });
    t.textContent = `${format(r.from)} to ${format(r.to)}${unit}`;
    s.append(t);
  });

  const legend = svg('text', { x: gutter, y: height - 8 });
  legend.textContent = 'before (grey) and after (gold)';
  s.append(legend);

  host.replaceChildren(s);
  return s;
}

/**
 * Horizontal bars on a log scale, for quantities spanning many orders.
 * rows: [{ label, value, highlight? }]
 */
export function logBars(host, { rows, note = '' }) {
  const rowH = 26, padT = 8, padB = note ? 30 : 12, gutter = 128, right = 84, width = 640;
  const height = padT + rows.length * rowH + padB;
  const plot = width - gutter - right;
  const lo = 0, hi = Math.log10(Math.max(...rows.map(r => r.value)));
  const x = v => (Math.log10(Math.max(v, 1)) - lo) / (hi - lo) * plot;

  const s = frame(width, height);
  rows.forEach((r, i) => {
    const cy = padT + i * rowH + rowH / 2;
    const label = svg('text', { x: gutter - 10, y: cy + 4, 'text-anchor': 'end' });
    label.textContent = r.label;
    s.append(label);
    s.append(svg('rect', { x: gutter, y: cy - 7, width: Math.max(1, x(r.value)), height: 14, rx: 2,
      class: r.highlight ? 'bar alt' : 'bar' }));
    const t = svg('text', { x: gutter + x(r.value) + 8, y: cy + 4, class: r.highlight ? 'val' : '' });
    t.textContent = r.display;
    s.append(t);
  });

  if (note) {
    const t = svg('text', { x: gutter, y: height - 8 });
    t.textContent = note;
    s.append(t);
  }
  host.replaceChildren(s);
  return s;
}
