import { SheetHead } from "./SheetHead";
import { ScreenshotFrame } from "./ScreenshotFrame";
import { useI18n } from "../i18n";

export function Diagnostics() {
  const { t } = useI18n();
  const headId = "diagnostics-title";

  return (
    <section className="section" id="diagnostics" aria-labelledby={headId}>
      <div className="shell">
        <SheetHead name={t.diag.shotTitle} index="04 / 06" />
        <div className="head">
          <h2 className="head__title" id={headId}>
            {t.diag.heading}
          </h2>
          <p className="lead">{t.diag.lead}</p>
        </div>

        <div className="split split--flip">
          <div>
            <ul className="points">
              {t.diag.points.map((p) => (
                <li key={p}>
                  <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.7" aria-hidden="true">
                    <path d="M20 6 9 17l-5-5" />
                  </svg>
                  <span>{p}</span>
                </li>
              ))}
            </ul>

            <div style={{ marginTop: "1.7rem" }}>
              <p className="json__note">{t.diag.schemaNote}</p>
              <pre className="json" aria-label={t.diag.schemaNote}>
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
          </div>

          <ScreenshotFrame
            src="./images/diagnostics-dark.png"
            alt={t.diag.shotAlt}
            title={t.diag.shotTitle}
          />
        </div>
      </div>
    </section>
  );
}
