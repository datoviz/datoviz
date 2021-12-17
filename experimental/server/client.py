import json
from pathlib import Path
import requests

ROOT_DIR = Path(__file__).resolve().parent / "../../"

with open(ROOT_DIR / "datoviz/tests/triangle.json", "r") as f:
    d = f.read()

r = requests.post('http://localhost:1234/request', json=json.loads(d))
