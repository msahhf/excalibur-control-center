import { useEffect, useState } from "react";
import { BrandMark } from "./Brand";
import { LanguageSwitcher } from "./LanguageSwitcher";
import { IconGithub, IconDownload } from "./Icons";
import { useI18n } from "../i18n";
import { useScrolled } from "../hooks/useEnvironment";
import { NAV_IDS, RELEASE_PAGE, RELEASE_VERSION, REPO } from "../data/site";

/** Scroll-spy: mark the section currently in view for the nav highlight. */
function useActiveSection(ids: readonly string[]) {
  const [active, setActive] = useState<string | null>(null);
  useEffect(() => {
    const els = ids
      .map((id) => document.getElementById(id))
      .filter((el): el is HTMLElement => el !== null);
    if (!els.length) return;
    const io = new IntersectionObserver(
      (entries) => {
        const visible = entries
          .filter((e) => e.isIntersecting)
          .sort((a, b) => b.intersectionRatio - a.intersectionRatio)[0];
        if (visible?.target.id) setActive(visible.target.id);
      },
      { rootMargin: "-45% 0px -45% 0px", threshold: [0, 0.25, 0.5, 1] },
    );
    els.forEach((el) => io.observe(el));
    return () => io.disconnect();
  }, [ids]);
  return active;
}

export function Nav() {
  const { t } = useI18n();
  const solid = useScrolled(20);
  const [open, setOpen] = useState(false);
  const active = useActiveSection(NAV_IDS);

  useEffect(() => {
    const onResize = () => {
      if (window.innerWidth >= 1000) setOpen(false);
    };
    window.addEventListener("resize", onResize);
    return () => window.removeEventListener("resize", onResize);
  }, []);

  return (
    <header className={["nav", solid ? "is-solid" : "", open ? "is-open" : ""].filter(Boolean).join(" ")}>
      <div className="shell nav__inner">
        <a href="#top" className="brand" aria-label={t.nav.home}>
          <BrandMark className="brand__mark" />
          <span className="brand__word" translate="no">
            EXCALIBUR<small>CONTROL CENTER</small>
          </span>
        </a>

        <nav className="nav__links" aria-label={t.nav.primary}>
          {NAV_IDS.map((id) => (
            <a
              key={id}
              className="nav__link"
              href={`#${id}`}
              aria-current={active === id ? "true" : undefined}
            >
              {t.nav[id]}
            </a>
          ))}
        </nav>

        <div className="nav__tools">
          <LanguageSwitcher />
          <a className="btn" href={REPO} target="_blank" rel="noreferrer noopener">
            <IconGithub /> {t.nav.github}
          </a>
          <a className="btn btn--primary" href={RELEASE_PAGE} target="_blank" rel="noreferrer noopener">
            <IconDownload /> {t.nav.get} v{RELEASE_VERSION}
          </a>
        </div>

        <button
          className="nav__burger"
          type="button"
          aria-expanded={open}
          aria-controls="mobile-menu"
          aria-label={open ? t.nav.closeMenu : t.nav.openMenu}
          onClick={() => setOpen((v) => !v)}
        >
          <span />
          <span />
          <span />
        </button>
      </div>

      <div className="nav__panel" id="mobile-menu">
        {NAV_IDS.map((id) => (
          <a key={id} href={`#${id}`} onClick={() => setOpen(false)}>
            {t.nav[id]}
          </a>
        ))}
        <a href={REPO} target="_blank" rel="noreferrer noopener" onClick={() => setOpen(false)}>
          {t.nav.repoLink} ↗
        </a>
        <a href={RELEASE_PAGE} target="_blank" rel="noreferrer noopener" onClick={() => setOpen(false)}>
          {t.nav.get} v{RELEASE_VERSION} ↗
        </a>
        <LanguageSwitcher />
      </div>
    </header>
  );
}
