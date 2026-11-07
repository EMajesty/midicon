import argparse,sys
from .models import defaults
from .serialization import load_json,save_json
from .validation import validate
from .protocol import Device
def main(argv=None):
    p=argparse.ArgumentParser(prog="python -m configurator");p.add_argument("--port",default="/dev/ttyUSB0");s=p.add_subparsers(dest="cmd",required=True)
    for n in ("validate","upload"):q=s.add_parser(n);q.add_argument("file")
    q=s.add_parser("download");q.add_argument("file");s.add_parser("save");s.add_parser("reload");s.add_parser("gui");q=s.add_parser("defaults");q.add_argument("file")
    a=p.parse_args(argv)
    if a.cmd=="validate":
        e=validate(load_json(a.file));print("valid" if not e else "\n".join(e));return bool(e)
    if a.cmd=="defaults":save_json(defaults(),a.file);return 0
    if a.cmd=="gui":from .ui.main_window import run;return run()
    d=Device(a.port)
    if a.cmd=="upload":d.upload(load_json(a.file))
    elif a.cmd=="download":save_json(d.download(),a.file)
    elif a.cmd=="save":d.save()
    elif a.cmd=="reload":d.reload()
    return 0
if __name__=="__main__":sys.exit(main())
