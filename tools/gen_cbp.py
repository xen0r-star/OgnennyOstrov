import pathlib

root = pathlib.Path(__file__).resolve().parent.parent
tpl = (root / "tools/Goulag.cbp.in").read_text(encoding="utf-8")

CLIENT = ["Client_Debug", "Client_Release"]
SERVER = ["Server_Debug", "Server_Release"]
ALL = CLIENT + SERVER

def targets_for(rel: str):
    """Même logique que les GLOB du CMakeLists."""
    if rel.startswith(("src/models/", "src/network/", "src/utils/")):
        return ALL
    if rel.startswith(("src/views/", "src/controllers/client/")):
        return CLIENT
    if rel.startswith("src/controllers/server/"):
        return SERVER
    if rel in ("src/core/Game.cpp", "src/main.cpp"):
        return CLIENT
    if rel in ("src/core/GameServer.cpp", "src/main_server.cpp"):
        return SERVER
    return None

units = []
for p in sorted((root / "src").rglob("*")):
    if p.suffix not in {".cpp", ".h", ".hpp"}:
        continue
    rel = p.relative_to(root).as_posix()

    if p.suffix == ".cpp":
        tg = targets_for(rel)
        if tg is None:
            print(f"[!] ignoré (pas dans CMake) : {rel}")
            continue
        opts = "\n".join(f'      <Option target="{t}" />' for t in tg)
        units.append(
            f'    <Unit filename="{rel}">\n'
            f'      <Option compilerVar="CPP" />\n{opts}\n'
            f'    </Unit>'
        )
    else:
        units.append(f'    <Unit filename="{rel}" />')

(root / "Goulag.cbp").write_text(tpl.replace("@@UNITS@@", "\n".join(units)), encoding="utf-8")
print(f"Goulag.cbp généré ({len(units)} fichiers)")