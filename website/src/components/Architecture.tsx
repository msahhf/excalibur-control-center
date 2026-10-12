import { SheetHead } from "./SheetHead";
import { useI18n } from "../i18n";

const STAGE_IDS = ["driver", "hwmon", "app", "output"] as const;

export function Architecture() {
  const { t } = useI18n();
  const headId = "architecture-title";

  return (
    <section className="section" id="architecture" aria-labelledby={headId}>
      <div className="shell">
        <SheetHead name={t.nav.architecture} index="03 / 06" />
        <div className="head">
          <h2 className="head__title" id={headId}>
            {t.arch.heading}
          </h2>
          <p className="lead">{t.arch.lead}</p>
        </div>

        <ol className="stages">
          {STAGE_IDS.map((id, i) => {
            const s = t.arch.stages[id];
            return (
              <li className="stage" key={id}>
                <span className="stage__no">{String(i + 1).padStart(2, "0")}</span>
                <h3 className="stage__title">{s.title}</h3>
                <p className="stage__body">{s.body}</p>
                <code className="stage__code" translate="no">
                  {s.code}
                </code>
              </li>
            );
          })}
        </ol>
      </div>
    </section>
  );
}
