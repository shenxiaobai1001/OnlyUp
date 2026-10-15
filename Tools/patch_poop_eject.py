from pathlib import Path

p = Path(r"D:\MyWork1\UEProject\Project\OnlyUP\Source\OnlyUP\Gift\PoopEjectComponent.cpp")
text = p.read_text(encoding="utf-8")
needle = """\t\tif (Event.NotifyStateClass && Event.NotifyStateClass->GetName().Contains(TEXT("shit")))
\t\t{
\t\t\treturn Event.GetTriggerTime();
\t\t}
"""
if needle not in text:
    raise SystemExit("needle not found")
text = text.replace("\n" + needle, "\n")
p.write_text(text, encoding="utf-8")
print("patched")
