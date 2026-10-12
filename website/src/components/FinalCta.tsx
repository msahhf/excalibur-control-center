import { IconArrow, IconDownload, IconGithub } from "./Icons";
import { RELEASE_PAGE, RELEASE_VERSION, REPO } from "../data/site";

export function FinalCta() {
  return (
    <section className="final" aria-labelledby="final-title">
      <div className="shell">
        <h2 className="final__title" id="final-title">
          See your system more clearly.
        </h2>
        <p className="lead" style={{ maxWidth: "38rem", margin: "0 auto" }}>
          Read-only telemetry for CASPER EXCALIBUR laptops on Linux — native, unprivileged and honest about
          what it can and cannot do.
        </p>
        <div className="final__cta">
          <a className="btn btn--primary" href={RELEASE_PAGE} target="_blank" rel="noreferrer noopener">
            <IconDownload /> Download v{RELEASE_VERSION}
          </a>
          <a className="btn btn--ghost" href={REPO} target="_blank" rel="noreferrer noopener">
            <IconGithub /> Explore the source <IconArrow />
          </a>
        </div>
      </div>
    </section>
  );
}
