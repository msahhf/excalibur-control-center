import { useEffect, useRef } from "react";
import { ScreenshotFrame } from "./ScreenshotFrame";
import { IconArrow, IconDownload } from "./Icons";
import { RELEASE_PAGE, RELEASE_VERSION } from "../data/site";
import { usePrefersReducedMotion } from "../hooks/useReveal";

/** Very subtle 3D tilt that follows the pointer — desktop, hover-capable only. */
function usePointerTilt() {
  const ref = useRef<HTMLDivElement | null>(null);
  const reduced = usePrefersReducedMotion();

  useEffect(() => {
    const el = ref.current;
    if (!el || reduced) return;
    if (!window.matchMedia("(hover: hover) and (pointer: fine)").matches) return;

    let raf = 0;
    const onMove = (e: MouseEvent) => {
      if (raf) return;
      raf = requestAnimationFrame(() => {
        raf = 0;
        const r = el.getBoundingClientRect();
        const px = (e.clientX - r.left) / r.width - 0.5;
        const py = (e.clientY - r.top) / r.height - 0.5;
        el.style.setProperty("--rx", `${(-py * 6).toFixed(2)}deg`);
        el.style.setProperty("--ry", `${(px * 8 - 6).toFixed(2)}deg`);
      });
    };
    const onLeave = () => {
      el.style.setProperty("--rx", "6deg");
      el.style.setProperty("--ry", "-6deg");
    };
    el.addEventListener("mousemove", onMove);
    el.addEventListener("mouseleave", onLeave);
    return () => {
      el.removeEventListener("mousemove", onMove);
      el.removeEventListener("mouseleave", onLeave);
      if (raf) cancelAnimationFrame(raf);
    };
  }, [reduced]);

  return ref;
}

export function Hero() {
  const tilt = usePointerTilt();

  return (
    <section className="hero" id="top">
      <div className="shell">
        <div className="hero__grid">
          <div className="hero__copy reveal is-in">
            <span className="eyebrow">Read-only hardware telemetry · Linux</span>
            <h1 className="hero__title">
              Your hardware.
              <br />
              <em>Clearly understood.</em>
            </h1>
            <p className="lead">
              EXCALIBUR Control Center is a native Qt6 desktop app that reads the temperature, fan and
              system telemetry your CASPER EXCALIBUR laptop already exposes on Linux — CPU/GPU °C, fan RPM,
              hardware and kernel details, and an exportable diagnostics snapshot.
            </p>
            <div className="hero__cta">
              <a className="btn btn--primary" href="#telemetry">
                Explore the experience <IconArrow />
              </a>
              <a className="btn btn--ghost" href={RELEASE_PAGE} target="_blank" rel="noreferrer noopener">
                <IconDownload /> Get v{RELEASE_VERSION}
              </a>
            </div>
            <ul className="hero__facts" aria-label="At a glance">
              <li className="tag">CPU / GPU temperature</li>
              <li className="tag">Fan RPM</li>
              <li className="tag">60-second history</li>
              <li className="tag">Diagnostics JSON</li>
              <li className="tag">No root required</li>
            </ul>
          </div>

          <div className="hero__stage reveal is-in" style={{ ["--reveal-delay" as string]: "120ms" }}>
            <div className="hero__glow" aria-hidden="true" />
            <div ref={tilt} style={{ perspective: "1600px" }}>
              <ScreenshotFrame
                className="frame--tilt"
                src="./images/dashboard-dark.png"
                alt="EXCALIBUR Control Center dashboard: CPU and GPU temperatures with 60-second history and fan RPM"
                title="Dashboard"
                eager
              />
            </div>
          </div>
        </div>
      </div>
    </section>
  );
}
