/**
 * A datasheet-style section header: a hairline rule carrying the section name
 * and its position in the page. Structure, not decoration — used once per
 * section instead of a decorative label above every heading.
 */
export function SheetHead({ name, index }: { name: string; index: string }) {
  return (
    <div className="sheet-head">
      <span className="sheet-head__name">{name}</span>
      <span className="sheet-head__index tnum">{index}</span>
    </div>
  );
}
