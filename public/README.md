# GBSControl webui

Redesigned UI for the GBSControl with added features:

- 72 Named Slots avaliable
- Slots save current filters state.
- Slots filter state can be toggled between local/global
- Backup / Restore system
- Option to enable / disable developer options (hidden by default)
- Integrated wifi management in system menu

## Building

```bash
npm install
npm run build
```

The build pipeline:

1. **`public/` sources** (`src/index.html.tpl`, `src/index.js`, `src/style.css`, …) are bundled into
2. **`webui.html`** at the repo root — intermediate artifact (gitignored)
3. **`webui_html.h`** at the repo root — gzipped C array embedded by the firmware sketch

Commit **`webui_html.h`** after UI changes. Do not commit `webui.html`.

Then compile and upload the GBS-Control sketch (Arduino IDE or PlatformIO).

## Tips

* Before every push do a `npm run build` to be sure bin files are updated to the latest.
