const express = require("express");
const http = require("http");
const { Server } = require("socket.io");
const { SerialPort } = require("serialport");
const { ReadlineParser } = require("@serialport/parser-readline");
const cors = require("cors");

const app = express();
app.use(cors());
app.use(express.json());

const server = http.createServer(app);
const io = new Server(server, {
  cors: { origin: "*" }
});

// ================== SERIAL ==================
const port = new SerialPort({
  path: "COM11",   // change if needed
  baudRate: 115200
});

const parser = port.pipe(new ReadlineParser({ delimiter: "\n" }));

// ================== PATIENT STORAGE ==================
let patients = [];
let currentPatientId = null;

// ================== SERIAL DATA ==================
parser.on("data", (line) => {
  try {
    const data = JSON.parse(line);
    console.log("DATA:", data);

    const patient = patients.find(p => p.id === currentPatientId);

    if (patient) {
      patient.data = data;
    }

    io.emit("patientsData", patients);

  } catch (err) {
    console.log("Invalid JSON:", line);
  }
});

// ================== ADD PATIENT ==================
app.post("/add-patient", (req, res) => {
  const { name, bed } = req.body;

  if (!name || !bed) {
    return res.status(400).send("Invalid patient");
  }

  const newPatient = {
    id: Date.now(),
    name,
    bed,
    data: null
  };

  patients.push(newPatient);
  currentPatientId = newPatient.id;

  res.json(newPatient);
});

// ================== SET CAPACITY ==================
app.post("/set-capacity", (req, res) => {
  const { capacity } = req.body;

  if (!capacity || capacity <= 0) {
    return res.status(400).send("Invalid capacity");
  }

  const msg = String(capacity) + "\n";

  port.write(msg, (err) => {
    if (err) {
      console.log("Write error:", err.message);
      return res.status(500).send("Write failed");
    }

    port.drain();
    console.log("Sent:", msg.trim());

    res.send("OK");
  });
});

// ================== FRONTEND ==================
app.use(express.static("public"));

app.get("/", (req, res) => {
  res.sendFile(__dirname + "/public/nurse.html");
});
app.get("/patient", (req, res) => {
  res.sendFile(__dirname + "/public/patient.html");
});
// ================== START ==================
server.listen(3000, () => {
  console.log("Server running on http://localhost:3000");
});