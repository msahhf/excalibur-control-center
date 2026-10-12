import { useLightbox } from "./Lightbox";

type Props = {
  src: string;
  alt: string;
  title: string;
  /** small caption shown at the right of the frame bar */
  app?: string;
  className?: string;
  /** when true, the inner image is a real screenshot button (lightbox) */
  zoomable?: boolean;
  eager?: boolean;
};

/**
 * A screenshot presented as the real product: a thin title bar + the untouched
 * application bitmap. No CSS re-drawing of the UI, no invented sensor values.
 */
export function ScreenshotFrame({
  src,
  alt,
  title,
  app = "excalibur-control-center",
  className,
  zoomable = true,
  eager = false,
}: Props) {
  const { open } = useLightbox();
  const img = (
    <img
      className="frame__img"
      src={src}
      alt={alt}
      loading={eager ? "eager" : "lazy"}
      decoding="async"
      width={1280}
      height={720}
    />
  );

  return (
    <figure className={["frame", className].filter(Boolean).join(" ")} style={{ margin: 0 }}>
      <div className="frame__bar">
        <span className="frame__dots" aria-hidden="true">
          <i />
          <i />
          <i />
        </span>
        <span className="frame__title">{title}</span>
        <span className="frame__app">{app}</span>
      </div>
      {zoomable ? (
        <button
          type="button"
          className="shot"
          onClick={() => open({ src, title })}
          aria-label={`Enlarge screenshot: ${title}`}
        >
          {img}
        </button>
      ) : (
        img
      )}
    </figure>
  );
}
