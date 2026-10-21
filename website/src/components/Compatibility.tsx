import { SheetHead } from "./SheetHead";
import { useI18n } from "../i18n";
import { COMPAT_ROWS, type CompatRowId } from "../data/site";

const TONE: Record<CompatRowId, "ok" | "warn" | "muted"> = {
  hardware: "ok",
  primaryDistro: "ok",
  publishedPackages: "ok",
  appimage: "muted",
  kernel72: "ok",
  kernel618: "warn",
  fedora: "muted",
};

export function Compatibility() {
  const { t } = useI18n();
  return (
    <section className="section" id="compatibility" aria-labelledby="compat-title">
      <div className="shell">
        <SheetHead name={t.nav.compatibility} index="06 / 06" />
        <div className="head">
          <h2 className="head__title" id="compat-title">
            {t.compat.heading}
          </h2>
          <p className="lead">{t.compat.lead}</p>
        </div>

        <ul className="compat">
          {COMPAT_ROWS.map((id) => {
            const row = t.compat.rows[id];
            return (
              <li className="compat__row" key={id}>
                <span>{row.label}</span>
                <span className={`tag tag--${TONE[id]}`}>{row.tag}</span>
              </li>
            );
          })}
        </ul>
        <p className="pkg__note" style={{ marginTop: "1.3rem", maxWidth: "70ch" }}>
          {t.compat.note}
        </p>
      </div>
    </section>
  );
}
