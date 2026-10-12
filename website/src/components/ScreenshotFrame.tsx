import { useLightbox } from "./Lightbox";
import { useI18n } from "../i18n";

type Props = {
  src: string;
  alt: string;
  title: string;
  /** right-hand caption; defaults to the executable name */
  app?: string;
  className?: string;
  zoomable?: boolean;
  eager?: boolean;
};

/**
 * The real application screenshot, framed — never re-drawn in CSS, never given
 * invented values. Clicking opens the full-size lightbox.
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
  const { t } = useI18n();

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
        <span className="frame__title">{title}</span>
        <span className="frame__app" translate="no">
          {app}
        </span>
      </div>
      {zoomable ? (
        <button
          type="button"
          className="shot"
          onClick={() => open({ src, title })}
          aria-label={`${t.a11y.enlarge}: ${title}`}
        >
          {img}
        </button>
      ) : (
        img
      )}
    </figure>
  );
}
