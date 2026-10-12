import { SheetHead } from "./SheetHead";
import { useI18n } from "../i18n";
import type { CSSProperties } from "react";

export function ReadOnly() {
  const { t } = useI18n();
  const d = t.readonly.diagram;
  const headId = "readonly-title";

  const X = [76, 332, 588, 844];
  const W = 200;
  const H = 74;
  const Y = 88;
  const CY = Y + H / 2;

  const nodes = [
    { x: X[0], accent: false, label: d.sensors, sub: d.sensorsSub },
    { x: X[1], accent: false, label: d.driver, sub: d.driverSub },
    { x: X[2], accent: true, label: d.hwmon, sub: d.hwmonSub },
    { x: X[3], accent: true, label: d.app, sub: d.appSub },
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

        <figure className="ro-diagram">
          {/* Blueprint surface: faint blueprint grid + corner registration marks */}
          <span className="ro-diagram__grid" aria-hidden="true" />
          <span className="ro-diagram__corner ro-diagram__corner--tl" aria-hidden="true" />
          <span className="ro-diagram__corner ro-diagram__corner--tr" aria-hidden="true" />
          <span className="ro-diagram__corner ro-diagram__corner--bl" aria-hidden="true" />
          <span className="ro-diagram__corner ro-diagram__corner--br" aria-hidden="true" />

          <svg
            className="pipeline"
            viewBox="0 0 1120 272"
            role="img"
            aria-labelledby="ro-diagram-cap"
            preserveAspectRatio="xMidYMid meet"
          >
            <desc id="ro-diagram-cap">{d.caption}</desc>

            {/* One-way read wires with travelling telemetry pulses. */}
            {nodes.slice(0, -1).map((n, i) => {
              const x1 = n.x + W;
              const x2 = nodes[i + 1].x;
              return (
                <g key={i}>
                  <line className="p-wire" x1={x1} y1={CY} x2={x2} y2={CY} />
                  <polygon
                    className="p-arrow"
                    points={`${x2 - 2},${CY} ${x2 - 12},${CY - 5} ${x2 - 12},${CY + 5}`}
                  />
                  <g className="p-flow" style={{ "--len": `${x2 - x1}px` } as CSSProperties}>
                    <circle className="p-pulse" cx={x1 + 5} cy={CY} r="3.2" />
                  </g>
                </g>
              );
            })}

            {/* Stages. */}
            {nodes.map((n, i) => (
              <g key={i} className={n.accent ? "p-stage p-stage--in" : "p-stage"}>
                <rect className="p-node" x={n.x} y={Y} width={W} height={H} rx="8" />
                <rect
                  className={n.accent ? "p-edge p-edge--accent" : "p-edge"}
                  x={n.x}
                  y={Y + 14}
                  width="3"
                  height={H - 28}
                  rx="1.5"
                />
                <text className="p-index" x={n.x + 16} y={Y + 22}>
                  {String(i + 1).padStart(2, "0")}
                </text>
                <text className="p-label" x={n.x + 16} y={Y + 44}>
                  {n.label}
                </text>
                <text className="p-sub" x={n.x + 16} y={Y + 62}>
                  {n.sub}
                </text>
              </g>
            ))}

            {/* The read-only boundary brackets hwmon + application. */}
            <rect className="p-boundary" x={X[2] - 14} y={Y - 18} width={W * 2 + 28 + (X[3] - X[2] - W)} height={H + 36} rx="12" />
            <text className="p-boundary-label" x={X[2] - 14} y={Y - 28}>
              {d.boundary}
            </text>

            {/* Direction caption. */}
            <text className="p-caption" x="76" y="234">
              {d.caption}
            </text>
            <text className="p-direction" x="76" y="258">
              → {d.direction}
            </text>
          </svg>
        </figure>
      </div>
    </section>
  );
}
