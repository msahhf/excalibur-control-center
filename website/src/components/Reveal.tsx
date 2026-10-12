import type { ElementType, ReactNode } from "react";
import { useReveal } from "../hooks/useReveal";

type RevealProps = {
  children: ReactNode;
  as?: ElementType;
  delay?: number;
  className?: string;
};

/** Wraps content so it fades/rises in once, when it scrolls into view. */
export function Reveal({ children, as: Tag = "div", delay = 0, className }: RevealProps) {
  const ref = useReveal<HTMLElement>();
  return (
    <Tag
      ref={ref}
      className={["reveal", className].filter(Boolean).join(" ")}
      style={delay ? ({ "--reveal-delay": `${delay}ms` } as Record<string, string>) : undefined}
    >
      {children}
    </Tag>
  );
}
