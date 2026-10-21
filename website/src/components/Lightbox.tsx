import {
  createContext,
  useCallback,
  useContext,
  useEffect,
  useRef,
  useState,
  type ReactNode,
} from "react";
import { useI18n } from "../i18n";

type Shot = { src: string; title: string };
type LightboxApi = { open: (shot: Shot) => void };

const Ctx = createContext<LightboxApi | null>(null);

export function useLightbox(): LightboxApi {
  const api = useContext(Ctx);
  if (!api) throw new Error("useLightbox must be used inside <LightboxProvider>");
  return api;
}

export function LightboxProvider({ children }: { children: ReactNode }) {
  const { t } = useI18n();
  const [shot, setShot] = useState<Shot | null>(null);
  const closeRef = useRef<HTMLButtonElement | null>(null);
  const lastFocus = useRef<HTMLElement | null>(null);

  const open = useCallback((s: Shot) => {
    lastFocus.current = document.activeElement as HTMLElement | null;
    setShot(s);
  }, []);

  const close = useCallback(() => {
    setShot(null);
    lastFocus.current?.focus?.();
  }, []);

  useEffect(() => {
    if (!shot) return;
    const onKey = (e: KeyboardEvent) => {
      if (e.key === "Escape") close();
    };
    document.addEventListener("keydown", onKey);
    const prevOverflow = document.body.style.overflow;
    document.body.style.overflow = "hidden";
    closeRef.current?.focus();
    return () => {
      document.removeEventListener("keydown", onKey);
      document.body.style.overflow = prevOverflow;
    };
  }, [shot, close]);

  return (
    <Ctx.Provider value={{ open }}>
      {children}
      {shot && (
        <div
          className="lb"
          role="dialog"
          aria-modal="true"
          aria-label={`${shot.title} — ${t.a11y.lightbox}`}
          onClick={close}
        >
          <div className="lb__inner" onClick={(e) => e.stopPropagation()}>
            <div className="lb__bar">
              <span>{shot.title}</span>
              <button ref={closeRef} className="lb__close" type="button" onClick={close}>
                {t.a11y.close} ✕
              </button>
            </div>
            <img className="lb__img" src={shot.src} alt={`${shot.title} — ${t.a11y.lightbox}`} />
          </div>
        </div>
      )}
    </Ctx.Provider>
  );
}
