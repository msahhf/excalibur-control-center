// Real-browser verification for the static build, using the system Chromium via
// playwright-core (no browser download). Serves dist/ over a throwaway server
// (module scripts cannot load from file://) and checks:
//   responsive overflow (360/390/768/1366/1920), images, console errors,
//   synchronised horizontal scrollytelling (forward + backward), sticky
//   behaviour, mobile vertical fallback, reduced motion, the read-only pipeline
//   diagram, the downloads segmented selector (real URLs + keyboard), the
//   EN/TR/DE localisation (lang, title, copy, persistence) and the footer credit.
//
//   npm run build && npm run shots

import { chromium } from "playwright-core";
import { createServer } from "node:http";
import { mkdirSync, readFileSync, statSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { dirname, extname, join, normalize, resolve } from "node:path";

const __dirname = dirname(fileURLToPath(import.meta.url));
const distDir = resolve(__dirname, "../dist");
const outDir = resolve(__dirname, "../test-output");
mkdirSync(outDir, { recursive: true });

const MIME = {
  ".html": "text/html; charset=utf-8",
  ".js": "text/javascript; charset=utf-8",
  ".css": "text/css; charset=utf-8",
  ".svg": "image/svg+xml",
  ".png": "image/png",
  ".json": "application/json",
};

function startServer() {
  const server = createServer((req, res) => {
    let p = decodeURIComponent((req.url ?? "/").split("?")[0]);
    if (p === "/") p = "/index.html";
    const file = normalize(join(distDir, p));
    if (!file.startsWith(distDir) || !statSync(file, { throwIfNoEntry: false })?.isFile()) {
      res.statusCode = 404;
      res.end("not found");
      return;
    }
    res.setHeader("content-type", MIME[extname(file)] ?? "application/octet-stream");
    res.end(readFileSync(file));
  });
  return new Promise((ok) => server.listen(0, "127.0.0.1", () => ok(server)));
}

let failures = 0;
const fail = (m) => {
  failures++;
  console.error("FAIL:", m);
};
const ok = (m) => console.log("ok  :", m);
const eq = (got, want, label) => (got === want ? ok(`${label} = ${want}`) : fail(`${label}: got ${got}, want ${want}`));

const SCENES = ["dashboard", "cooling", "device", "diagnostics"];
const SCENE_IMG = {
  dashboard: "dashboard-dark.png",
  cooling: "cooling-dark.png",
  device: "device-dark.png",
  diagnostics: "diagnostics-dark.png",
};

async function disableSmooth(page) {
  await page.evaluate(() => {
    document.documentElement.style.scrollBehavior = "auto";
  });
}

async function waitForImages(page, timeout = 15000) {
  await page
    .waitForFunction(
      () =>
        [...document.images]
          .filter((i) => i.getClientRects().length > 0)
          .every((i) => i.complete && i.naturalWidth > 0),
      null,
      { timeout },
    )
    .catch(() => {});
}

async function auditOverflow(page, label) {
  const overflow = await page.evaluate(() => {
    const cw = document.documentElement.clientWidth;
    const clipped = (el) => {
      let p = el.parentElement;
      while (p && p !== document.documentElement) {
        if (/hidden|auto|clip|scroll/.test(getComputedStyle(p).overflowX)) return true;
        p = p.parentElement;
      }
      return false;
    };
    let worst = 0;
    for (const el of document.querySelectorAll("body *")) {
      const r = el.getBoundingClientRect();
      if (r.width > 0 && r.right > cw + 1 && !clipped(el)) worst = Math.max(worst, r.right - cw);
    }
    return Math.round(worst);
  });
  if (overflow > 1) fail(`${label}: content overflows horizontally by ${overflow}px`);
  else ok(`${label}: no horizontal overflow`);
}

async function scrollAll(page) {
  await page.evaluate(async () => {
    for (let y = 0; y < document.body.scrollHeight; y += window.innerHeight * 0.6) {
      window.scrollTo(0, y);
      await new Promise((r) => setTimeout(r, 110));
    }
    window.scrollTo(0, 0);
  });
  await page.waitForTimeout(400);
}

const centeredScene = (page) =>
  page.evaluate(() => {
    const mid = window.innerWidth / 2;
    const scenes = [...document.querySelectorAll(".scene")];
    let best = -1;
    let bd = Infinity;
    scenes.forEach((el, i) => {
      const r = el.getBoundingClientRect();
      const d = Math.abs(r.left + r.width / 2 - mid);
      if (d < bd) {
        bd = d;
        best = i;
      }
    });
    return best;
  });

const navActive = (page) =>
  page.evaluate(() =>
    [...document.querySelectorAll(".story__dot")].findIndex((b) => b.getAttribute("aria-current") === "true"),
  );

async function scrollToScene(page, i) {
  const y = await page.evaluate((idx) => {
    const track = document.querySelector(".story__track");
    const stickyH = window.innerHeight - 68;
    const total = track.offsetHeight - stickyH;
    const absTop = window.scrollY + track.getBoundingClientRect().top;
    return absTop + (idx / 3) * total;
  }, i);
  await page.evaluate((yy) => window.scrollTo(0, yy), y);
  await page.waitForTimeout(450);
}

const server = await startServer();
const { port } = server.address();
const url = `http://127.0.0.1:${port}/`;
console.log("serving", distDir, "at", url);

const browser = await chromium.launch({
  executablePath: process.env.CHROMIUM ?? "/usr/bin/chromium",
  args: ["--no-sandbox", "--disable-dev-shm-usage", "--force-color-profile=srgb"],
});

try {
  // ===================== Desktop =====================
  {
    const ctx = await browser.newContext({ viewport: { width: 1440, height: 900 }, locale: "en-US" });
    const page = await ctx.newPage();
    const errors = [];
    page.on("console", (m) => m.type() === "error" && errors.push(m.text()));
    page.on("pageerror", (e) => errors.push("pageerror: " + e.message));
    await page.goto(url, { waitUntil: "load" });
    await disableSmooth(page);
    await page.waitForTimeout(500);

    // metadata + hero
    if (/EXCALIBUR/.test(await page.title())) ok("desktop: title present");
    else fail("desktop: title missing");
    const heroSrc = await page.getAttribute(".hero__shot img", "src");
    if (heroSrc?.includes("dashboard-dark.png")) ok("desktop: hero uses the real dashboard screenshot");
    else fail(`desktop: hero src = ${heroSrc}`);

    // lightbox
    await page.locator(".hero__shot .shot").click();
    await page.waitForSelector(".lb", { state: "visible", timeout: 3000 });
    await page.screenshot({ path: `${outDir}/lightbox.png` });
    await page.keyboard.press("Escape");
    await page.waitForSelector(".lb", { state: "detached", timeout: 3000 });
    ok("desktop: lightbox opens and closes with Escape");

    await scrollAll(page);
    await waitForImages(page);
    await auditOverflow(page, "desktop");

    const enhanced = await page.getAttribute(".story", "data-enhanced");
    eq(enhanced, "true", "desktop: horizontal story enhanced");

    // Reference shots for each scene (full-page capture cannot show a sticky rail).
    for (let i = 0; i < 4; i++) {
      await scrollToScene(page, i);
      await page.screenshot({ path: `${outDir}/story-${i}.png` });
    }

    // Scrollytelling sync — forward then backward
    for (const order of [[0, 1, 2, 3], [3, 2, 1, 0]]) {
      for (const i of order) {
        await scrollToScene(page, i);
        const centered = await centeredScene(page);
        const nav = await navActive(page);
        const img = await page.evaluate((idx) => {
          const el = document.querySelectorAll(".scene")[idx];
          return el?.querySelector(".scene__media img")?.getAttribute("src");
        }, centered);
        const expected = SCENE_IMG[SCENES[centered]];
        if (centered !== i) fail(`desktop: forward/back at scene ${i} → centered ${centered}`);
        else if (nav !== i) fail(`desktop: at scene ${i} nav active ${nav} (text/image desync)`);
        else if (!img?.includes(expected)) fail(`desktop: at scene ${i} image ${img} != ${expected}`);
        else ok(`desktop: scene ${i} — text, image and nav in sync`);
      }
    }

    // Sticky must be pinned (no jump) in the middle of the story
    await scrollToScene(page, 1);
    const stickyTop = await page.evaluate(() => document.querySelector(".story__sticky").getBoundingClientRect().top);
    if (stickyTop > 64 && stickyTop < 72) ok(`desktop: sticky pinned at ${Math.round(stickyTop)}px (no jump)`);
    else fail(`desktop: sticky top ${Math.round(stickyTop)}px not pinned`);

    await page.evaluate(() => window.scrollTo(0, 0));
    await page.waitForTimeout(300);
    await page.screenshot({ path: `${outDir}/desktop-hero.png` });
    for (const id of ["readonly", "architecture", "diagnostics", "compatibility"]) {
      await page.evaluate((sel) => document.querySelector(sel)?.scrollIntoView({ block: "start" }), `#${id}`);
      await page.waitForTimeout(320);
      await page.screenshot({ path: `${outDir}/section-${id}.png` });
    }
    await page.evaluate(() => window.scrollTo(0, 0));
    await page.screenshot({ path: `${outDir}/desktop-full.png`, fullPage: true });

    // ---- read-only ----
    const hasPipeline = await page.locator(".pipeline").count();
    if (hasPipeline === 1) ok("read-only: positive pipeline diagram present");
    else fail("read-only: pipeline diagram missing");
    const redX = await page.evaluate(() =>
      [...document.querySelectorAll(".pipeline *")].some((el) => {
        const s = getComputedStyle(el);
        return s.stroke === "rgb(251, 113, 133)" || s.fill === "rgb(251, 113, 133)";
      }),
    );
    if (!redX) ok("read-only: old red-X control path removed");
    else fail("read-only: a red-X style element is still present");

    // ---- downloads ----
    await page.evaluate(() => document.querySelector("#download")?.scrollIntoView({ block: "start" }));
    await waitForImages(page);
    await page.screenshot({ path: `${outDir}/desktop-downloads.png` });
    const picker = page.getByRole("tablist", { name: /platform|Plattform|Platform/i });
    if (await picker.count()) ok("downloads: platform selector has tablist semantics");
    else fail("downloads: missing tablist");

    const pkgCheck = async (id, expect) => {
      await page.getByRole("tab", { name: id, exact: true }).click();
      await page.waitForTimeout(120);
      const hrefs = await page.evaluate(() =>
        [...document.querySelectorAll('[role="tabpanel"] a[href^="http"]')].map((a) => a.getAttribute("href")),
      );
      const hit = hrefs.some((h) => h.includes(expect));
      if (hit) ok(`downloads: ${id} links to ${expect}`);
      else fail(`downloads: ${id} missing link containing ${expect}`);
    };
    await pkgCheck("Arch", "excalibur-control-center-0.5.6-1-x86_64.pkg.tar.zst");
    await pkgCheck("Debian", ".deb13_amd64.deb");
    await pkgCheck("Ubuntu", ".ubuntu24.04_amd64.deb");
    await pkgCheck("AppImage", "EXCALIBUR_Control_Center-x86_64.AppImage");

    const panelText = await page.locator('[role="tabpanel"]').innerText();
    if (/does not install/i.test(panelText)) ok("downloads: AppImage driver warning present");
    else fail("downloads: AppImage driver warning missing");
    await page.getByRole("tab", { name: "Debian", exact: true }).click();
    await page.waitForTimeout(120);
    const debianText = await page.locator('[role="tabpanel"]').innerText();
    if (/not interchangeable/i.test(debianText)) ok("downloads: Debian vs Ubuntu distinction present");
    else fail("downloads: Debian note missing");

    // keyboard selection
    await page.getByRole("tab", { name: "Arch", exact: true }).click();
    await page.waitForTimeout(100);
    await page.keyboard.press("ArrowRight");
    await page.waitForTimeout(150);
    const selName = await page.evaluate(() =>
      document.querySelector('[role="tab"][aria-selected="true"]')?.textContent?.trim(),
    );
    eq(selName, "Debian", "downloads: ArrowRight selects next platform");

    // ---- localisation ----
    const checkLocale = async (label, lang, navText, diagTag, navLandmark, metaLabel) => {
      await page.getByRole("button", { name: label, exact: true }).click();
      await page.waitForTimeout(200);
      eq(await page.evaluate(() => document.documentElement.lang), lang, `${lang}: <html lang>`);
      const nav = await page.evaluate(
        () => document.querySelector(".nav__links .nav__link")?.textContent?.trim(),
      );
      eq(nav, navText, `${lang}: first nav label`);
      eq(
        await page.getAttribute(".nav__links", "aria-label"),
        navLandmark,
        `${lang}: nav landmark aria-label localised`,
      );
      eq(
        await page.getAttribute(".hero__meta", "aria-label"),
        metaLabel,
        `${lang}: hero meta aria-label localised`,
      );
      const credit = await page.locator(".credit").innerText();
      ok(`${lang}: credit = "${credit.trim()}"`);
      if (credit.trim() === "Made by Muhammedşah Fidan") ok(`${lang}: credit text exact`);
      else fail(`${lang}: credit text is "${credit.trim()}"`);
      if (diagTag) {
        const diagram = await page.locator(".pipeline").evaluate((el) => el.textContent ?? "");
        if (diagram.includes(diagTag)) ok(`${lang}: diagram localised (${diagTag})`);
        else fail(`${lang}: diagram not localised`);
      }
    };
    await checkLocale("Türkçe", "tr", "Telemetri", "SALT OKUNUR SINIR", "Ana gezinme", "Kısaca");
    await checkLocale("Deutsch", "de", "Telemetrie", "NUR-LESEN-GRENZE", "Hauptnavigation", "Auf einen Blick");

    // title/description follow the locale
    const deTitle = await page.title();
    if (/EXCALIBUR/.test(deTitle)) ok(`de: title = "${deTitle.slice(0, 40)}…"`);
    else fail("de: title not updated");
    const deDesc = await page.getAttribute('meta[name="description"]', "content");
    if (deDesc && /Nur-Lesen/i.test(deDesc)) ok("de: meta description localised");
    else fail("de: meta description not localised");

    // persistence across reload
    await page.reload({ waitUntil: "load" });
    await page.waitForTimeout(400);
    eq(await page.evaluate(() => document.documentElement.lang), "de", "de: locale persists after reload");
    await page.getByRole("button", { name: "English", exact: true }).click();
    await page.waitForTimeout(200);

    // footer credit link
    const creditHref = await page.getAttribute(".credit a", "href");
    eq(creditHref, "https://github.com/msahhf", "footer: credit link target");
    const creditIsLast = await page.evaluate(() => {
      const credit = document.querySelector(".credit");
      return !!credit && credit.parentElement?.lastElementChild === credit;
    });
    if (creditIsLast) ok("footer: credit is the last content line");
    else fail("footer: credit is not last");
    await page.locator(".credit a").focus();
    const focused = await page.evaluate(() => document.activeElement?.textContent?.trim());
    eq(focused, "Muhammedşah Fidan", "footer: credit link is keyboard focusable");
    await page.evaluate(() => window.scrollTo(0, document.documentElement.scrollHeight));
    await page.waitForTimeout(400);
    await page.screenshot({ path: `${outDir}/footer.png` });

    if (errors.length) fail(`desktop: console errors → ${errors.join(" | ")}`);
    else ok("desktop: no console errors");
    await ctx.close();
  }

  // ===================== Responsive =====================
  for (const w of [360, 390, 768, 1366, 1920]) {
    const ctx = await browser.newContext({ viewport: { width: w, height: 900 }, locale: "en-US" });
    const page = await ctx.newPage();
    const errors = [];
    page.on("pageerror", (e) => errors.push(e.message));
    await page.goto(url, { waitUntil: "load" });
    await disableSmooth(page);
    await scrollAll(page);
    await waitForImages(page);
    await auditOverflow(page, `${w}px`);
    if (errors.length) fail(`${w}px: page errors → ${errors.join(" | ")}`);
    if (w === 390) await page.screenshot({ path: `${outDir}/responsive-390.png` });
    if (w === 1920) await page.screenshot({ path: `${outDir}/responsive-1920.png` });
    await ctx.close();
  }

  // ===================== Mobile story fallback =====================
  {
    const ctx = await browser.newContext({ viewport: { width: 390, height: 844 }, isMobile: true, hasTouch: true, deviceScaleFactor: 2, locale: "en-US" });
    const page = await ctx.newPage();
    await page.goto(url, { waitUntil: "load" });
    await disableSmooth(page);
    await page.waitForTimeout(400);

    eq(await page.getAttribute(".story", "data-enhanced"), "false", "mobile: story not enhanced (vertical)");
    const stickyPos = await page.evaluate(() => getComputedStyle(document.querySelector(".story__sticky")).position);
    eq(stickyPos, "static", "mobile: no sticky height consumed");
    const inlineImgs = await page.locator(".scene .scene__media img").count();
    eq(inlineImgs, 4, "mobile: four inline scene images");

    await page.locator(".nav__burger").click();
    await page.waitForTimeout(500);
    if (await page.locator("#mobile-menu a").first().isVisible()) ok("mobile: menu opens");
    else fail("mobile: menu did not open");
    const deBtn = page.locator("#mobile-menu .lang__btn", { hasText: "DE" });
    if (await deBtn.count()) ok("mobile: language switcher in menu");
    else fail("mobile: language switcher not in menu");
    await deBtn.click();
    await page.waitForTimeout(250);
    eq(await page.evaluate(() => document.documentElement.lang), "de", "mobile: language switched to German");
    await page.locator("#mobile-menu a").first().click();
    await page.waitForTimeout(500);
    const panelH = await page.evaluate(() => document.querySelector(".nav__panel").getBoundingClientRect().height);
    if (panelH < 8) ok("mobile: menu collapses fully (no leaked padding)");
    else fail(`mobile: menu still ${Math.round(panelH)}px tall`);

    // German must not overflow on a phone
    await scrollAll(page);
    await waitForImages(page);
    await auditOverflow(page, "mobile (de)");
    await page.screenshot({ path: `${outDir}/mobile-de-full.png`, fullPage: true });
    await page.evaluate(() => document.querySelector("#telemetry")?.scrollIntoView({ block: "start" }));
    await page.waitForTimeout(350);
    await page.screenshot({ path: `${outDir}/mobile-de-story.png` });
    await page.evaluate(() => document.querySelector("#download")?.scrollIntoView({ block: "start" }));
    await page.waitForTimeout(350);
    await page.screenshot({ path: `${outDir}/mobile-de-downloads.png` });
    await ctx.close();
  }

  // ===================== Reduced motion =====================
  {
    const ctx = await browser.newContext({ viewport: { width: 1440, height: 900 }, reducedMotion: "reduce", locale: "en-US" });
    const page = await ctx.newPage();
    await page.goto(url, { waitUntil: "load" });
    await page.waitForTimeout(400);
    eq(await page.getAttribute(".story", "data-enhanced"), "false", "reduced-motion: story not enhanced");
    const titles = await page.locator(".scene__title").count();
    eq(titles, 4, "reduced-motion: all scene copy present");
    const visible = await page.evaluate(() =>
      [...document.querySelectorAll(".scene")].every((el) => el.getBoundingClientRect().height > 40),
    );
    if (visible) ok("reduced-motion: scenes rendered in vertical flow");
    else fail("reduced-motion: scenes collapsed");
    await ctx.close();
  }

  // ===================== Browser language detection =====================
  {
    const ctx = await browser.newContext({ viewport: { width: 1280, height: 800 }, locale: "tr-TR" });
    const page = await ctx.newPage();
    await page.goto(url, { waitUntil: "load" });
    await page.waitForTimeout(300);
    eq(await page.evaluate(() => document.documentElement.lang), "tr", "detection: tr-TR browser → Turkish by default");
    const title = await page.title();
    if (/salt okunur/i.test(title)) ok("detection: Turkish title applied");
    else fail(`detection: title not Turkish ("${title}")`);
    await ctx.close();
  }
} finally {
  await browser.close();
  server.close();
}

console.log(failures === 0 ? "\nALL BROWSER CHECKS PASSED" : `\n${failures} BROWSER CHECK(S) FAILED`);
process.exit(failures === 0 ? 0 : 1);
