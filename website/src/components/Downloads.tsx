import { useId, useRef, useState, type CSSProperties, type KeyboardEvent } from "react";
import { SheetHead } from "./SheetHead";
import { IconArrow, IconDownload } from "./Icons";
import { ScreenshotFrame } from "./ScreenshotFrame";
import { useI18n, interpolate } from "../i18n";
import {
  APPIMAGE_NODRIVER_IMAGE,
  packageFiles,
  packageMeta,
  packageOrder,
  pickerLabels,
  RELEASE_PAGE,
  RELEASE_VERSION,
  REPO,
  SOURCE_ARCHIVE,
  type PackageId,
} from "../data/site";

function formatSize(bytes: number): string {
  if (bytes >= 1024 * 1024) return `${(bytes / 1024 / 1024).toFixed(1)} MB`;
  if (bytes >= 1024) return `${Math.round(bytes / 1024)} KB`;
  return `${bytes} B`;
}

export function Downloads() {
  const { t } = useI18n();
  const [activeId, setActiveId] = useState<PackageId>("arch");
  const baseId = useId();
  const tabRefs = useRef<Partial<Record<PackageId, HTMLButtonElement | null>>>({});
  const activeIndex = packageOrder.indexOf(activeId);

  const select = (id: PackageId, focus = true) => {
    setActiveId(id);
    if (focus) tabRefs.current[id]?.focus();
  };

  const onKeyDown = (e: KeyboardEvent) => {
    const dir = e.key === "ArrowRight" || e.key === "ArrowDown" ? 1 : e.key === "ArrowLeft" || e.key === "ArrowUp" ? -1 : 0;
    if (!dir) return;
    e.preventDefault();
    const next = (activeIndex + dir + packageOrder.length) % packageOrder.length;
    select(packageOrder[next]);
  };

  const pkg = t.downloads.packages[activeId];
  const meta = packageMeta[activeId];
  const files = packageFiles[activeId];
  const primary = files[0];

  return (
    <section className="section" id="download" aria-labelledby="download-title">
      <div className="shell">
        <SheetHead name={t.nav.download} index="05 / 06" />
        <div className="head">
          <h2 className="head__title" id="download-title">
            {t.downloads.heading}
          </h2>
          <p className="lead">{interpolate(t.downloads.lead, { version: RELEASE_VERSION })}</p>
        </div>

        <div className="picker" style={{ "--n": packageOrder.length } as CSSProperties}>
          <span
            className="picker__ind"
            aria-hidden="true"
            style={{ "--i": activeIndex } as CSSProperties}
          />
          <div className="picker__row" role="tablist" aria-label={t.downloads.selectorLabel} onKeyDown={onKeyDown}>
            {packageOrder.map((id) => (
              <button
                key={id}
                ref={(el) => {
                  tabRefs.current[id] = el;
                }}
                type="button"
                role="tab"
                id={`${baseId}-tab-${id}`}
                aria-selected={activeId === id}
                aria-controls={`${baseId}-panel`}
                tabIndex={activeId === id ? 0 : -1}
                className="picker__btn"
                onClick={() => select(id, false)}
              >
                {pickerLabels[id]}
              </button>
            ))}
          </div>
        </div>

        <div
          key={activeId}
          id={`${baseId}-panel`}
          role="tabpanel"
          aria-labelledby={`${baseId}-tab-${activeId}`}
          className="pkg"
        >
          <div className="pkg__id">
            <h3 className="pkg__title">{pkg.title}</h3>

            <div className="pkg__chips">
              <span className="chip" translate="no">
                {meta.format}
              </span>
              <span className="chip" translate="no">
                {meta.arch}
              </span>
              <span className="chip">{interpolate(t.downloads.fileCount, { count: files.length })}</span>
            </div>

            <p className="pkg__desc">{pkg.desc}</p>

            <a className="btn btn--primary btn--lg pkg__primary" href={primary.url} download>
              <IconDownload /> {t.downloads.download}
            </a>
            <p className="pkg__primary-meta tnum" translate="no">
              v{RELEASE_VERSION} · {meta.format} · {meta.arch}
            </p>

            <div className="pkg__links">
              <a href={RELEASE_PAGE} target="_blank" rel="noreferrer noopener">
                {t.downloads.all} · v{RELEASE_VERSION}
              </a>
              <a href={SOURCE_ARCHIVE} className={activeId === "appimage" ? "pkg__file-link" : undefined}>
                {t.downloads.sourceArchive}
              </a>
              <a href={REPO} target="_blank" rel="noreferrer noopener">
                {t.nav.repoLink} <IconArrow style={{ verticalAlign: "-2px", width: "0.85em" }} />
              </a>
            </div>
          </div>

          <div className="pkg__files">
            <span className="pkg__files-label">{t.downloads.files}</span>
            {files.map((f) => (
              <div className="pkg__file" key={f.name}>
                <span className="pkg__file-name">
                  <code translate="no">{f.name}</code>
                  {typeof f.size === "number" && (
                    <span className="pkg__file-size tnum">{formatSize(f.size)}</span>
                  )}
                </span>
                <a className="pkg__file-link" href={f.url} download>
                  {f.name === "SHA256SUMS" ? t.downloads.checksums : t.downloads.download}
                  <IconDownload />
                </a>
              </div>
            ))}
            {pkg.note && <p className="pkg__note">ⓘ {pkg.note}</p>}
            {activeId === "appimage" && (
              <ScreenshotFrame
                src={APPIMAGE_NODRIVER_IMAGE}
                alt={t.downloads.appimageNoDriver}
                title={pkg.title}
                eager
              />
            )}
            {activeId === "appimage" && <p className="pkg__note">{t.downloads.appimageNoDriver}</p>}
          </div>
        </div>
      </div>
    </section>
  );
}
