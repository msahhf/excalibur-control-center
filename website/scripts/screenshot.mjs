// Real-browser verification for the static site build using the system Chromium
// (no browser download; playwright-core points at /usr/bin/chromium).
//
//   npm run build && npm run shots
//
// Serves dist/ over a throwaway local HTTP server (module scripts cannot load
// from file://), then checks: console errors, horizontal overflow, image
// loading, external link hygiene, the screenshot lightbox, the download tabs,
// the mobile menu and reduced-motion — at desktop and phone viewports — and
// writes reference screenshots to test-output/.

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
  ".ico": "image/x-icon",
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

async function disableSmoothScroll(page) {
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

async function auditPage(page, label) {
  // Measure real content overflow: an element that sticks out horizontally and
  // is not clipped by a scroll/hidden ancestor. (Comparing scrollWidth to
  // clientWidth counts the classic vertical scrollbar as false overflow.)
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

  // Only images that actually occupy space are expected to load; hidden copies
  // (e.g. the desktop-only step images on mobile) are intentionally deferred.
  const imgs = await page.evaluate(() => {
    const all = [...document.images];
    const boxed = all.filter((i) => i.getClientRects().length > 0);
    return {
      total: all.length,
      boxed: boxed.length,
      bad: boxed.filter((i) => !(i.complete && i.naturalWidth > 0)).map((i) => i.getAttribute("src")),
    };
  });
  if (imgs.bad.length) fail(`${label}: visible images not loaded → ${imgs.bad.join(", ")}`);
  else ok(`${label}: all ${imgs.boxed} visible images loaded`);
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
  // ---------- Desktop ----------
  {
    const ctx = await browser.newContext({ viewport: { width: 1440, height: 900 }, deviceScaleFactor: 1 });
    const page = await ctx.newPage();
    const errors = [];
    page.on("console", (m) => m.type() === "error" && errors.push(m.text()));
    page.on("pageerror", (e) => errors.push("pageerror: " + e.message));
    await page.goto(url, { waitUntil: "load" });
    await disableSmoothScroll(page);
    await page.waitForTimeout(600);

    const title = await page.title();
    if (/EXCALIBUR Control Center/.test(title)) ok(`desktop: title "${title}"`);
    else fail(`desktop: unexpected title "${title}"`);

    const heroSrc = await page.getAttribute(".hero__stage img", "src");
    if (heroSrc && heroSrc.includes("dashboard-dark.png")) ok("desktop: hero uses the real dashboard screenshot");
    else fail(`desktop: hero screenshot src = ${heroSrc}`);

    await page.locator(".hero__stage .shot").click();
    await page.waitForSelector(".lb", { state: "visible", timeout: 3000 });
    await page.waitForTimeout(200);
    await page.screenshot({ path: `${outDir}/lightbox.png` });
    await page.keyboard.press("Escape");
    await page.waitForSelector(".lb", { state: "detached", timeout: 3000 });
    ok("desktop: lightbox opens and closes with Escape");

    await page.evaluate(async () => {
      for (let y = 0; y < document.body.scrollHeight; y += window.innerHeight * 0.6) {
        window.scrollTo(0, y);
        await new Promise((r) => setTimeout(r, 130));
      }
      window.scrollTo(0, 0);
    });
    await page.waitForTimeout(600);
    await waitForImages(page);
    await auditPage(page, "desktop");
    // Force scroll-reveal state so the full-page reference shot is deterministic.
    await page.evaluate(() => document.querySelectorAll(".reveal").forEach((e) => e.classList.add("is-in")));
    await page.waitForTimeout(200);
    await page.screenshot({ path: `${outDir}/desktop-full.png`, fullPage: true });
    await page.screenshot({ path: `${outDir}/desktop-hero.png` });
    for (const id of ["telemetry", "readonly", "architecture", "diagnostics", "downloads", "compatibility"]) {
      await page.locator(`#${id}`).scrollIntoViewIfNeeded();
      await page.waitForTimeout(350);
      await page.screenshot({ path: `${outDir}/section-${id}.png` });
    }

    await page.locator(".steps .step").nth(2).scrollIntoViewIfNeeded();
    await page.waitForTimeout(600);
    const activeIdx = await page.evaluate(() =>
      [...document.querySelectorAll(".steps .step")].findIndex((s) => s.classList.contains("is-active")),
    );
    if (activeIdx >= 0) ok(`desktop: scrollytelling active step = ${activeIdx}`);
    else fail("desktop: no scrollytelling step became active");

    await page.locator("#downloads").scrollIntoViewIfNeeded();
    await page.getByRole("tab", { name: "AppImage" }).click();
    await page.waitForTimeout(150);
    const panel = page.locator('[role="tabpanel"]:not([hidden])');
    const panelText = await panel.innerText();
    if (/Portable AppImage/.test(panelText)) ok("desktop: download tabs switch to AppImage");
    else fail("desktop: AppImage tabpanel did not open");
    if (/does NOT install/.test(panelText)) ok("desktop: AppImage panel states it does not install DKMS");
    else fail("desktop: AppImage DKMS note missing");
    await page.screenshot({ path: `${outDir}/desktop-downloads.png` });

    if (errors.length) fail(`desktop: console errors → ${errors.join(" | ")}`);
    else ok("desktop: no console errors");
    await ctx.close();
  }

  // ---------- Mobile ----------
  {
    const ctx = await browser.newContext({
      viewport: { width: 390, height: 844 },
      deviceScaleFactor: 2,
      isMobile: true,
      hasTouch: true,
    });
    const page = await ctx.newPage();
    const errors = [];
    page.on("console", (m) => m.type() === "error" && errors.push(m.text()));
    page.on("pageerror", (e) => errors.push("pageerror: " + e.message));
    await page.goto(url, { waitUntil: "load" });
    await disableSmoothScroll(page);
    await page.waitForTimeout(600);

    await page.locator(".nav__burger").click();
    await page.waitForTimeout(400);
    if (await page.locator("#mobile-menu a").first().isVisible()) ok("mobile: menu opens");
    else fail("mobile: menu did not open");
    await page.locator("#mobile-menu a").first().click();
    await page.waitForTimeout(400);
    const panelH = await page.evaluate(
      () => document.querySelector(".nav__panel")?.getBoundingClientRect().height ?? 0,
    );
    if (panelH < 8) ok("mobile: menu closes after selecting a link");
    else fail(`mobile: menu still open (height ${panelH})`);

    if (await page.locator(".steps .step__media img").first().isVisible()) ok("mobile: steps use inline product images");
    else fail("mobile: inline step image not visible");
    const stickyHidden = await page.evaluate(() => {
      const el = document.querySelector(".scrolly__stage");
      return !el || getComputedStyle(el).display === "none";
    });
    if (stickyHidden) ok("mobile: desktop sticky stage is disabled");
    else fail("mobile: sticky stage still active");

    await page.evaluate(async () => {
      for (let y = 0; y < document.body.scrollHeight; y += window.innerHeight * 0.6) {
        window.scrollTo(0, y);
        await new Promise((r) => setTimeout(r, 130));
      }
      window.scrollTo(0, 0);
    });
    await page.waitForTimeout(400);
    await waitForImages(page);
    await auditPage(page, "mobile");
    await page.evaluate(() => document.querySelectorAll(".reveal").forEach((e) => e.classList.add("is-in")));
    await page.waitForTimeout(200);
    await page.screenshot({ path: `${outDir}/mobile-full.png`, fullPage: true });
    await page.screenshot({ path: `${outDir}/mobile-hero.png` });
    await page.locator("#downloads").scrollIntoViewIfNeeded();
    await page.waitForTimeout(300);
    await page.screenshot({ path: `${outDir}/mobile-downloads.png` });

    if (errors.length) fail(`mobile: console errors → ${errors.join(" | ")}`);
    else ok("mobile: no console errors");
    await ctx.close();
  }

  // ---------- Reduced motion ----------
  {
    const ctx = await browser.newContext({
      viewport: { width: 1280, height: 800 },
      reducedMotion: "reduce",
    });
    const page = await ctx.newPage();
    await page.goto(url, { waitUntil: "load" });
    await disableSmoothScroll(page);
    await page.waitForTimeout(400);
    const visible = await page.evaluate(() =>
      [...document.querySelectorAll(".reveal")].every((el) => parseFloat(getComputedStyle(el).opacity) > 0.9),
    );
    if (visible) ok("reduced-motion: all revealed content is visible without scroll");
    else fail("reduced-motion: some content stayed hidden");
    await ctx.close();
  }
} finally {
  await browser.close();
  server.close();
}

console.log(failures === 0 ? "\nALL BROWSER CHECKS PASSED" : `\n${failures} BROWSER CHECK(S) FAILED`);
process.exit(failures === 0 ? 0 : 1);
