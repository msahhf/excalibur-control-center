import { SheetHead } from "./SheetHead";
import { useI18n } from "../i18n";
import type { CSSProperties } from "react";

export function ReadOnly() {
  const { t } = useI18n();
  const d = t.readonly.diagram;
  const headId = "readonly-title";

  const segments = [
    { x1: 180, x2: 250, len: 70 },
    { x1: 440, x2: 510, len: 70 },
    { x1: 680, x2: 750, len: 70 },
  ];

  return (
    <section className="section" id="readonly" aria-labelledby={headId}>
      <div className="shell">
        <SheetHead name={t.nav.readonly} index="02 / 06" />

        <div className="head">
          <h2 className="head__title" id={headId}>
            {t.readonly.heading}
          </h2>
          <p className="lead">{t.readonly.lead}</p>
        </div>

        <ul className="ro-msgs ro-msgs--row">
          {t.readonly.messages.map((m) => (
            <li key={m.title}>
              <h3>{m.title}</h3>
              <p>{m.body}</p>
            </li>
          ))}
        </ul>

        <figure className="ro-diagram" style={{ margin: "clamp(2rem, 4vw, 3rem) 0 0" }}>
          <svg
            className="pipeline"
            viewBox="0 0 1000 230"
            role="img"
            aria-labelledby="ro-diagram-cap"
            preserveAspectRatio="xMidYMid meet"
          >
            <desc id="ro-diagram-cap">{d.caption}</desc>

            {/* One-way read wires. */}
            {segments.map((seg, i) => (
              <g key={i}>
                <line className="p-wire" x1={seg.x1} y1="110" x2={seg.x2} y2="110" />
                <polygon
                  className="p-arrow"
                  points={`${seg.x2},110 ${seg.x2 - 9},105 ${seg.x2 - 9},115`}
                />
                <g className="p-flow" style={{ "--len": `${seg.len}px` } as CSSProperties}>
                  <circle className="p-pulse" cx={seg.x1 + 6} cy="110" r="3.4" />
                </g>
              </g>
            ))}

            {/* Stages. */}
            <g>
              <rect className="p-node" x="20" y="80" width="160" height="60" rx="6" />
              <text className="p-label" x="36" y="107">
                {d.sensors}
              </text>
              <text className="p-sub" x="36" y="124">
                {d.sensorsSub}
              </text>
            </g>
            <g>
              <rect className="p-node p-node--end" x="250" y="80" width="190" height="60" rx="6" />
              <text className="p-label" x="266" y="107">
                {d.driver}
              </text>
              <text className="p-sub" x="266" y="124">
                {d.driverSub}
              </text>
            </g>
            <g>
              <rect className="p-node" x="510" y="80" width="170" height="60" rx="6" />
              <text className="p-label" x="526" y="107">
                {d.hwmon}
              </text>
              <text className="p-sub" x="526" y="124">
                {d.hwmonSub}
              </text>
            </g>
            <g>
              <rect className="p-node p-node--end" x="750" y="80" width="170" height="60" rx="6" />
              <text className="p-label" x="766" y="107">
                {d.app}
              </text>
              <text className="p-sub" x="766" y="124">
                {d.appSub}
              </text>
            </g>

            {/* The read-only boundary sits around hwmon + application. */}
            <rect className="p-boundary" x="494" y="58" width="444" height="104" rx="10" />
            <text className="p-boundary-label" x="494" y="46">
              {d.boundary}
            </text>

            <text className="p-sub" x="20" y="186" fill="var(--ink-3)">
              {d.caption}
            </text>
            <text className="p-sub" x="20" y="206" fill="var(--signal)">
              → {d.direction}
            </text>
          </svg>
        </figure>
      </div>
    </section>
  );
}
