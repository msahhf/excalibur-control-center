import { Reveal } from "./Reveal";
import { compat } from "../data/site";

const toneClass = {
  ok: "tag tag--ok",
  warn: "tag tag--warn",
  muted: "tag tag--muted",
} as const;

export function Compatibility() {
  return (
    <section className="section" id="compatibility" aria-labelledby="compat-title">
      <div className="shell">
        <Reveal className="head">
          <span className="eyebrow">Compatibility</span>
          <h2 className="head__title" id="compat-title">
            Verified scope, stated plainly.
          </h2>
          <p className="lead">
            What has actually been tested is marked as verified; everything else is labelled honestly. No
            platform is presented as supported before it has been.
          </p>
        </Reveal>

        <Reveal>
          <ul className="compat" style={{ listStyle: "none", margin: 0, padding: 0 }}>
            {compat.map((c) => (
              <li className="compat__row" key={c.label}>
                <span>{c.label}</span>
                <span className={toneClass[c.tone]}>{c.tag}</span>
              </li>
            ))}
          </ul>
          <p className="dl__note" style={{ marginTop: "1.4rem", maxWidth: "60ch" }}>
            The read-only driver targets the kernel WMI API available in the 7.2.x series. It does not build on
            6.18.x LTS, and a Fedora RPM is not published. AppImage ships the GUI only — install a native driver
            package for live telemetry.
          </p>
        </Reveal>
      </div>
    </section>
  );
}
