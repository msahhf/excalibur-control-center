import { ScreenshotFrame } from "./ScreenshotFrame";
import { IconArrow, IconDownload } from "./Icons";
import { useI18n } from "../i18n";
import { RELEASE_PAGE, RELEASE_VERSION } from "../data/site";

export function Hero() {
  const { t } = useI18n();
  return (
    <section className="hero" id="top">
      <div className="hero__aura" aria-hidden="true" />

      <div className="shell">
        <div className="hero__head">
          <div className="hero__headline">
            <p className="hero__kicker">{t.hero.kicker}</p>
            <h1 className="hero__title">
              {t.hero.titleA}
              <span>{t.hero.titleB}</span>
            </h1>
          </div>

          <div className="hero__aside">
            <p className="lead hero__lead">{t.hero.lead}</p>
            <div className="hero__cta">
              <a className="btn btn--primary btn--lg" href="#telemetry">
                {t.hero.ctaExplore} <IconArrow />
              </a>
              <a className="btn btn--lg" href={RELEASE_PAGE} target="_blank" rel="noreferrer noopener">
                <IconDownload /> {t.hero.ctaDownload} v{RELEASE_VERSION}
              </a>
            </div>
          </div>
        </div>

        <ul className="hero__meta" aria-label={t.hero.metaLabel}>
          {t.hero.facts.map((f) => (
            <li key={f}>{f}</li>
          ))}
        </ul>
      </div>

      <div className="hero__stage">
        <div className="hero__stage-inner">
          <span className="hero__stage-floor" aria-hidden="true" />
          <span className="hero__stage-frame" aria-hidden="true" />
          <div className="hero__shot">
            <ScreenshotFrame
              src="./images/dashboard-dark.png"
              alt={t.hero.shotAlt}
              title={t.hero.shotTitle}
              eager
            />
          </div>
        </div>

        <a className="hero__cue" href="#telemetry">
          <span>{t.hero.scroll}</span>
          <i aria-hidden="true" />
        </a>
      </div>
    </section>
  );
}
