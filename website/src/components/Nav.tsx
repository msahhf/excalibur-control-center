import { useEffect, useState } from "react";
import { BrandMark } from "./Brand";
import { IconGithub, IconDownload } from "./Icons";
import { useScrolled } from "../hooks/useReveal";
import { nav, RELEASE_PAGE, RELEASE_VERSION, REPO } from "../data/site";

export function Nav() {
  const solid = useScrolled(28);
  const [open, setOpen] = useState(false);

  // Close the mobile panel when the viewport grows into the desktop layout.
  useEffect(() => {
    const onResize = () => {
      if (window.innerWidth >= 900) setOpen(false);
    };
    window.addEventListener("resize", onResize);
    return () => window.removeEventListener("resize", onResize);
  }, []);

  return (
    <header className={["nav", solid ? "is-solid" : "", open ? "is-open" : ""].filter(Boolean).join(" ")}>
      <div className="shell nav__inner">
        <a href="#top" className="brand" aria-label="EXCALIBUR Control Center — home">
          <BrandMark className="brand__mark" />
          <span className="brand__text">
            <span className="brand__name">EXCALIBUR</span>
            <span className="brand__sub">CONTROL CENTER</span>
          </span>
        </a>

        <nav className="nav__links" aria-label="Primary">
          {nav.map((n) => (
            <a key={n.href} className="nav__link" href={n.href}>
              {n.label}
            </a>
          ))}
        </nav>

        <div className="nav__actions">
          <a className="btn btn--ghost" href={REPO} target="_blank" rel="noreferrer noopener">
            <IconGithub /> GitHub
          </a>
          <a className="btn btn--primary" href={RELEASE_PAGE} target="_blank" rel="noreferrer noopener">
            <IconDownload /> Get v{RELEASE_VERSION}
          </a>
        </div>

        <button
          className="nav__burger"
          type="button"
          aria-expanded={open}
          aria-controls="mobile-menu"
          aria-label={open ? "Close menu" : "Open menu"}
          onClick={() => setOpen((v) => !v)}
        >
          <span />
          <span />
          <span />
        </button>
      </div>

      <div className="nav__panel" id="mobile-menu">
        {nav.map((n) => (
          <a key={n.href} href={n.href} onClick={() => setOpen(false)}>
            {n.label}
          </a>
        ))}
        <a href={REPO} target="_blank" rel="noreferrer noopener" onClick={() => setOpen(false)}>
          GitHub repository ↗
        </a>
        <a href={RELEASE_PAGE} target="_blank" rel="noreferrer noopener" onClick={() => setOpen(false)}>
          Download v{RELEASE_VERSION} ↗
        </a>
      </div>
    </header>
  );
}
