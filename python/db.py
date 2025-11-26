# mymodule.py
import sqlite3

connection = None


def init(params):
    """
    Inicializuje databázu.
    Očakávaný vstup:
        { "path": "database.db" }
    """
    global connection

    try:
        db_path = params.get("path", "database.db")
        connection = sqlite3.connect(db_path)

        cursor = connection.cursor()
        cursor.execute("""
            CREATE TABLE IF NOT EXISTS integrity (
                file TEXT PRIMARY KEY,
                hash TEXT NOT NULL
            )
        """)
        connection.commit()
        cursor.close()

        return {"status": "OK"}

    except Exception as e:
        return {"status": "ERROR", "message": str(e)}


def run(params):
    """
    Spracovanie príkazov:

    INSERT:
        {
            "action": "insert",
            "file": "index.html",
            "hash": "abc123"
        }

    SELECT:
        {
            "action": "select",
            "file": "index.html"
        }

    DELETE ONE:
        {   "action": "delete_one",
            "file": "index.html" 
        }

    DELETE ALL:
        {   
            "action":"delete_all" 
        }        
    """

    global connection

    try:
        if connection is None:
            return {"status": "ERROR", "message": "Database not initialized"}

        cursor = connection.cursor()
        action = params.get("action")

        # -------- INSERT ---------
        if action == "insert":
            file = params["file"]
            hash_value = params["hash"]

            cursor.execute(
                "INSERT OR REPLACE INTO integrity (file, hash) VALUES (?, ?)",
                (file, hash_value)
            )
            connection.commit()

            return {"status": "OK", "message": "Inserted"}

        # -------- SELECT ---------
        elif action == "select":
            file = params["file"]

            cursor.execute(
                "SELECT hash FROM integrity WHERE file = ?",
                (file,)
            )
            row = cursor.fetchone()

            return {
                "status": "OK",
                "file": file,
                "hash": row[0] if row else "NULL"
            }

        # -------- DELETE ONE ---------
        elif action == "delete_one":
            file = params["file"]

            cursor.execute("DELETE FROM integrity WHERE file = ?", (file,))
            connection.commit()

            return {"status": "OK", "message": f"Deleted {file}"}

        # -------- DELETE ALL ---------
        elif action == "delete_all":
            cursor.execute("DELETE FROM integrity")
            connection.commit()

            return {"status": "OK", "message": "All rows deleted"}



        # -------- UNKNOWN ---------
        else:
            return {"status": "ERROR", "message": "Unknown action"}

    except Exception as e:
        return {"status": "ERROR", "message": str(e)}

    finally:
        try:
            cursor.close()
        except:
            pass
