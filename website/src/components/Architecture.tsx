import { Reveal } from "./Reveal";
import { IconChip, IconDoc, IconFan, IconPulse } from "./Icons";

const stages = [
  {
    n: "01",
    icon: <IconChip />,
    title: "EXCALIBUR WMI / kernel driver",
    body: "A small read-only kernel module, excalibur-wmi, speaks the laptop's WMI interface and registers a standard hwmon device. It requests telemetry; it never writes hardware state.",
    code: "excalibur_wmi · WMI GET 0x200",
  },
  {
    n: "02",
    icon: <IconPulse />,
    title: "Read-only hwmon / sysfs",
    body: "The kernel exposes the readings as ordinary sysfs files under a device named excalibur_g870 — the single, unprivileged interface the whole app reads from.",
    code: "temp1_input · temp2_input · fan1_input · fan2_input",
  },
  {
    n: "03",
    icon: <IconFan />,
    title: "Qt6 native desktop app",
    body: "One HwmonClient discovers the device by name and reads it on a timer. Every page is fed from a single telemetry state — no page touches sysfs on its own.",
    code: "HwmonClient → TelemetryModel → pages",
  },
  {
    n: "04",
    icon: <IconDoc />,
    title: "Visual monitoring & diagnostics",
    body: "The dashboard, cooling and device views render the live snapshot, while Diagnostics turns the same state into a portable JSON report for support.",
    code: "Dashboard · Cooling · Device · Diagnostics",
  },
];

export function Architecture() {
  return (
    <section className="section" id="architecture" aria-labelledby="arch-title">
      <div className="shell">
        <Reveal className="head">
          <span className="eyebrow">How it works</span>
          <h2 className="head__title" id="arch-title">
            A straight line from silicon to screen.
          </h2>
          <p className="lead">
            Four honest stages. The same architecture described in the project's source and README — nothing is
            embellished for this page.
          </p>
        </Reveal>

        <ol className="pillars" style={{ gridTemplateColumns: "1fr", listStyle: "none", margin: 0, padding: 0 }}>
          {stages.map((s, i) => (
            <Reveal as="li" key={s.n} delay={i * 90} className="pillar">
              <div style={{ display: "flex", alignItems: "center", gap: "0.9rem", marginBottom: "0.9rem" }}>
                <span className="pillar__ic" aria-hidden="true">
                  {s.icon}
                </span>
                <span className="step__no">STAGE {s.n}</span>
              </div>
              <h3>{s.title}</h3>
              <p>{s.body}</p>
              <p className="mono" style={{ marginTop: "0.9rem", fontSize: "0.78rem", color: "var(--ink-3)" }}>
                {s.code}
              </p>
            </Reveal>
          ))}
        </ol>
      </div>
    </section>
  );
}
