import os
import sys

def find_pid_by_name(process_name):
    """Sistemde çalışan sürecin PID'sini otomatik bulur."""
    for proc in os.listdir('/proc'):
        if proc.isdigit():
            try:
                with open(f'/proc/{proc}/comm', 'r') as f:
                    if process_name in f.read():
                        return proc
            except:
                continue
    return None

def main():
    target_name = "bgt210_binary"
    print(f"[*] BGT210 Forensics: '{target_name}' süreci sistemde aranıyor...")
    
    pid = find_pid_by_name(target_name)
    if not pid:
        print(f"[-] HATA: '{target_name}' bulunamadı! Önce C programını çalıştırın.")
        return
        
    print(f"[+] Süreç başarıyla tespit edildi! Hedef PID: {pid}")
    print(f"[*] ptrace-less Canlı Bellek Analizi başlatılıyor...")

    try:
        maps_file = open(f"/proc/{pid}/maps", "r")
        mem_file = open(f"/proc/{pid}/mem", "rb")
    except PermissionError:
        print("[-] HATA: Çekirdek bellek blokları için 'sudo' yetkisi gereklidir!")
        return

    for line in maps_file:
        parts = line.split()
        if len(parts) < 2:
            continue
            
        addr_range = parts[0]
        perms = parts[1]
        
        # Okunabilir anonim bellek sayfalarını tara
        if "r" in perms and not line.endswith("]"):
            start_str, end_str = addr_range.split("-")
            start = int(start_str, 16)
            end = int(end_str, 16)
            
            try:
                mem_file.seek(start)
                chunk = mem_file.read(end - start)
                
                if b"BGT210_SECRET" in chunk:
                    idx = chunk.find(b"BGT210_SECRET")
                    flag = chunk[idx:].split(b"\x00")[0].decode("utf-8", errors="ignore")
                    print("\n[+] =====================================================")
                    print("[+] BGT210 GÖREV BAŞARILI: CANLI RAM DEŞİFRE EDİLDİ!")
                    print(f"[+] Bellek Adresi (Offset): 0x{start_str}")
                    print(f"[+] RAM'den Çekilen Veri: {flag}")
                    print("[+] =====================================================\n")
                    return
            except:
                continue

    print("[-] Tarama bitti fakat veri bulunamadı.")

if __name__ == "__main__":
    main()
