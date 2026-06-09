import os
import signal
from flask import Flask, jsonify, render_template

app = Flask(__name__, template_folder="templates", static_folder="static")

def get_advanced_telemetry():
    target_name = "bgt210_binary"
    for proc in os.listdir('/proc'):
        if proc.isdigit():
            try:
                with open(f'/proc/{proc}/comm', 'r') as f:
                    if target_name in f.read():
                        pid = int(proc)
                        
                        status_info = {}
                        with open(f'/proc/{proc}/status', 'r') as f:
                            for line in f:
                                if "VmRSS:" in line:
                                    # Grafiğin okuyabilmesi için sadece sayısal değeri ayıklıyoruz
                                    return {
                                        "status": "ACTIVE",
                                        "pid": pid,
                                        "ram_usage": line.split(":")[1].replace("kB", "").strip()
                                    }
            except:
                continue
    return {"status": "INACTIVE", "pid": "N/A", "ram_usage": "0"}

@app.route('/')
@app.route('/dashboard')
def dashboard():
    return render_template('dashboard.html')

@app.route('/api/telemetry', methods=['GET'])
def telemetry():
    return jsonify(get_advanced_telemetry())

@app.route('/api/trigger', methods=['POST'])
def trigger():
    data = get_advanced_telemetry()
    if data["status"] == "INACTIVE":
        return jsonify({"success": False, "message": "Laboratuvar süreci aktif degil!"}), 400
    try:
        os.kill(data["pid"], signal.SIGUSR1)
        return jsonify({"success": True})
    except Exception as e:
        return jsonify({"success": False, "message": str(e)})

if __name__ == '__main__':
    app.run(host='127.0.0.1', port=5000, debug=True)