/** EXCALIBUR Control Center symbol: a calm chevron/signal mark, not a copy of
 *  any CASPER branding. Used in the nav and footer. */
export function BrandMark({ className }: { className?: string }) {
  return (
    <svg
      className={className}
      viewBox="0 0 48 48"
      role="img"
      aria-label="EXCALIBUR Control Center"
    >
      <defs>
        <linearGradient id="bm" x1="0" y1="0" x2="1" y2="1">
          <stop offset="0" stopColor="#5b9bff" />
          <stop offset="1" stopColor="#22d3ee" />
        </linearGradient>
      </defs>
      <rect x="2" y="2" width="44" height="44" rx="12" fill="#0b0f18" stroke="url(#bm)" strokeWidth="1.6" />
      <path d="M24 9 L31 20 H27 V31 H21 V20 H17 Z" fill="url(#bm)" />
      <path d="M15 34 H33" stroke="url(#bm)" strokeWidth="2.4" strokeLinecap="round" />
      <circle cx="24" cy="39.5" r="2.1" fill="url(#bm)" />
    </svg>
  );
}
