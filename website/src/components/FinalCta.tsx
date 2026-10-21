import { IconArrow, IconDownload, IconGithub } from "./Icons";
import { useI18n } from "../i18n";
import { RELEASE_PAGE, RELEASE_VERSION, REPO } from "../data/site";

export function FinalCta() {
  const { t } = useI18n();
  return (
    <section className="final" aria-labelledby="final-title">
      <div className="shell">
        <h2 className="final__title" id="final-title">
          {t.final.heading}
        </h2>
        <p className="lead final__lead">{t.final.lead}</p>
        <div className="final__cta">
          <a className="btn btn--primary" href={RELEASE_PAGE} target="_blank" rel="noreferrer noopener">
            <IconDownload /> {t.final.primary} v{RELEASE_VERSION}
          </a>
          <a className="btn" href={REPO} target="_blank" rel="noreferrer noopener">
            <IconGithub /> {t.final.secondary} <IconArrow />
          </a>
        </div>
      </div>
    </section>
  );
}
