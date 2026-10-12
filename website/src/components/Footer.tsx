import { BrandMark } from "./Brand";
import { ISSUES, LICENSE_URL, README, RELEASES, RELEASE_PAGE, RELEASE_VERSION, REPO } from "../data/site";

export function Footer() {
  return (
    <footer className="footer">
      <div className="shell">
        <div className="footer__grid">
          <div className="footer__col">
            <a href="#top" className="brand" aria-label="EXCALIBUR Control Center — top">
              <BrandMark className="brand__mark" />
              <span className="brand__text">
                <span className="brand__name">EXCALIBUR</span>
                <span className="brand__sub">CONTROL CENTER</span>
              </span>
            </a>
            <p style={{ marginTop: "1rem", color: "var(--ink-2)", maxWidth: "30ch", fontSize: "0.92rem" }}>
              A read-only telemetry control center for CASPER EXCALIBUR laptops on Linux.
            </p>
          </div>

          <div className="footer__col">
            <h4>Project</h4>
            <ul>
              <li>
                <a href={REPO} target="_blank" rel="noreferrer noopener">
                  GitHub repository
                </a>
              </li>
              <li>
                <a href={RELEASES} target="_blank" rel="noreferrer noopener">
                  Releases
                </a>
              </li>
              <li>
                <a href={README} target="_blank" rel="noreferrer noopener">
                  README
                </a>
              </li>
              <li>
                <a href={ISSUES} target="_blank" rel="noreferrer noopener">
                  Report an issue
                </a>
              </li>
            </ul>
          </div>

          <div className="footer__col">
            <h4>Download</h4>
            <ul>
              <li>
                <a href={RELEASE_PAGE} target="_blank" rel="noreferrer noopener">
                  v{RELEASE_VERSION} release
                </a>
              </li>
              <li>
                <a href="#downloads">Packages &amp; AppImage</a>
              </li>
              <li>
                <a href="#compatibility">Compatibility</a>
              </li>
              <li>
                <a href="#architecture">Architecture</a>
              </li>
            </ul>
          </div>

          <div className="footer__col">
            <h4>Legal</h4>
            <ul>
              <li>
                <a href={LICENSE_URL} target="_blank" rel="noreferrer noopener">
                  License (GPL-2.0-only)
                </a>
              </li>
              <li>
                <a href={ISSUES} target="_blank" rel="noreferrer noopener">
                  Diagnostics &amp; support
                </a>
              </li>
            </ul>
          </div>
        </div>

        <div className="footer__bottom">
          <span>© {new Date().getFullYear()} EXCALIBUR Control Center · GPL-2.0-only</span>
          <span className="footer__legal">
            <span>Not affiliated with or endorsed by CASPER.</span>
            <span>Static showcase — no tracking, no cookies.</span>
          </span>
        </div>
      </div>
    </footer>
  );
}
