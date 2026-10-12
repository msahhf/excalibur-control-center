# EXCALIBUR Control Center — product website

A premium, interactive single-page product showcase for **EXCALIBUR Control
Center**, the read-only hardware telemetry app for CASPER EXCALIBUR laptops on
Linux. This is a **static, front-end-only** site: no backend, no database, no
analytics, no tracking, no cookies.

It lives entirely under `website/` and is intentionally independent from the Qt6
desktop application, the `driver/` sources and the packaging recipes.

## Stack

- **Vite 5** + **React 18** + **TypeScript** (strict)
- Hand-written CSS design system (no UI/CSS framework)
- No runtime dependencies beyond React
- `oxlint` for linting, `playwright-core` (system Chromium) for the browser test

The dependency set is deliberately small. Animations use CSS transforms/opacity
plus `IntersectionObserver`; there is no animation or WebGL library.

## Commands

```bash
npm install            # install dependencies (project-local)
npm run dev            # start the dev server
npm run typecheck      # tsc -b (strict type check)
npm run lint           # oxlint src
npm run build          # typecheck + production build → dist/
npm run preview        # serve the built dist/ locally
npm run shots          # build a real-browser pass against dist/ (see below)
```

`npm run shots` serves `dist/` over a throwaway local HTTP server and drives
**the system Chromium** via `playwright-core` (no browser download). It checks
desktop + phone viewports for horizontal overflow, image loading and console
errors, exercises the screenshot lightbox, the download tabs, the mobile menu
and `prefers-reduced-motion`, and writes reference screenshots to
`test-output/` (git-ignored). Override the Chromium path with `CHROMIUM=/path`.

> `preview`/`shots` serve the *built* output; run `npm run build` first.

## Structure

```
website/
  index.html            # SEO + Open Graph metadata, favicon, noscript fallback
  vite.config.ts
  tsconfig*.json
  public/
    favicon.svg
    images/             # REAL product screenshots (see provenance below)
  src/
    main.tsx            # entry, imports the styles in order
    App.tsx             # page composition
    data/site.ts        # product facts, real release asset names + URLs
    hooks/useReveal.ts  # reveal-on-scroll, scrolled state, reduced motion
    styles/
      tokens.css        # colour, type, space, depth, motion tokens
      base.css          # reset, typography, layout primitives
      layout.css        # nav, hero, section heads, final CTA, footer
      widgets.css       # screenshot frames, scrollytelling, diagrams, tabs, lightbox
    components/
      Nav, Hero, Scrolly, ReadOnly, Architecture, Diagnostics,
      Downloads, Compatibility, FinalCta, Footer,
      ScreenshotFrame, Lightbox, Reveal, Brand, Icons
  scripts/screenshot.mjs  # the real-browser verification pass
```

### Page flow

Cinematic hero → scrollytelling telemetry (Dashboard / Cooling / Device /
Diagnostics) → read-only philosophy + data-flow diagram → how-it-works
architecture → exportable diagnostics → downloads → compatibility → final CTA →
footer.

## Product images (provenance)

Everything under `public/images/` is a **real screenshot of the actual
application** rendering **real hardware telemetry**, captured from the v0.5.6
build of the app (1280×720, dark theme, plus one light-theme and one
no-driver/`Unavailable` shot). None of the UI is re-drawn in CSS, and no sensor
values were fabricated.

| File | Screen |
| --- | --- |
| `dashboard-dark.png` / `dashboard-light.png` | Dashboard |
| `cooling-dark.png` | Cooling |
| `device-dark.png` | Device |
| `diagnostics-dark.png` | Diagnostics |
| `settings-dark.png` | Settings |
| `about-dark.png` | About |
| `dashboard-nodriver-dark.png` | Dashboard, telemetry unavailable |

## Honesty notes

- The site shows **no live telemetry**: a browser page cannot read the hardware.
  The screenshots are static captures, and the Diagnostics JSON shown is a
  **representative schema with real field names and placeholder values** (clearly
  labelled in the UI).
- Download links and filenames are the **real** assets of the published
  **v0.5.6** release; nothing is guessed.
- Compatibility is stated exactly as verified: Fedora RPM is not published, the
  driver does not support kernel 6.18.x LTS, and the AppImage is GUI-only.
- The logo is an original mark for EXCALIBUR Control Center. The site is **not**
  affiliated with or endorsed by CASPER.

## Before hosting (not done here)

No hosting, domain or CI deployment is configured (by project decision). When a
domain is chosen, make `og:image` / `twitter:image` absolute URLs in
`index.html` and add a `<link rel="canonical">`. `vite.config.ts` uses
`base: "./"`, so the build is portable to any sub-path.
