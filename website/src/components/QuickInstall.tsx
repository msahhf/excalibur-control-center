import { useEffect, useRef, useState } from "react";
import { IconArrow, IconCheck, IconCopy, IconTerminal } from "./Icons";
import { useI18n } from "../i18n";
import {
  INSTALL_COMMAND,
  INSTALL_DISTROS,
  INSTALL_SCRIPT_ASSET,
  INSTALL_SUMS_URL,
  RELEASE_VERSION,
} from "../data/site";

type CopyState = "idle" | "copied" | "error";

/**
 * Clipboard write with a fallback for browsers/contexts where the async
 * Clipboard API is unavailable (older Chromium, non-secure origins).
 */
async function writeClipboard(text: string): Promise<boolean> {
  try {
    if (navigator.clipboard?.writeText) {
      await navigator.clipboard.writeText(text);
      return true;
    }
  } catch {
    /* fall through to the legacy path */
  }
  try {
    const area = document.createElement("textarea");
    area.value = text;
    area.setAttribute("readonly", "");
    area.style.position = "fixed";
    area.style.top = "-1000px";
    area.style.opacity = "0";
    document.body.appendChild(area);
    area.select();
    const copied = document.execCommand("copy");
    document.body.removeChild(area);
    return copied;
  } catch {
    return false;
  }
}

/** Selects the visible command so a failed copy still ends in a manual copy. */
function selectCommand(node: HTMLElement | null) {
  if (!node) return;
  const range = document.createRange();
  range.selectNodeContents(node);
  const selection = window.getSelection();
  selection?.removeAllRanges();
  selection?.addRange(range);
}

/**
 * Quick Install: the one-command installer, shown inline in the Download
 * section. Every fact here (URL, version, supported distributions, privilege
 * model) is read from src/data/site.ts and matches scripts/install.sh in the
 * repository, which is published verbatim as the release's install.sh asset.
 */
export function QuickInstall() {
  const { t } = useI18n();
  const [state, setState] = useState<CopyState>("idle");
  const codeRef = useRef<HTMLElement | null>(null);
  const timer = useRef<number | undefined>(undefined);
  const labels = t.downloads.install;

  useEffect(() => () => window.clearTimeout(timer.current), []);

  const resetLater = () => {
    window.clearTimeout(timer.current);
    timer.current = window.setTimeout(() => setState("idle"), 4000);
  };

  const onCopy = async () => {
    const copied = await writeClipboard(INSTALL_COMMAND);
    setState(copied ? "copied" : "error");
    if (!copied) selectCommand(codeRef.current);
    resetLater();
  };

  return (
    <section className="qi" aria-labelledby="qi-title">
      <div className="qi__intro">
        <p className="qi__badge">
          <IconTerminal />
          {labels.badge}
        </p>
        <h3 className="qi__title" id="qi-title">
          {labels.title}
        </h3>
        <p className="qi__lead">{labels.lead}</p>

        <ul className="qi__distros" aria-label={labels.distrosLabel}>
          {INSTALL_DISTROS.map((d) => (
            <li className="chip" translate="no" key={d}>
              {d}
            </li>
          ))}
        </ul>
      </div>

      <div className="qi__panel">
        <div className="qi__panel-head">
          <span className="qi__panel-label">{labels.commandLabel}</span>
          <span className="qi__panel-meta">
            <span className="qi__ver tnum" translate="no">
              v{RELEASE_VERSION}
            </span>
            <a
              className="qi__script-link"
              href={INSTALL_SCRIPT_ASSET.url}
              target="_blank"
              rel="noreferrer noopener"
            >
              <code translate="no">{INSTALL_SCRIPT_ASSET.name}</code> <IconArrow />
            </a>
          </span>
        </div>

        <pre className="qi__code" aria-label={labels.commandLabel}>
          <code ref={codeRef} translate="no">
            {INSTALL_COMMAND}
          </code>
        </pre>

        <div className="qi__actions">
          <button
            type="button"
            className={`btn btn--primary qi__copy${state === "copied" ? " is-copied" : ""}`}
            onClick={onCopy}
          >
            {state === "copied" ? <IconCheck /> : <IconCopy />}
            {state === "copied" ? labels.copiedButton : labels.copy}
          </button>
          <a className="qi__sums" href={INSTALL_SUMS_URL} target="_blank" rel="noreferrer noopener">
            {labels.sumsLink}
          </a>
        </div>

        <p className="qi__hint">{labels.commandHint}</p>
        <p className={`qi__status${state === "error" ? " is-error" : ""}`} role="status" aria-live="polite">
          {state === "copied" ? labels.copied : state === "error" ? labels.copyError : ""}
        </p>
      </div>

      <ul className="qi__notes">
        {labels.notes.map((note) => (
          <li key={note}>{note}</li>
        ))}
      </ul>
    </section>
  );
}
