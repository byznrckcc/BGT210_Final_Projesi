import os
import signal
import sys
import time

def find_pid_by_name(process_name):
    """Sistem süreç tablosunda hedef binary ismini arar ve PID döndürür."""
    for proc in os.listdir('/proc'):
        if proc.isdigit():
            try:
                with open(f'/proc/{proc}/comm', 'r') as f:
                    if process_name in f.read():
                        return int(proc)
            except:
                continue
    return None

def main():
    target_name = "bgt210_binary"
    print(f"=====================================================")
    print(f"[*] BGT210 Olay Monitoru: '{target_name}' araniyor...")
    print(f"=====================================================")
    
    # 1. Adım: Hedef Sürecin PID Kimliğini Otomatik Bulma
    pid = find_pid_by_name(target_name)
    if not pid:
        print(f"[-] HATA: '{target_name}' bulunamadi! Once C programini calistirin.")
        return

    print(f"[+] Hedef Secenek Tespit Edildi! Aktif PID: {pid}")
    print("[*] Veri yasam dongusunu tetiklemek icin USR1 sinyali hazirlaniyor...")
    time.sleep(2)

    # 2. Adım: Güvenli Olay Tetikleme Sinyali (SIGUSR1) Gönderimi
    print(f"[🚀] Sinyal Enjekte Ediliyor: kill -USR1 {pid}")
    try:
        os.kill(pid, signal.SIGUSR1)
        print("[+] Sinyal basariyla iletildi!")
        print("[*] TALİMAT: Diger terminal sekmesine gecip CANLI RAM DEĞİŞİMİNİ izleyin!")
    except ProcessLookupError:
        print("[-] HATA: Surec aniden kapandigi icin sinyal gonderilemedi.")
    except PermissionError:
        print("[-] HATA: Çekirdek sinyal hatti icin yetki yetersiz.")

if __name__ == "__main__":
    main()
