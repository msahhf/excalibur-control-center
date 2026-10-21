# EXCALIBUR Control Center — product website

A premium, interactive single-page product site (EN / TR / DE) for **EXCALIBUR
Control Center**, the read-only hardware telemetry app for CASPER EXCALIBUR
laptops on Linux. Static and front-end only: no backend, no database, no
analytics, no tracking, no cookies.

It lives entirely under `website/` and is independent from the Qt6 desktop app,
the `driver/` sources and the packaging recipes.

## Stack

- **Vite 5** + **React 18** + **TypeScript** (strict)
- Hand-written CSS design system (`src/styles/`)
- A typed, dependency-free i18n layer (`src/i18n/`)
- `oxlint` for linting; `playwright-core` (system Chromium) for the browser test

No runtime dependencies beyond React. Animations are CSS transforms/opacity plus
a single `requestAnimationFrame` scroll loop; there is no animation or WebGL
library.

## Commands

```bash
npm install            # install dependencies (project-local)
npm run dev            # dev server
npm run typecheck      # tsc -b (strict)
npm run lint           # oxlint src
npm run build          # typecheck + production build → dist/
npm run preview        # serve the built dist/
npm run shots          # real-browser verification against dist/ (see below)
```

`npm run shots` serves `dist/` over a throwaway HTTP server and drives the
**system Chromium** via `playwright-core` (no browser download). It checks
responsive overflow (360/390/768/1366/1920), image loading, console errors, the
synchronised horizontal scrollytelling (forward + backward, text/image/nav never
diverging), sticky behaviour, the mobile vertical fallback, reduced motion, the
read-only pipeline diagram, the downloads selector (real URLs + keyboard), the
EN/TR/DE localisation (lang, title, meta, persistence) and the footer credit,
and writes reference screenshots to `test-output/` (git-ignored). Override the
browser with `CHROMIUM=/path`.

## Structure

```
website/
  index.html              # SEO + Open Graph metadata, favicon, noscript fallback
  public/images/          # REAL product screenshots (see provenance)
  scripts/screenshot.mjs  # the real-browser verification pass
  src/
    main.tsx              # entry: I18nProvider + styles
    App.tsx               # page composition
    data/site.ts          # non-textual facts: real asset names, URLs, image paths
    i18n/                 # en.ts (source of truth) · tr.ts · de.ts · index.tsx
    hooks/useEnvironment.ts
    styles/               # tokens · base · layout · widgets
    components/
      Nav, LanguageSwitcher, Hero, Story, ReadOnly, Architecture,
      Diagnostics, Downloads, QuickInstall, Compatibility, FinalCta, Footer,
      ScreenshotFrame, Lightbox, SheetHead, Brand, Icons
```

### Page flow

Cinematic hero → a **vertical intro that becomes a horizontal scrollytelling
story** (Dashboard / Cooling / Device / Diagnostics) and returns to vertical flow
→ *Read, don't control* with a one-way telemetry pipeline diagram → how it works
→ exportable diagnostics → downloads (Quick Install one-liner + segmented
platform selector) →
compatibility → final CTA → footer.

## Localisation

Three locales — **English** (default), **Türkçe**, **Deutsch**. `src/i18n/en.ts`
defines the dictionary shape; `tr.ts` and `de.ts` are typed as that shape, so a
**missing or extra key is a compile error** (`npm run typecheck`). The switcher
lives in the navigation (and the mobile menu). The choice is stored in
`localStorage`; with no stored choice the browser's preferred language is used,
falling back to English. On change, `<html lang>`, `<title>` and the description
/ Open Graph metadata are updated. Brand names, file names, code tokens and the
footer credit keep `translate="no"`.

## Product images (provenance)

Everything under `public/images/` is a **real screenshot of the actual
application** rendering **real hardware telemetry** (v0.5.6 build, 1280×720, dark
theme plus one light and one no-driver shot). No UI is re-drawn in CSS and no
sensor values are fabricated.

## Honesty notes

- No live telemetry: a browser cannot read the hardware. Screenshots are static
  captures; the Diagnostics JSON is a **representative schema with real field
  names and placeholder values** (labelled as such in the UI).
- Download links are the **real** v0.5.8 release assets; nothing is guessed. The
  asset names, byte sizes and sha256 values in `src/data/site.ts` were taken from
  the GitHub release and verified by downloading every file and hashing it
  against the published `SHA256SUMS`.
- Compatibility is stated exactly as verified: Fedora RPM is not published, the
  driver does not support kernel 6.18.x LTS, and the AppImage is GUI-only.
- The logo is an original mark for EXCALIBUR Control Center; the site is **not**
  affiliated with or endorsed by CASPER.

## Quick Install (one-command installer)

The Download section shows the installer that the release workflow publishes as
the `install.sh` asset (`.github/workflows/release.yml` copies
`scripts/install.sh` and adds it to `SHA256SUMS`). The site shows the exact,
verified release URL and three explicit steps — download, review with `less`,
then `sudo bash install.sh` — instead of piping remote code into a root shell,
because the script itself never calls `sudo`. The supporting facts (supported
distributions, kernel 7.2 + headers, `--check`, the privilege model) come from
`scripts/install.sh` in the repository and are repeated in the card's notes.

The clipboard action has an accessible `role="status"` announcement for success
and failure; on failure the command text is selected so it can still be copied
by hand. `npm run shots` asserts the URL, the command shape, the copy result and
the failure path in a real browser.

## Hosting

The site is deployed on Vercel, which builds `website/` on every push to the
open website branch and on `main`:

- Production: <https://excalibur-control-center.vercel.app>

`index.html` therefore uses absolute URLs for `og:image`, `twitter:image`,
`og:url`, `rel="canonical"` and the apple-touch icon (`public/icon-180.png`,
rendered from `favicon.svg`). If the domain changes, update those URLs in
`index.html`. `vite.config.ts` keeps `base: "./"`, so the build still works from
any sub-path.
