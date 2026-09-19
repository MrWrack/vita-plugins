"""Repository Manager verification helper.
Use before publishing/copying a VPK. Returns SHA-256 and size and performs a ZIP test."""
from pathlib import Path
import hashlib, zipfile, sys, json
def verify(path):
    p=Path(path)
    if not p.is_file(): raise FileNotFoundError(path)
    h=hashlib.sha256()
    with p.open("rb") as f:
        for chunk in iter(lambda:f.read(1024*1024),b""): h.update(chunk)
    with zipfile.ZipFile(p,"r") as z:
        bad=z.testzip()
        if bad: raise ValueError("Corrupt ZIP entry: "+bad)
    return {"file":p.name,"size":p.stat().st_size,"sha256":h.hexdigest(),"zip_ok":True}
if __name__=="__main__":
    print(json.dumps(verify(sys.argv[1]),indent=2))
