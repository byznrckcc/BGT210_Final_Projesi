#!/bin/bash
echo "[*] BGT210 Lab Sanal Sınır Kontrolü Başlatılıyor..."
if ps aux | grep -v grep | grep bgt210_binary > /dev/null
then
    echo "[+] Çekirdek Süreç Durumu: AKTİF (ONLINE)"
else
    echo "[-] UYARI: Çekirdek Süreç Durumu: KAPALI (OFFLINE)"
fi

