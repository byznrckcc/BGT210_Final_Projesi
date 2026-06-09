function log(msg, state="info") {
    const box = document.getElementById("logs");
    let c = "text-green-400";
    if(state==="warn") c = "text-yellow-400 font-bold";
    if(state==="success") c = "text-cyan-300 font-bold";
    box.innerHTML += `<div>[${new Date().toLocaleTimeString()}] ${msg}</div>`;
    box.scrollTop = box.scrollHeight;
}

function update() {
    fetch('/api/telemetry').then(r => r.json()).then(d => {
        document.getElementById("pid").innerText = d.pid;
        document.getElementById("ram").innerText = d.ram_usage;
        const s = document.getElementById("status");
        if(d.status === "ACTIVE") {
            s.innerText = "ONLINE // ASLR ENFORCED";
            s.className = "text-green-400 font-bold";
        } else {
            s.innerText = "OFFLINE // BEKLENİYOR";
            s.className = "text-red-500 font-bold";
        }
    });
}

function triggerAttack() {
    log("Kritik sinyal enjeksiyonu baslatildi...", "warn");
    fetch('/api/trigger', {method:'POST'}).then(r => r.json()).then(d => {
        log("Sinyal iletildi! Çekirdek adres entropisinden anlik anahtar turetiliyor...", "success");
        log("Uygulama devingen yiginda (Heap) rastgele ofset araligina atladi!", "warn");
        setTimeout(() => {
            log("Pencere kapandi. secure_zero() bellek alanini kalici olarak kazidi.", "success");
        }, 5000);
    });
}

setInterval(update, 1000);
