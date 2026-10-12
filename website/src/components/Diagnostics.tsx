import { Reveal } from "./Reveal";
import { ScreenshotFrame } from "./ScreenshotFrame";
import { IconCheck, IconDoc, IconPulse, IconShield } from "./Icons";

const points = [
  { icon: <IconPulse />, text: "System and kernel identity — OS, kernel, architecture, desktop and session type." },
  { icon: <IconShield />, text: "Hardware telemetry that is actually reachable — temperature and fan channels, or an honest “unavailable”." },
  { icon: <IconDoc />, text: "A rolling history of the most recent samples, windowed to the last 60 seconds." },
  { icon: <IconCheck />, text: "One JSON export you can attach to a support thread — no environment variables, no credentials." },
];

export function Diagnostics() {
  return (
    <section className="section" id="diagnostics" aria-labelledby="diag-title">
      <div className="shell">
        <Reveal className="head">
          <span className="eyebrow">Exportable diagnostics</span>
          <h2 className="head__title" id="diag-title">
            When something looks off, you have the receipts.
          </h2>
          <p className="lead">
            The Diagnostics screen gathers everything the app knows into one view — and exports it as portable
            JSON so a bug report carries evidence instead of guesswork.
          </p>
        </Reveal>

        <div className="split split--flip">
          <Reveal>
            <ScreenshotFrame
              src="./images/diagnostics-dark.png"
              alt="Diagnostics page: application, system and telemetry sections plus an Export Diagnostics button"
              title="Diagnostics"
            />
          </Reveal>

          <Reveal delay={120}>
            <ul className="points">
              {points.map((p, i) => (
                <li key={i}>
                  {p.icon}
                  <span>{p.text}</span>
                </li>
              ))}
            </ul>

            <div style={{ marginTop: "1.8rem" }}>
              <div className="json__note">
                <span className="dot" style={{ background: "var(--cyan)", color: "var(--cyan)" }} />
                Representative schema — real field names from the app; values shown are placeholders.
              </div>
              <pre className="json" aria-label="Representative diagnostics JSON schema">
                <code>
                  {"{\n"}
                  {"  "}
                  <span className="k">"application"</span>
                  {": {\n"}
                  {"    "}
                  <span className="k">"product_name"</span>
                  {": "}
                  <span className="s">"EXCALIBUR Control Center"</span>
                  {",\n"}
                  {"    "}
                  <span className="k">"version"</span>
                  {": "}
                  <span className="s">"0.5.6"</span>
                  {",  "}
                  <span className="k">"build_type"</span>
                  {": "}
                  <span className="s">"Release"</span>
                  {"\n  },\n"}
                  {"  "}
                  <span className="k">"system"</span>
                  {": {\n"}
                  {"    "}
                  <span className="k">"os"</span>
                  {": "}
                  <span className="s">"…"</span>
                  {",  "}
                  <span className="k">"kernel"</span>
                  {": "}
                  <span className="s">"…"</span>
                  {",\n"}
                  {"    "}
                  <span className="k">"architecture"</span>
                  {": "}
                  <span className="s">"x86_64"</span>
                  {"\n  },\n"}
                  {"  "}
                  <span className="k">"telemetry"</span>
                  {": {\n"}
                  {"    "}
                  <span className="k">"state"</span>
                  {": "}
                  <span className="s">"Connected"</span>
                  {",\n"}
                  {"    "}
                  <span className="k">"cpu_temperature_c"</span>
                  {": "}
                  <span className="n">null</span>
                  {",  "}
                  <span className="k">"cpu_fan_rpm"</span>
                  {": "}
                  <span className="n">null</span>
                  {"\n  },\n"}
                  {"  "}
                  <span className="k">"sensors"</span>
                  {": { "}
                  <span className="k">"available"</span>
                  {": "}
                  <span className="n">true</span>
                  {" },\n"}
                  {"  "}
                  <span className="k">"history"</span>
                  {": { "}
                  <span className="k">"window_seconds"</span>
                  {": "}
                  <span className="n">60</span>
                  {", "}
                  <span className="k">"samples"</span>
                  {": "}
                  <span className="c">[ … ]</span>
                  {" }\n"}
                  {"}\n"}
                </code>
              </pre>
            </div>
          </Reveal>
        </div>
      </div>
    </section>
  );
}
