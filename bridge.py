from flask import Flask, request, jsonify
import json

app = Flask(__name__)
FILE = "patients.json"

def read():
    try:
        with open(FILE, "r") as f:
            return json.load(f)
    except:
        return []

def write(data):
    with open(FILE, "w") as f:
        json.dump(data, f, indent=4)

@app.route("/add", methods=["POST"])
def add():
    data = read()
    data.append(request.json)
    write(data)
    return {"status": "ok"}

@app.route("/all", methods=["GET"])
def all_data():
    return jsonify(read())

if __name__ == "__main__":
    app.run(port=5000)