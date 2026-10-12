import { BrandMark } from "./Brand";
import { useI18n, interpolate } from "../i18n";
import {
  AUTHOR,
  ISSUES,
  LICENSE_URL,
  README,
  RELEASES,
  RELEASE_PAGE,
  RELEASE_VERSION,
  REPO,
} from "../data/site";

export function Footer() {
  const { t } = useI18n();
  const year = new Date().getFullYear();

  return (
    <footer className="footer">
      <div className="shell">
        <div className="footer__grid">
          <div className="footer__col">
            <a href="#top" className="brand" aria-label={t.nav.home}>
              <BrandMark className="brand__mark" />
              <span className="brand__word" translate="no">
                EXCALIBUR<small>CONTROL CENTER</small>
              </span>
            </a>
            <p style={{ marginTop: "0.9rem", color: "var(--ink-2)", maxWidth: "32ch", fontSize: "0.92rem" }}>
              {t.footer.tagline}
            </p>
          </div>

          <div className="footer__col">
            <h4>{t.footer.project}</h4>
            <ul>
              <li>
                <a href={REPO} target="_blank" rel="noreferrer noopener">
                  {t.footer.repo}
                </a>
              </li>
              <li>
                <a href={RELEASES} target="_blank" rel="noreferrer noopener">
                  {t.footer.releases}
                </a>
              </li>
              <li>
                <a href={README} target="_blank" rel="noreferrer noopener">
                  {t.footer.readme}
                </a>
              </li>
              <li>
                <a href={ISSUES} target="_blank" rel="noreferrer noopener">
                  {t.footer.issues}
                </a>
              </li>
            </ul>
          </div>

          <div className="footer__col">
            <h4>{t.footer.downloadCol}</h4>
            <ul>
              <li>
                <a href={RELEASE_PAGE} target="_blank" rel="noreferrer noopener">
                  v{RELEASE_VERSION}
                </a>
              </li>
              <li>
                <a href="#download">{t.footer.packages}</a>
              </li>
              <li>
                <a href="#compatibility">{t.footer.compatibility}</a>
              </li>
              <li>
                <a href="#architecture">{t.footer.architecture}</a>
              </li>
            </ul>
          </div>

          <div className="footer__col">
            <h4>{t.footer.legalCol}</h4>
            <ul>
              <li>
                <a href={LICENSE_URL} target="_blank" rel="noreferrer noopener">
                  {t.footer.license}
                </a>
              </li>
              <li>
                <a href={ISSUES} target="_blank" rel="noreferrer noopener">
                  {t.footer.diagnostics}
                </a>
              </li>
            </ul>
          </div>
        </div>

        <div className="footer__bottom">
          <span>{interpolate(t.footer.copyright, { year })}</span>
          <span className="footer__legal">
            <span>{t.footer.notAffiliated}</span>
            <span>{t.footer.noTracking}</span>
          </span>
        </div>

        {/* Attribution — always the same words in every language. */}
        <div className="credit">
          <span>
            <span translate="no">{t.footer.credit}</span>{" "}
            <a href={AUTHOR} target="_blank" rel="noreferrer noopener" translate="no">
              Muhammedşah Fidan
            </a>
          </span>
        </div>
      </div>
    </footer>
  );
}
