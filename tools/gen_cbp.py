import pathlib

root = pathlib.Path(__file__).resolve().parent.parent
tpl = (root / "tools/Goulag.cbp.in").read_text(encoding="utf-8")

units = []
for p in sorted((root / "src").rglob("*")):
    if p.suffix not in {".cpp", ".h", ".hpp"}:
        continue
    rel = p.relative_to(root).as_posix()
    if p.suffix == ".cpp":
        units.append(f'    <Unit filename="{rel}">\n      <Option compilerVar="CPP" />\n    </Unit>')
    else:
        units.append(f'    <Unit filename="{rel}" />')

(root / "Goulag.cbp").write_text(tpl.replace("@@UNITS@@", "\n".join(units)), encoding="utf-8")
print(f"Goulag.cbp généré ({len(units)} fichiers)")