import type { SVGProps } from "react";

// Minimal, consistent line icons (no icon library dependency).
const base = {
  width: 20,
  height: 20,
  viewBox: "0 0 24 24",
  fill: "none",
  stroke: "currentColor",
  strokeWidth: 1.6,
  strokeLinecap: "round" as const,
  strokeLinejoin: "round" as const,
  "aria-hidden": true,
};

export const IconThermo = (p: SVGProps<SVGSVGElement>) => (
  <svg {...base} {...p}>
    <path d="M14 14.76V5a2 2 0 1 0-4 0v9.76a4 4 0 1 0 4 0Z" />
  </svg>
);

export const IconFan = (p: SVGProps<SVGSVGElement>) => (
  <svg {...base} {...p}>
    <circle cx="12" cy="12" r="2" />
    <path d="M12 10c1.5-2.5 1-5-1-6.5-1.5 2-1.5 4.5 1 6.5Z" />
    <path d="M14 12c2.5 1.5 5 1 6.5-1-2-1.5-4.5-1.5-6.5 1Z" />
    <path d="M12 14c-1.5 2.5-1 5 1 6.5 1.5-2 1.5-4.5-1-6.5Z" />
    <path d="M10 12c-2.5-1.5-5-1-6.5 1 2 1.5 4.5 1.5 6.5-1Z" />
  </svg>
);

export const IconChip = (p: SVGProps<SVGSVGElement>) => (
  <svg {...base} {...p}>
    <rect x="7" y="7" width="10" height="10" rx="2" />
    <path d="M10 3v2M14 3v2M10 19v2M14 19v2M3 10h2M3 14h2M19 10h2M19 14h2" />
  </svg>
);

export const IconPulse = (p: SVGProps<SVGSVGElement>) => (
  <svg {...base} {...p}>
    <path d="M3 12h4l2-6 4 12 2-6h6" />
  </svg>
);

export const IconShield = (p: SVGProps<SVGSVGElement>) => (
  <svg {...base} {...p}>
    <path d="M12 3 5 6v5c0 4.5 3 8 7 10 4-2 7-5.5 7-10V6l-7-3Z" />
  </svg>
);

export const IconLock = (p: SVGProps<SVGSVGElement>) => (
  <svg {...base} {...p}>
    <rect x="5" y="11" width="14" height="9" rx="2" />
    <path d="M8 11V8a4 4 0 0 1 8 0v3" />
  </svg>
);

export const IconDoc = (p: SVGProps<SVGSVGElement>) => (
  <svg {...base} {...p}>
    <path d="M7 3h7l4 4v14a1 1 0 0 1-1 1H7a1 1 0 0 1-1-1V4a1 1 0 0 1 1-1Z" />
    <path d="M14 3v4h4M9 13h6M9 17h4" />
  </svg>
);

export const IconDownload = (p: SVGProps<SVGSVGElement>) => (
  <svg {...base} {...p}>
    <path d="M12 4v11m0 0 4-4m-4 4-4-4" />
    <path d="M5 19h14" />
  </svg>
);

export const IconArrow = (p: SVGProps<SVGSVGElement>) => (
  <svg {...base} {...p}>
    <path d="M5 12h14M13 6l6 6-6 6" />
  </svg>
);

export const IconGithub = (p: SVGProps<SVGSVGElement>) => (
  <svg {...base} {...p}>
    <path d="M9 19c-4 1.2-4-2-6-2.5M15 21v-3.6c0-1 .1-1.5-.5-2.1 2.8-.3 5.5-1.4 5.5-6a4.6 4.6 0 0 0-1.3-3.2 4.2 4.2 0 0 0-.1-3.2s-1.1-.3-3.6 1.3a12.3 12.3 0 0 0-6.4 0C6.1 1.9 5 2.2 5 2.2a4.2 4.2 0 0 0-.1 3.2A4.6 4.6 0 0 0 3.6 8.6c0 4.6 2.7 5.7 5.5 6-.6.6-.6 1.2-.5 2.1V21" />
  </svg>
);

export const IconCheck = (p: SVGProps<SVGSVGElement>) => (
  <svg {...base} {...p}>
    <path d="M20 6 9 17l-5-5" />
  </svg>
);

export const IconNoWrite = (p: SVGProps<SVGSVGElement>) => (
  <svg {...base} {...p}>
    <path d="M12 3v7" />
    <path d="m4 13 8 4 8-4" />
    <path d="M4 17l8 4 8-4" />
    <path d="m3 3 18 18" stroke="var(--danger)" />
  </svg>
);
