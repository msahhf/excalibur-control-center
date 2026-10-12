import { LOCALES, useI18n, type Locale } from "../i18n";

/** A compact, keyboard-operable language switcher (native buttons + aria-pressed). */
export function LanguageSwitcher({ className }: { className?: string }) {
  const { locale, setLocale, t } = useI18n();
  return (
    <div className={["lang", className].filter(Boolean).join(" ")} role="group" aria-label={t.nav.language}>
      {LOCALES.map((l) => (
        <button
          key={l.id}
          type="button"
          className="lang__btn"
          aria-pressed={locale === l.id}
          aria-label={l.label}
          title={l.label}
          lang={l.id}
          onClick={() => setLocale(l.id as Locale)}
        >
          {l.id.toUpperCase()}
        </button>
      ))}
    </div>
  );
}
