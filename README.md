# 🛰️ Operasyon: Fantom Hafıza
### Advanced Time-Gating & Volatile Memory Wiping Core

> **BGT210 - Tersine Mühendislik | Final Projesi**  
> Öğrenci: Beyzanur Çakıcı (2420191032)  
> Danışman: Öğr. Gör. Keyvan Arasteh  
> Bölüm: Bilişim Güvenliği Teknolojisi

---

## 📌 Proje Özeti

**Operasyon: Fantom Hafıza**, statik imza tabanlı tersine mühendislik araçlarını (`strings`, `Ghidra`, `IDA Pro`) ve kaba kuvvet bellek dökümü (`gcore`) mekanizmalarını analiz etmek üzere tasarlanmış bir **Volatile Hafıza Yaşam Döngüsü Kontrol** platformudur.

Sistem; hassas verileri runtime boyunca RAM üzerinde XOR (`0x5A`) maskesiyle izole eder. Yalnızca çekirdek düzeyinde yetkilendirilmiş bir IPC sinyali (`SIGUSR1`) aldığında veriyi kısa süreliğine açığa çıkarır. Belirlenen zaman penceresi kapandığında `secure_zero` protokolü bellek bloklarını kalıcı olarak temizler.

---

## 📂 Proje Klasör Yapısı

```
BGT210_Final_Projesi/
├── .gitignore
├── README.md
├── requirements.txt
├── CHANGELOG.md
├── src/
│   ├── bgt210_kurban.c       # POSIX sinyal korumalı kaynak kodu
│   ├── bgt210_binary         # Derlenmiş çalıştırılabilir
│   └── bgt210_monitor.py     # IPC sinyal izleme betiği
└── research/
    └── BGT210_Akademik_Rapor.md
```

---

## 🌌 Teknik Özellikler

| Özellik | Açıklama |
|---|---|
| 🎭 **Dead-Store Elimination Immunity** | `volatile` pointer döngüsü ile derleyici optimizasyonuna karşı bellek silme koruması |
| 🧬 **Async-Signal-Safe Execution** | Signal handler içinde `write()` sistem çağrısı; `printf`/`malloc` kullanılmaz |
| 📡 **Automated Target Discovery** | `/proc/[PID]/comm` taramasıyla dinamik süreç tespiti |
| 🧪 **Zero-Dependency Framework** | Harici kütüphane bağımlılığı yok; saf POSIX çekirdek çağrıları |

---

## 🚀 Kurulum ve Çalıştırma

> ⚠️ **Bu proje yalnızca izole Kali Linux lab ortamında çalıştırılmalıdır.**

### Adım 1 — Derleme

```bash
gcc -Wall -Wextra -O0 -o src/bgt210_binary src/bgt210_kurban.c
```

> `-O0` bayrağı, derleyicinin Dead-Store Elimination optimizasyonuyla `secure_zero` çağrısını silmesini engeller.

### Adım 2 — Süreci Başlatma

```bash
./src/bgt210_binary &
```

Süreç arka planda başlar ve PID numarasını terminale yazdırır.

### Adım 3 — İzleme Betiğini Çalıştırma

```bash
python3 src/bgt210_monitor.py
```

Betik, `procfs` üzerinden süreci tespit eder ve `SIGUSR1` sinyalini enjekte eder.

### Adım 4 — Yaşam Döngüsünü Gözlemleme

İlk terminal ekranına dönün. Sırasıyla şunları gözlemleyebilirsiniz:

1. Maskelenmiş güvenli durum logları
2. Sinyal handler devreye giriyor
3. Veri 5 saniyeliğine açığa çıkıyor
4. `secure_zero` tetikleniyor → bellek temizleniyor
5. Sistem güvenli faza geri dönüyor

---

## 🔍 Analiz Perspektifi: Tersine Mühendislik Direnci

| DFIR / Analiz Taktiği | Sistemin Tepkisi |
|---|---|
| `strings` ile statik analiz | XOR maskesi nedeniyle anlamlı veri görünmez |
| `gcore` / bellek dökümü | Veri pencere dışında RAM'de bulunmaz |
| `SIGUSR1` sinyal enjeksiyonu | Kontrollü açığa çıkarma tetiklenir |
| Pencere sonrası analiz | `secure_zero` ile iz kalmaz |

---

## 📋 Gereksinimler

```
# BGT210 Telemetry Engine Requirements
# Harici paket bağımlılığı yoktur; yerleşik POSIX kütüphaneleri kullanılır.
```

---

## 📜 Lisans & Akademik Not

Bu proje **yalnızca akademik ve eğitim amaçlıdır**. BGT210 dersi kapsamında üretilmiş olup izole lab ortamları dışında kullanımı önerilmez.
