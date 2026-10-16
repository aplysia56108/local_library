import json
import re
from pathlib import Path

ROOT = ROOT = Path("/home/aplysia/ProCon")
TEMPLATE = ROOT / "library" / "template" / "template.cpp"
SNIPPET = ROOT / ".vscode" / "test.code-snippets"
SNIPPET_NAME = "atcoder_template"

# JSONC の文字列・コメント・記号を認識する
TOKEN = re.compile(
    r'"(?:\\.|[^"\\])*"'
    r'|//[^\r\n]*'
    r'|/\*[\s\S]*?\*/'
    r'|[{}\[\]:]'
)


def update_body(source: str, lines: list[str]) -> str:
    depth = 0
    target_depth = None
    pending = None
    body_start = None
    body_end = None
    array_depth = 0

    for match in TOKEN.finditer(source):
        token = match.group()

        if token.startswith("//") or token.startswith("/*"):
            continue

        if token.startswith('"'):
            if depth == 1 and token == json.dumps(SNIPPET_NAME):
                pending = "snippet"
            elif (
                target_depth == depth
                and token == '"body"'
            ):
                pending = "body"
            continue

        if token == "{":
            depth += 1
            if pending == "snippet":
                target_depth = depth
                pending = None

        elif token == "}":
            if depth == target_depth:
                target_depth = None
            depth -= 1

        elif token == "[":
            depth += 1
            if pending == "body":
                body_start = match.start()
                array_depth = depth
                pending = None

        elif token == "]":
            if depth == array_depth:
                body_end = match.end()
                break
            depth -= 1

    if body_start is None or body_end is None:
        raise ValueError(f"{SNIPPET_NAME}.body が見つかりません")

    # 元のインデントを取得
    line_start = source.rfind("\n", 0, body_start) + 1
    indent = re.match(r"\s*", source[line_start:]).group()
    child_indent = indent + "  "

    # VS Code snippet の $ をエスケープ
    escaped = [line.replace("$", r"\$") for line in lines]

    body = "[\n"
    body += ",\n".join(
        child_indent + json.dumps(line, ensure_ascii=False)
        for line in escaped
    )
    body += "\n" + indent + "]"

    return source[:body_start] + body + source[body_end:]


def main():
    lines = TEMPLATE.read_text(encoding="utf-8").splitlines()
    source = SNIPPET.read_text(encoding="utf-8")

    updated = update_body(source, lines)

    if updated != source:
        SNIPPET.write_text(updated, encoding="utf-8")
        print(f"Updated: {SNIPPET}")
    else:
        print("No changes")


if __name__ == "__main__":
    main()