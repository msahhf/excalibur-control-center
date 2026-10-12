import { useEffect, useRef, useState } from "react";
import { ScreenshotFrame } from "./ScreenshotFrame";
import { Reveal } from "./Reveal";
import { steps } from "../data/site";

export function Scrolly() {
  const [active, setActive] = useState(0);
  const refs = useRef<Array<HTMLLIElement | null>>([]);

  useEffect(() => {
    const update = () => {
      const mid = window.innerHeight * 0.5;
      let best = 0;
      let bestDist = Infinity;
      refs.current.forEach((el, i) => {
        if (!el) return;
        const r = el.getBoundingClientRect();
        const center = r.top + r.height / 2;
        const dist = Math.abs(center - mid);
        if (dist < bestDist) {
          bestDist = dist;
          best = i;
        }
      });
      setActive(best);
    };
    update();
    window.addEventListener("scroll", update, { passive: true });
    window.addEventListener("resize", update);
    return () => {
      window.removeEventListener("scroll", update);
      window.removeEventListener("resize", update);
    };
  }, []);

  return (
    <section className="section" id="telemetry" aria-labelledby="telemetry-title">
      <div className="shell">
        <Reveal className="head">
          <span className="eyebrow">Telemetry you can see</span>
          <h2 className="head__title" id="telemetry-title">
            Four screens, one clear story about your machine.
          </h2>
          <p className="lead">
            Every value is the real reading from the read-only hwmon boundary — nothing is averaged, clamped or
            invented. Scroll to follow what each screen shows.
          </p>
        </Reveal>

        <div className="scrolly__grid">
          <ol className="steps scrolly__steps">
            {steps.map((s, i) => (
              <li
                key={s.id}
                ref={(el) => {
                  refs.current[i] = el;
                }}
                className={["step", i === active ? "is-active" : ""].join(" ")}
              >
                <span className="step__no">STEP {s.no}</span>
                <h3 className="step__title">{s.title}</h3>
                <p className="step__body">{s.body}</p>
                <div className="step__media">
                  <ScreenshotFrame src={s.image} alt={s.alt} title={s.title} eager={i === 0} />
                </div>
              </li>
            ))}
          </ol>

          <div className="scrolly__stage">
            <div className="scrolly__sticky">
              <div className="scrolly__media">
                {steps.map((s, i) => (
                  <div key={s.id} className={["frame", i === active ? "is-active" : ""].join(" ")}>
                    <div className="frame__bar">
                      <span className="frame__dots" aria-hidden="true">
                        <i />
                        <i />
                        <i />
                      </span>
                      <span className="frame__title">{s.title}</span>
                      <span className="frame__app">excalibur-control-center</span>
                    </div>
                    <img className="frame__img" src={s.image} alt={s.alt} loading="lazy" decoding="async" />
                  </div>
                ))}
              </div>
            </div>
          </div>
        </div>
      </div>
    </section>
  );
}
