/* Header, breadcrumb and footer from site/manifest.json, so no page repeats
   them. Each page declares its depth with <body data-base="../../">; deriving
   it from the URL breaks the moment a page moves. Content links live in the
   pages themselves, so the site still reads with JavaScript off. */

const base = document.body.dataset.base || './';
const rootUrl = new URL(base, location.href);

function currentPath() {
  const rel = location.href.slice(rootUrl.href.length);
  return decodeURIComponent(rel.split('#')[0].split('?')[0]).replace(/index\.html$/, '');
}

function el(tag, attrs = {}, ...kids) {
  const n = document.createElement(tag);
  for (const [k, v] of Object.entries(attrs)) {
    if (k === 'class') n.className = v;
    else n.setAttribute(k, v);
  }
  for (const kid of kids) n.append(kid);
  return n;
}

function trail(pages, path) {
  return pages
    .filter(p => path === p.path || path.startsWith(p.path))
    .sort((a, b) => a.path.length - b.path.length);
}

function render(manifest) {
  const path = currentPath();
  const crumbs = trail(manifest.pages, path);

  const nav = el('nav', { class: 'crumbs', 'aria-label': 'Breadcrumb' });
  crumbs.forEach((p, i) => {
    if (i > 0) nav.append(el('span', { class: 'sep' }, '/'));
    if (i === crumbs.length - 1) nav.append(el('span', { 'aria-current': 'page' }, p.short));
    else nav.append(el('a', { href: base + p.path }, p.short));
  });

  document.body.prepend(el('header', { class: 'site-head' },
    el('div', { class: 'wrap' },
      el('a', { class: 'brand', href: base }, manifest.brand),
      nav,
      el('div', { class: 'spacer' }),
      el('a', { class: 'repo', href: 'https://github.com/Tewf' }, 'GitHub')
    )
  ));

  document.body.append(el('footer', { class: 'site-foot' },
    el('div', { class: 'wrap' },
      el('p', {},
        'Every figure on this site names the file it came from. ',
        el('a', { href: manifest.repo }, 'Source for this site'),
        '.'
      )
    )
  ));
}

fetch(base + 'site/manifest.json')
  .then(r => r.json())
  .then(render)
  .catch(() => { /* no chrome beats a broken page */ });
