import { Reveal } from "./Reveal";
import { IconLock, IconNoWrite, IconShield } from "./Icons";

export function ReadOnly() {
  return (
    <section className="section" id="readonly" aria-labelledby="readonly-title">
      <div className="shell">
        <div className="split">
          <Reveal>
            <span className="eyebrow">Read-only by design</span>
            <h2 className="head__title" id="readonly-title">
              Visibility without taking control.
            </h2>
            <div className="prose" style={{ marginTop: "1.1rem" }}>
              <p>
                EXCALIBUR Control Center <strong>reads</strong> hardware telemetry and nothing else. It does not
                set fan speeds, power limits, performance modes or RGB — that capability simply does not exist
                in the app.
              </p>
              <p>
                The GUI never talks to WMI or the embedded controller directly. A single read boundary reads the
                kernel's read-only hwmon files, and every screen is fed from that one state.
              </p>
            </div>
            <ul className="points" style={{ marginTop: "1.6rem" }}>
              <li>
                <IconLock />
                <span>Runs entirely as an unprivileged user; the GUI never needs root.</span>
              </li>
              <li>
                <IconShield />
                <span>Temperature, fan RPM, hardware identity and diagnostics — all disclosed.</span>
              </li>
              <li>
                <IconNoWrite />
                <span>No write path to fans, power or thermal limits is implemented or promised.</span>
              </li>
            </ul>
          </Reveal>

          <Reveal delay={120}>
            <figure style={{ margin: 0 }} aria-label="Read-only data flow diagram">
              <svg className="diagram" viewBox="0 0 460 300" role="img" aria-labelledby="ro-desc">
                <desc id="ro-desc">
                  Telemetry flows one way: hardware sensors to the WMI kernel driver, to read-only hwmon/sysfs,
                  into the Qt6 app. A separate control path is crossed out as not implemented.
                </desc>

                {/* read path */}
                <line className="d-line d-line--hot d-flow" x1="120" y1="70" x2="180" y2="70" />
                <line className="d-line d-line--hot d-flow" x1="300" y1="70" x2="360" y2="70" />
                <line className="d-line d-line--hot d-flow" x1="230" y1="110" x2="230" y2="170" />

                <g>
                  <rect className="d-node" x="20" y="44" width="100" height="52" rx="10" />
                  <text className="d-label" x="34" y="66" fontSize="11">
                    Sensors
                  </text>
                  <text className="d-sub" x="34" y="82" fontSize="9">
                    EC / thermal
                  </text>
                </g>

                <g>
                  <rect className="d-node d-node--accent" x="180" y="44" width="120" height="52" rx="10" />
                  <text className="d-label" x="194" y="66" fontSize="11">
                    excalibur-wmi
                  </text>
                  <text className="d-sub" x="194" y="82" fontSize="9">
                    WMI GET · kernel
                  </text>
                </g>

                <g>
                  <rect className="d-node" x="360" y="44" width="80" height="52" rx="10" />
                  <text className="d-label" x="374" y="66" fontSize="11">
                    hwmon
                  </text>
                  <text className="d-sub" x="374" y="82" fontSize="9">
                    /sys
                  </text>
                </g>

                <g>
                  <rect className="d-node d-node--accent" x="150" y="170" width="160" height="56" rx="10" />
                  <text className="d-label" x="166" y="194" fontSize="11">
                    Qt6 desktop app
                  </text>
                  <text className="d-sub" x="166" y="210" fontSize="9">
                    single read boundary
                  </text>
                </g>

                {/* read-only badge */}
                <g>
                  <rect className="d-badge" x="330" y="180" width="110" height="34" rx="17" />
                  <text x="346" y="201" fontSize="11" fill="#34d399">
                    ✓ read-only
                  </text>
                </g>

                {/* crossed control path */}
                <g opacity="0.8">
                  <line className="d-line" x1="230" y1="206" x2="230" y2="244" strokeDasharray="4 5" />
                  <rect className="d-node" x="150" y="244" width="160" height="30" rx="8" />
                  <text className="d-sub" x="166" y="263" fontSize="10" fill="#fb7185">
                    fan · power · RGB control
                  </text>
                  <line x1="318" y1="240" x2="348" y2="276" stroke="#fb7185" strokeWidth="2" />
                  <line x1="348" y1="240" x2="318" y2="276" stroke="#fb7185" strokeWidth="2" />
                </g>
              </svg>
            </figure>
          </Reveal>
        </div>
      </div>
    </section>
  );
}
