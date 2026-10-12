import { useId, useState } from "react";
import { Reveal } from "./Reveal";
import { IconArrow, IconDownload, IconGithub } from "./Icons";
import { packages, RELEASE_PAGE, RELEASE_VERSION, REPO } from "../data/site";

const ASSET_BASE = `${REPO}/releases/download/v${RELEASE_VERSION}`;

export function Downloads() {
  const [active, setActive] = useState(packages[0].id);
  const baseId = useId();

  return (
    <section className="section" id="downloads" aria-labelledby="dl-title">
      <div className="shell">
        <Reveal className="head">
          <span className="eyebrow">Downloads</span>
          <h2 className="head__title" id="dl-title">
            Pick the package built for your system.
          </h2>
          <p className="lead">
            Every artifact below is the real file published in the v{RELEASE_VERSION} release. Debian and Ubuntu
            packages are compiled against different Qt versions — use the one for your distribution.
          </p>
        </Reveal>

        <Reveal>
          <div className="tabs" role="tablist" aria-label="Package formats">
            {packages.map((p) => (
              <button
                key={p.id}
                role="tab"
                id={`${baseId}-tab-${p.id}`}
                aria-selected={active === p.id}
                aria-controls={`${baseId}-panel-${p.id}`}
                className="tab"
                type="button"
                onClick={() => setActive(p.id)}
                tabIndex={active === p.id ? 0 : -1}
                onKeyDown={(e) => {
                  const idx = packages.findIndex((x) => x.id === active);
                  if (e.key === "ArrowRight" || e.key === "ArrowDown") {
                    e.preventDefault();
                    setActive(packages[(idx + 1) % packages.length].id);
                  } else if (e.key === "ArrowLeft" || e.key === "ArrowUp") {
                    e.preventDefault();
                    setActive(packages[(idx - 1 + packages.length) % packages.length].id);
                  }
                }}
              >
                {p.label}
              </button>
            ))}
          </div>

          {packages.map((p) => (
            <div
              key={p.id}
              role="tabpanel"
              id={`${baseId}-panel-${p.id}`}
              aria-labelledby={`${baseId}-tab-${p.id}`}
              hidden={active !== p.id}
            >
              <div className="dl__card" style={{ maxWidth: "62rem" }}>
                <div className="dl__head">
                  <h3>{p.title}</h3>
                  <span className="dl__meta">{p.meta}</span>
                </div>
                <p className="dl__desc">{p.desc}</p>

                <div className="dl__files">
                  {p.files.map((f) => (
                    <div className="dl__file" key={f.name}>
                      <code>{f.name}</code>
                      <a href={f.url} download>
                        Download ↓
                      </a>
                    </div>
                  ))}
                  {p.id !== "appimage" && (
                    <div className="dl__file">
                      <code>SHA256SUMS</code>
                      <a href={`${ASSET_BASE}/SHA256SUMS`} download>
                        Checksums ↓
                      </a>
                    </div>
                  )}
                </div>

                {p.note && <p className="dl__note">ⓘ {p.note}</p>}
              </div>
            </div>
          ))}

          <div className="dl__foot">
            <div>
              <a className="btn btn--primary" href={RELEASE_PAGE} target="_blank" rel="noreferrer noopener">
                <IconDownload /> All downloads · v{RELEASE_VERSION}
              </a>
              <p className="dl__note" style={{ marginTop: "0.7rem" }}>
                Source archive:{" "}
                <a href={`${ASSET_BASE}/excalibur-control-center-${RELEASE_VERSION}.tar.gz`}>
                  excalibur-control-center-{RELEASE_VERSION}.tar.gz
                </a>
              </p>
            </div>
            <a className="mono" href={REPO} target="_blank" rel="noreferrer noopener">
              <IconGithub /> {REPO.replace("https://", "")} <IconArrow style={{ verticalAlign: "-2px" }} />
            </a>
          </div>
        </Reveal>
      </div>
    </section>
  );
}
