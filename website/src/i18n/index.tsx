import {
  createContext,
  useContext,
  useEffect,
  useMemo,
  useState,
  type ReactNode,
} from "react";
import { en, type Dict } from "./en";
import { tr } from "./tr";
import { de } from "./de";

export type Locale = "en" | "tr" | "de";

export const LOCALES: { id: Locale; label: string }[] = [
  { id: "en", label: "English" },
  { id: "tr", label: "Türkçe" },
  { id: "de", label: "Deutsch" },
];

export const DICTS: Record<Locale, Dict> = { en, tr, de };
const SUPPORTED: Locale[] = ["en", "tr", "de"];
const STORAGE_KEY = "ecc.locale";

/** Saved choice → browser preference → English fallback. */
function detectLocale(): Locale {
  if (typeof window === "undefined") return "en";
  try {
    const saved = window.localStorage.getItem(STORAGE_KEY);
    if (saved && SUPPORTED.includes(saved as Locale)) return saved as Locale;
  } catch {
    /* storage unavailable */
  }
  const languages = navigator.languages ?? [navigator.language];
  for (const l of languages) {
    const base = (l || "").toLowerCase().split("-")[0];
    if (SUPPORTED.includes(base as Locale)) return base as Locale;
  }
  return "en";
}

function setMeta(attr: "name" | "property", key: string, content: string) {
  let el = document.head.querySelector<HTMLMetaElement>(`meta[${attr}="${key}"]`);
  if (!el) {
    el = document.createElement("meta");
    el.setAttribute(attr, key);
    document.head.appendChild(el);
  }
  el.setAttribute("content", content);
}

type Ctx = { locale: Locale; setLocale: (l: Locale) => void; t: Dict };

const I18nCtx = createContext<Ctx | null>(null);

export function I18nProvider({ children }: { children: ReactNode }) {
  const [locale, setLocaleState] = useState<Locale>(() => detectLocale());
  const t = DICTS[locale];

  const setLocale = (l: Locale) => {
    setLocaleState(l);
    try {
      window.localStorage.setItem(STORAGE_KEY, l);
    } catch {
      /* ignore */
    }
  };

  useEffect(() => {
    document.documentElement.lang = locale;
    document.title = t.meta.title;
    setMeta("name", "description", t.meta.description);
    setMeta("property", "og:title", t.meta.ogTitle);
    setMeta("property", "og:description", t.meta.ogDescription);
    setMeta("name", "twitter:title", t.meta.ogTitle);
    setMeta("name", "twitter:description", t.meta.ogDescription);
  }, [locale, t]);

  const value = useMemo<Ctx>(() => ({ locale, setLocale, t }), [locale, t]);
  return <I18nCtx.Provider value={value}>{children}</I18nCtx.Provider>;
}

export function useI18n(): Ctx {
  const c = useContext(I18nCtx);
  if (!c) throw new Error("useI18n must be used inside <I18nProvider>");
  return c;
}

/** Replaces {name} tokens. */
export function interpolate(template: string, vars: Record<string, string | number>): string {
  return template.replace(/\{(\w+)\}/g, (_, k) => String(vars[k] ?? `{${k}}`));
}

/** Leaf key paths of a dictionary — used to assert translation completeness. */
export function flattenKeys(obj: unknown, prefix = ""): string[] {
  if (obj === null || typeof obj !== "object") return [prefix];
  if (Array.isArray(obj)) {
    return obj.flatMap((v, i) => flattenKeys(v, `${prefix}[${i}]`));
  }
  return Object.entries(obj).flatMap(([k, v]) =>
    flattenKeys(v, prefix ? `${prefix}.${k}` : k),
  );
}
