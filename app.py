import os
import signal
import time
from flask import Flask, jsonify, render_template, request

app = Flask(__name__, template_folder="templates", static_folder="static")

def get_process_telemetry():
    """procfs üzerinden bgt210_binary sürecinin tüm hayati verilerini söker."""
    target_name = "bgt210_binary"
    for proc in os.listdir('/proc'):
        if proc.isdigit():
            try:
                # Süreç ismini doğrula
                with open(f'/proc/{proc}/comm', 'r') as f:
                    comm_name = f.read().strip()
                
                if target_name in comm_name:
                    pid = int(proc)
                    
                    # /proc/[PID]/status dosyasından bellek ve durum oku
                    status_info = {}
                    with open(f'/proc/{proc}/status', 'r') as f:
                        for line in f:
                            if any(k in line for k in ["State:", "VmRSS:", "VmSize:", "Threads:"]):
                                parts = line.split(":")
                                status_info[parts[0].strip()] = parts[1].strip()
                    
                    return {
                        "status": "ACTIVE",
                        "pid": pid,
                        "name": comm_name,
                        "state": status_info.get("State", "UNKNOWN"),
                        "ram_usage": status_info.get("VmRSS", "0 kB"),
                        "virtual_memory": status_info.get("VmSize", "0 kB"),
                        "threads": status_info.get("Threads", "1")
                    }
            except:
                continue
    return {"status": "INACTIVE", "pid": "N/A", "ram_usage": "0 kB", "state": "OFFLINE"}

@app.route('/')
@app.route('/dashboard')
def dashboard():
    return render_template('dashboard.html')

@app.route('/api/telemetry', channels=['GET'])
def telemetry():
    """Arayüzün her saniye sorgulayacağı canlı veri hattı."""
    return jsonify(get_process_telemetry())

@app.route('/api/trigger', methods=['POST'])
def trigger_signal():
    """Arayüzden gelen butona basıldığında SIGUSR1 enjekte eder."""
    telemetry_data = get_process_telemetry()
    if telemetry_data["status"] == "INACTIVE":
        return jsonify({"success": False, "message": "Target process offline!"}), 400
    
    try:
        os.kill(telemetry_data["pid"], signal.SIGUSR1)
        return jsonify({
            "success": True, 
            "message": f"SIGUSR1 successfully injected into PID {telemetry_data['pid']}"
        })
    except Exception as e:
        return jsonify({"success": False, "message": str(e)}), 500

if __name__ == '__main__':
    # Kurumsal port ayarı
    app.run(host='127.0.0.1', port=5000, debug=True)
