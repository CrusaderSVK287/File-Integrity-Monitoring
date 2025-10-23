# mymodule.py

_config = {}

def init(params):
    global _config
    _config = params.copy()
    return {"status": "initialized"}

def run(params):
    data = _config.copy()
    data.update(params)
    name = data.get("name", "unknown")
    count = data.get("count", 0)
    return {
        "status": "ok",
        "message": f"Hello {name}, processed count={count*2}"
    }

