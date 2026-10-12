import { useCallback, useEffect, useRef, useState } from "react";
import { ScreenshotFrame } from "./ScreenshotFrame";
import { SheetHead } from "./SheetHead";
import { useI18n } from "../i18n";
import { useMediaQuery, usePrefersReducedMotion } from "../hooks/useEnvironment";
import { storyImages, storyOrder } from "../data/site";

// One source of truth: `progress` (0..1) drives the horizontal transform and the
// scrub-linked meter, and a single `activeIndex` (derived from the same progress)
// drives the scene label, the navigation state and focus management — so text and
// image can never drift apart. Panels carry their own copy AND screenshot.
export function Story() {
  const { t } = useI18n();
  const reduced = usePrefersReducedMotion();
  const isDesktop = useMediaQuery("(min-width: 1000px)");
  const enhanced = isDesktop && !reduced;

  const trackRef = useRef<HTMLDivElement | null>(null);
  const railRef = useRef<HTMLDivElement | null>(null);
  const meterRef = useRef<HTMLSpanElement | null>(null);
  const sceneRefs = useRef<Array<HTMLElement | null>>([]);
  const frame = useRef(0);
  const [active, setActive] = useState(0);

  const count = storyOrder.length;

  // Scroll → progress → transform + meter + activeIndex, batched in one rAF.
  useEffect(() => {
    if (!enhanced) {
      if (railRef.current) railRef.current.style.transform = "";
      if (meterRef.current) meterRef.current.style.transform = "";
      sceneRefs.current.forEach((el) => el && (el.inert = false));
      return;
    }
    const track = trackRef.current;
    const rail = railRef.current;
    if (!track || !rail) return;

    const update = () => {
      frame.current = 0;
      const rect = track.getBoundingClientRect();
      const stickyH = window.innerHeight - 68;
      const total = track.offsetHeight - stickyH;
      const p = total > 0 ? Math.min(1, Math.max(0, -rect.top / total)) : 0;
      const gap = parseFloat(getComputedStyle(rail).columnGap) || 0;
      const shifted = -p * (count - 1) * (rail.clientWidth + gap);
      rail.style.transform = `translate3d(${shifted.toFixed(2)}px, 0, 0)`;
      if (meterRef.current) meterRef.current.style.transform = `scaleX(${p.toFixed(4)})`;
      const idx = Math.min(count - 1, Math.max(0, Math.round(p * (count - 1))));
      setActive((prev) => (prev === idx ? prev : idx));
    };
    const onScroll = () => {
      if (!frame.current) frame.current = requestAnimationFrame(update);
    };

    update();
    window.addEventListener("scroll", onScroll, { passive: true });
    window.addEventListener("resize", onScroll);
    return () => {
      window.removeEventListener("scroll", onScroll);
      window.removeEventListener("resize", onScroll);
      if (frame.current) cancelAnimationFrame(frame.current);
      frame.current = 0;
    };
  }, [enhanced, count]);

  // Keep off-screen scenes out of the tab order while the horizontal layout is on.
  useEffect(() => {
    sceneRefs.current.forEach((el, i) => {
      if (el) el.inert = enhanced && i !== active;
    });
  }, [enhanced, active]);

  const goTo = useCallback(
    (i: number) => {
      if (!enhanced) {
        sceneRefs.current[i]?.scrollIntoView({ block: "start" });
        return;
      }
      const track = trackRef.current;
      if (!track) return;
      const stickyH = window.innerHeight - 68;
      const total = track.offsetHeight - stickyH;
      const absTop = window.scrollY + track.getBoundingClientRect().top;
      const top = absTop + (count > 1 ? (i / (count - 1)) * total : 0);
      window.scrollTo({ top, behavior: reduced ? "auto" : "smooth" });
    },
    [enhanced, count, reduced],
  );

  const headId = "telemetry-title";
  const pad = (n: number) => String(n).padStart(2, "0");

  return (
    <section className="section" id="telemetry" aria-labelledby={headId}>
      <div className="shell">
        <SheetHead name={t.nav.telemetry} index="01 / 06" />

        <div className="story" data-enhanced={enhanced ? "true" : "false"}>
          {/* Stage 1 — the story opens in normal vertical flow. */}
          <div className="story__intro">
            <div className="story__intro-copy">
              <h2 className="head__title" id={headId}>
                {t.story.heading}
              </h2>
              <p className="lead">{t.story.lead}</p>
            </div>
            <div className="story__intro-shot">
              <ScreenshotFrame
                src={storyImages[storyOrder[0]]}
                alt={t.story.scenes[storyOrder[0]].alt}
                title={t.story.scenes[storyOrder[0]].title}
                eager
              />
            </div>
          </div>

          {/* Stage 2 — horizontal scenes on desktop; a natural vertical stack
              everywhere else (mobile, reduced motion, or with JS unavailable). */}
          <div className="story__track" ref={trackRef}>
            <div className="story__sticky">
              <div className="story__rail" ref={railRef}>
                {storyOrder.map((id, i) => {
                  const sc = t.story.scenes[id];
                  return (
                    <article
                      key={id}
                      className="scene"
                      id={`scene-${id}`}
                      ref={(el) => {
                        sceneRefs.current[i] = el;
                      }}
                    >
                      <div className="scene__copy">
                        <span className="scene__ghost tnum" aria-hidden="true">
                          {pad(i + 1)}
                        </span>
                        <span className="scene__no tnum">
                          {t.story.sceneLabel} {pad(i + 1)} {t.story.of} {pad(count)}
                        </span>
                        <h3 className="scene__title">{sc.title}</h3>
                        <p className="scene__body">{sc.body}</p>
                      </div>
                      <div className="scene__media">
                        <ScreenshotFrame
                          src={storyImages[id]}
                          alt={sc.alt}
                          title={sc.title}
                          eager={i === 0}
                        />
                      </div>
                    </article>
                  );
                })}
              </div>

              {/* Scrub-linked meter: shares the same progress as the transform. */}
              <div className="story__meter" aria-hidden="true">
                <span className="story__meter-count tnum">
                  {pad(active + 1)} <i>/</i> {pad(count)}
                </span>
                <span className="story__meter-track">
                  <span className="story__meter-fill" ref={meterRef} />
                </span>
              </div>
            </div>
          </div>

          {/* Scene navigation: shared state with the scenes above. */}
          <nav className="story__nav" aria-label={t.story.scenesLabel}>
            {storyOrder.map((id, i) => (
              <button
                key={id}
                type="button"
                className="story__dot"
                aria-current={active === i ? "true" : undefined}
                aria-controls={`scene-${id}`}
                aria-label={`${t.story.sceneLabel} ${i + 1}: ${t.story.scenes[id].title}`}
                onClick={() => goTo(i)}
              >
                <i aria-hidden="true" />
                <span className="tnum">{pad(i + 1)}</span>
              </button>
            ))}
          </nav>
        </div>
      </div>
    </section>
  );
}
