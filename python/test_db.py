import db

print("---- INIT ----")
print(db.init({"path": "database.db"}))

print("---- INSERT ----")
print(db.run({
    "action": "insert",
    "file": "index.html",
    "hash": "abc123"
}))

print("---- SELECT (after insert) ----")
print(db.run({
    "action": "select",
    "file": "index.html"
}))

print("---- DELETE ONE ----")
print(db.run({
    "action": "delete_one",
    "file": "index.html"
}))

print("---- SELECT (after delete one) ----")
print(db.run({
    "action": "select",
    "file": "index.html"
}))

print("---- INSERT AGAIN ----")
print(db.run({
    "action": "insert",
    "file": "style.css",
    "hash": "FFF111"
}))

print("---- DELETE ALL ----")
print(db.run({
    "action": "delete_all"
}))

print("---- SELECT (after delete all) ----")
print(db.run({
    "action": "select",
    "file": "style.css"
}))
