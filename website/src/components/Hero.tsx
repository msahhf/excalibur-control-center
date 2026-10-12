import { ScreenshotFrame } from "./ScreenshotFrame";
import { IconArrow, IconDownload } from "./Icons";
import { useI18n } from "../i18n";
import { RELEASE_PAGE, RELEASE_VERSION } from "../data/site";

export function Hero() {
  const { t } = useI18n();
  return (
    <section className="hero" id="top">
      <div className="shell">
        <div className="hero__grid">
          <div className="hero__copy">
            <p className="hero__kicker">{t.hero.kicker}</p>
            <h1 className="hero__title">
              {t.hero.titleA}
              <span>{t.hero.titleB}</span>
            </h1>
            <p className="lead hero__lead">{t.hero.lead}</p>
            <div className="hero__cta">
              <a className="btn btn--primary" href="#telemetry">
                {t.hero.ctaExplore} <IconArrow />
              </a>
              <a className="btn" href={RELEASE_PAGE} target="_blank" rel="noreferrer noopener">
                <IconDownload /> {t.hero.ctaDownload} v{RELEASE_VERSION}
              </a>
            </div>
            <ul className="hero__meta" aria-label="At a glance">
              {t.hero.facts.map((f) => (
                <li key={f}>{f}</li>
              ))}
            </ul>
          </div>

          <div className="hero__shot">
            <ScreenshotFrame
              src="./images/dashboard-dark.png"
              alt={t.hero.shotAlt}
              title={t.hero.shotTitle}
              eager
            />
          </div>
        </div>
      </div>
    </section>
  );
}
