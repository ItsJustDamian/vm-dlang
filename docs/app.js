(async function () {
  const state = {
    data: null,
    activeId: null,
    query: ""
  };

  const quickLinks = document.getElementById("quickLinks");
  const featureLinks = document.getElementById("featureLinks");
  const nativeLinks = document.getElementById("nativeLinks");
  const pageTag = document.getElementById("pageTag");
  const pageTitle = document.getElementById("pageTitle");
  const pageSummary = document.getElementById("pageSummary");
  const detailHeader = document.getElementById("detailHeader");
  const detailBody = document.getElementById("detailBody");
  const referenceCards = document.getElementById("referenceCards");
  const syntaxCards = document.getElementById("syntaxCards");
  const search = document.getElementById("search");

  const response = await fetch("docs.json");
  state.data = await response.json();

  const nativeFunctions = (state.data.nativeApi ?? []).flatMap((group) =>
    group.functions.map((fn) => ({
      ...fn,
      category: group.category,
      categoryLabel: group.title,
      categorySummary: group.summary,
      type: "native"
    }))
  );

  const sections = [...state.data.quickStart, ...state.data.features, ...nativeFunctions];
  state.activeId = sections[0]?.id ?? null;

  function matchesQuery(item) {
    if (!state.query) {
      return true;
    }

    const haystack = [
      item.title,
      item.summary,
      item.details ?? "",
      item.syntax ?? "",
      item.signature ?? "",
      ...(item.params ?? []).map((param) => `${param.name} ${param.type} ${param.description}`),
      ...(item.examples ?? []).map((example) => `${example.title} ${example.code}`),
      ...(item.sections ?? []).map((section) => `${section.title} ${section.content}`)
    ].join(" ").toLowerCase();
    return haystack.includes(state.query);
  }

  function renderNav(container, items) {
    container.innerHTML = "";

    items.forEach((item) => {
      const button = document.createElement("button");
      button.type = "button";
      button.className = `nav-item ${item.id === state.activeId ? "active" : ""}`;
      button.innerHTML = `<span class="label">${item.title}</span><span class="meta">${item.summary}</span>`;
      button.addEventListener("click", () => {
        state.activeId = item.id;
        render();
      });
      container.appendChild(button);
    });
  }

  function escapeHtml(value) {
    return value
      .replaceAll("&", "&amp;")
      .replaceAll("<", "&lt;")
      .replaceAll(">", "&gt;")
      .replaceAll('"', "&quot;")
      .replaceAll("'", "&#39;");
  }

  function tokenizeCode(code) {
    const tokens = [];
    const keywords = new Set(state.data.syntax?.keywords ?? []);
    const builtins = new Set(state.data.syntax?.builtins ?? []);
    const pattern = /\/\/[^\n]*|"(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])*'|[+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?[fF]?|[A-Za-z_][A-Za-z0-9_.]*|\+\=|\-\=|\>\=|\<\=|\=\=|\!\=|[\[\]\{\}\!\%\^\&\*\(\)\-\+\=\~\|\<\>\?\/\;\,\.]/g;
    let lastIndex = 0;

    for (const match of code.matchAll(pattern)) {
      const [value] = match;
      const index = match.index ?? 0;

      if (index > lastIndex) {
        tokens.push({ type: "text", value: code.slice(lastIndex, index) });
      }

      let type = "text";

      if (value.startsWith("//")) {
        type = "comment";
      } else if (value.startsWith('"') || value.startsWith("'")) {
        type = "string";
      } else if (/^[+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?[fF]?$/.test(value)) {
        type = "number";
      } else if (keywords.has(value)) {
        type = "keyword";
      } else if (builtins.has(value)) {
        type = "builtin";
      } else if (/^[A-Za-z_][A-Za-z0-9_.]*$/.test(value)) {
        type = "identifier";
      } else {
        type = "punctuation";
      }

      tokens.push({ type, value });
      lastIndex = index + value.length;
    }

    if (lastIndex < code.length) {
      tokens.push({ type: "text", value: code.slice(lastIndex) });
    }

    return tokens;
  }

  function highlightCode(code) {
    return tokenizeCode(code)
      .map((token) => {
        if (token.type === "text") {
          return escapeHtml(token.value);
        }

        return `<span class="token token-${token.type}">${escapeHtml(token.value)}</span>`;
      })
      .join("");
  }

  function renderDetail(item) {
    pageTag.textContent = item.badge ? item.badge : item.categoryLabel ? item.categoryLabel : "Feature";
    pageTitle.textContent = item.title;
    pageSummary.textContent = item.summary;

    const headerPills = [item.id, item.categoryLabel, item.type === "native" ? "Native function" : null].filter(Boolean);
    detailHeader.innerHTML = headerPills.map((pill) => `<span class="pill">${escapeHtml(String(pill))}</span>`).join("");
    detailBody.innerHTML = "";

    if (item.type === "native") {
      const overviewSection = document.createElement("section");
      overviewSection.className = "section";
      overviewSection.innerHTML = `<h4>Description</h4><p>${escapeHtml(item.description ?? item.details ?? "")}</p>`;
      detailBody.appendChild(overviewSection);

      if (item.signature) {
        const signatureSection = document.createElement("section");
        signatureSection.className = "section";
        signatureSection.innerHTML = `<h4>Signature</h4><pre class="code-block">${highlightCode(item.signature)}</pre>`;
        detailBody.appendChild(signatureSection);
      }

      if (item.params?.length) {
        const paramsSection = document.createElement("section");
        paramsSection.className = "section";
        paramsSection.innerHTML = `<h4>Parameters</h4>`;

        const paramsGrid = document.createElement("div");
        paramsGrid.className = "param-grid";

        item.params.forEach((param) => {
          const card = document.createElement("div");
          card.className = "param-card";
          card.innerHTML = `
            <div class="param-head">
              <span class="param-name">${escapeHtml(param.name)}</span>
              <span class="param-type">${escapeHtml(param.type)}</span>
            </div>
            <p>${escapeHtml(param.description)}</p>
          `;
          paramsGrid.appendChild(card);
        });

        paramsSection.appendChild(paramsGrid);
        detailBody.appendChild(paramsSection);
      }

      if (item.returns) {
        const returnSection = document.createElement("section");
        returnSection.className = "section";
        returnSection.innerHTML = `<h4>Returns</h4><p>${escapeHtml(item.returns)}</p>`;
        detailBody.appendChild(returnSection);
      }

      if (item.examples?.length) {
        item.examples.forEach((example) => {
          const exampleSection = document.createElement("section");
          exampleSection.className = "section";
          exampleSection.innerHTML = `<h4>${escapeHtml(example.title)}</h4><pre class="code-block">${highlightCode(example.code)}</pre>`;
          detailBody.appendChild(exampleSection);
        });
      }

      if (item.notes) {
        const notesSection = document.createElement("section");
        notesSection.className = "section";
        notesSection.innerHTML = `<h4>Notes</h4><p>${escapeHtml(item.notes)}</p>`;
        detailBody.appendChild(notesSection);
      }

      return;
    }

    (item.sections ?? []).forEach((section) => {
      const wrapper = document.createElement("section");
      wrapper.className = "section";

      const title = document.createElement("h4");
      title.textContent = section.title;
      wrapper.appendChild(title);

      if (section.type === "code") {
        const pre = document.createElement("pre");
        pre.className = "code-block";
        pre.innerHTML = highlightCode(section.content);
        wrapper.appendChild(pre);
      } else {
        const paragraph = document.createElement("p");
        paragraph.textContent = section.content;
        wrapper.appendChild(paragraph);
      }

      detailBody.appendChild(wrapper);
    });

    if (item.syntax) {
      const syntaxSection = document.createElement("section");
      syntaxSection.className = "section";
      syntaxSection.innerHTML = `<h4>Syntax</h4><pre class="code-block">${highlightCode(item.syntax)}</pre><p>${escapeHtml(item.details ?? "")}</p>`;
      detailBody.appendChild(syntaxSection);
    }
  }

  function renderReference() {
    referenceCards.innerHTML = state.data.reference
      .map((entry) => `<div class="reference-card"><p class="name">${entry.name}</p><p class="value">${entry.value}</p></div>`)
      .join("");
  }

  function renderSyntaxPalette() {
    syntaxCards.innerHTML = state.data.syntax.palette
      .map((entry) => `<div class="reference-card"><p class="name"><span class="swatch" style="background:${entry.value}"></span>${entry.name}</p><p class="value">${entry.token}</p></div>`)
      .join("");
  }

  function render() {
    const visibleQuickStart = state.data.quickStart.filter(matchesQuery);
    const visibleFeatures = state.data.features.filter(matchesQuery);
    const visibleNative = nativeFunctions.filter(matchesQuery);

    renderNav(quickLinks, visibleQuickStart);
    renderNav(featureLinks, visibleFeatures);
    renderNav(nativeLinks, visibleNative);
    renderReference();
    renderSyntaxPalette();

    const activeVisible = sections.find((item) => item.id === state.activeId && matchesQuery(item));
    const active = activeVisible ?? visibleQuickStart[0] ?? visibleFeatures[0] ?? visibleNative[0] ?? sections[0];

    if (active) {
      renderDetail(active);
    } else {
      pageTag.textContent = "No results";
      pageTitle.textContent = "No results";
      pageSummary.textContent = "Try a different search term.";
      detailHeader.innerHTML = "";
      detailBody.innerHTML = "";
    }
  }

  search.addEventListener("input", () => {
    state.query = search.value.trim().toLowerCase();
    render();
  });

  render();
})();