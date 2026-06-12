<p align="center">
  <img src="logo-istinye-renkli-2.svg" alt="İstinye Üniversitesi Logo" width="220">
</p>

# 🛰️ OPERASYON: FANTOM HAFIZA (PHASE-II)
### İleri Düzey ASLR Entropi Tabanlı Çalışma Zamanı Anahtar Türetimi ve Devingen Yığın Atlamalı (Heap Hopping) Canlı Bellek Adli Analiz Platformu

---

<p align="center">
  <video src="simulation.mp4" width="100%" controls poster="Screenshot From 2026-06-09 19-25-17.png" style="border: 2px solid #00f0ff; border-radius: 8px; box-shadow: 0 0 20px rgba(0,240,255,0.3);">
    Tarayıcınız video etiketini desteklemiyor. Lütfen ana dizindeki <code>simulation.mp4</code> dosyasını indirerek manuel oynatınız.
  </video>
</p>



## 👤 Geliştirici ve Akademik Künye

| Alan | Bilgi |
|---|---|
| **Araştırmacı / Öğrenci** | Beyzanur Çakıcı |
| **Öğrenci Numarası** | 2420191032 |
| **Bölüm** | Bilişim Güvenliği Teknolojisi |
| **Kurum** | İstinye Üniversitesi, Mühendislik ve Doğa Bilimleri Fakültesi |
| **Ders** | BGT210 - Tersine Mühendislik (Final Projesi) |
| **Proje Danışmanı** | Öğr. Gör. Keyvan Arasteh |

---

## 🖼️ SOC Dashboard Görüntüsü

<p align="center">
  <img src="Screenshot From 2026-06-09 19-25-17.png" alt="Phantom Memory Phase-II SOC Dashboard" width="100%" style="border: 2px solid #00f0ff; border-radius: 8px; box-shadow: 0 0 20px rgba(0,240,255,0.3);">
</p>

---

## 📝 1. Proje Özeti

Geleneksel tersine mühendislik ve adli bilişim çalışmaları, çoğunlukla disk üzerindeki statik dosyaların (`EXE`, `ELF`) yapısını incelemeye odaklanır. Ancak modern gelişmiş kalıcı tehdit aktörleri (APT) ve yeni nesil zararlı yazılımlar, statik analiz araçlarından (`strings`, `Ghidra`, `IDA Pro`) kaçmak amacıyla kritik yapılandırma verilerini diskte açık metin olarak saklamazlar. Bu veriler yalnızca çalışma anında (runtime) dinamik olarak RAM üzerinde inşa edilir.

**Operasyon: Fantom Hafıza (Phase-II)**, disk üzerinde hiçbir adli iz bırakmayan bu "hayalet verilerin" canlı bellek analiz yöntemleriyle nasıl tespit edilebileceğini uygulamalı olarak kanıtlarken, aynı zamanda analiz süreçlerini sabote eden anti-tersine mühendislik mekanizmalarını laboratuvar ortamında simüle eder. Proje; endüstriyel düzeyde bir **Polimorfik Bellek Koruma Sistemi** ve **Asenkron Web Telemetri Arayüzü (SOC Dashboard)** ile donatılmıştır.

---

## 🎯 2. Senaryo ve Siber Tehdit Modellemesi

Sistem, hedef bir kurban yazılımın hassas bir parolayı/flag verisini başlangıçtan itibaren koruma altında tuttuğu senaryoyu temel alır. Savunma katmanları:

1. **Anti-Strings (Statik Körlük):** Hassas veri disk üzerinde ham halde bulunmaz; ilk derleme aşamasında statik XOR maskesiyle kör edilir.
2. **Dinamik ASLR Entropi Key Derivasyonu:** Sabit bir şifre çözme anahtarı kullanılmaz. İşletim sisteminin sürece atadığı rastgele yığın adres alanının (ASLR) entropisini milisaniyelik zaman damgasıyla harmanlayarak çalışma zamanında dinamik bir anahtar türetilir.
3. **Devingen Yığın Atlaması (Heap Hopping):** Çözülen veri RAM üzerinde sabit bir adreste kalmaz. Rastgele boyutlarda bellek blokları tahsis edilerek veri sürekli farklı yığın ofsetlerine taşınır; bu durum bellek izleyicilerin sabit adres dinlemesini engeller.
4. **Volatile Memory Burn:** 5 saniyelik operasyon penceresi kapandığında `secure_zero()` fonksiyonu devreye girerek bellek alanını kalıcı olarak kazır.

---

## 🛠️ 3. Teknik İş Akışı

```
+------------------------+      +--------------------------+      +----------------------------+
|  AŞAMA 1: DERLEME      | ---> |  AŞAMA 2: STATİK ANALİZ  | ---> | AŞAMA 3: MANUEL DİNAMİK    |
|  -O0 Strict SecOps     |      |  Ghidra & strings        |      | GDB Attach & Offset Hunt   |
|  fPIE/pie Enforcements |      |  Zafiyet Tespiti         |      | Adres Haritalama (Procfs)  |
+------------------------+      +--------------------------+      +----------------------------+
                                                                                |
                                                                                v
+------------------------+      +--------------------------+      +----------------------------+
|  FİNAL: DOCKER DEPLOY  | <--- |  AŞAMA 5: OTOMASYON      | <--- | AŞAMA 4: ADLİ REAKSİYON    |
|  Isolated Sandbox      |      |  Flask REST API          |      | SIGUSR1 Sinyal Enjeksiyonu |
|  Non-Root Privileges   |      |  Chart.js & Hex Matrix   |      | Heap Hopping & Wipe Out    |
+------------------------+      +--------------------------+      +----------------------------+
```

### Aşama Açıklamaları

**Aşama 1 — Yazılım Geliştirme:** Çekirdek yazılım POSIX C standartlarında, `sigaction` sinyal mekanizmalarını doğrudan dinleyecek mimaride kodlanmıştır.

**Aşama 2 — Statik Analiz:** Derlenen ikilinin disk üzerindeki zafiyet haritası çıkarılmış; `.rodata` segment sızıntıları manuel olarak incelenmiştir.

**Aşama 3 — Manuel Dinamik Analiz:** Linux platformunda GDB ve procfs kullanılarak canlı RAM dökümü üzerinden bellek sayfaları haritalanmıştır. Windows bacağında Process Hacker ile alınan dump, HxD Hex Editor ile ham byte seviyesinde analiz edilmiştir.

**Aşama 4 — Adli Doğrulama:** Tetiklenen `SIGUSR1` sinyaliyle birlikte dinamik bellek deşifre süreçleri, yığın kaymaları ve bellek kazıma döngüleri doğrulanmıştır.

**Aşama 5 — SOC Otomasyonu:** Tüm süreçler Python Flask tabanlı, anlık grafik hatlarına sahip bir siber harekat panosuna (Dashboard) bağlanmıştır.

---

## 🧰 4. Araç Seti (Toolkit Matrix)

| Analiz Katmanı | Kullanılan Araç / Altyapı | Operasyonel Amaç |
|---|---|---|
| İzole Laboratuvar | Kali Linux v2026.1 / Windows 11 | Saldırı yüzeyi azaltılmış izole adli analiz ortamı |
| Statik Analiz | Ghidra v11.x & `strings` | Kontrol akış grafikleri ve `.rodata` sızıntı tespiti |
| Dinamik Hata Ayıklama | GDB (GNU Debugger) | Canlı sürece kancalanma ve hafıza adresi tarama |
| Windows Adli Bilişim | HxD Hex Editor & Process Hacker | Canlı RAM dökümü ve ham Hex byte blok takibi |
| Orkestrasyon & GUI | Python 3.10+, Flask, Chart.js | SOC Paneli ve canlı telemetri grafikleri |
| CI/CD & Otomasyon | Makefile & Docker | Çok aşamalı güvenli derleme ve sandbox test yatağı |

---

## 🔬 5. Manuel Adli Bellek Analizi Uygulama Rehberi

### strings ile Statik İpucu Avı

```bash
strings src/bgt210_binary | grep BGT210
# Bulgular: Sabit string literalleri ifşa olur ancak runtime anındaki
# dinamik anahtarlar ve heap hopping adresleri diskte görünmez.
```

### GDB ile Çalışma Anında Bellek Haritalama

Kurban süreç arka planda çalışırken GDB ile sürece bağlanılır:

```bash
sudo gdb src/bgt210_binary 14374
```

Sanal bellek adres aralıklarını ve Heap/Stack ofsetlerini listelemek için:

```
(gdb) info proc mappings
```

Elde edilen adres aralığında gizli verinin ham byte araması:

```
(gdb) find /b 0x562dcab7a000, 0x562dcab7b000, "BGT210"
```

---

## 📂 6. Proje Klasör Hiyerarşisi

```
BGT210_Final_Projesi/
├── app.py                      # Flask REST API & Çekirdek Süreç Takip Motoru
├── Dockerfile                  # Multi-stage, non-root korumalı SecOps imajı
├── docker-compose.yml          # Network-isolated Sandbox orkestrasyonu
├── Makefile                    # Endüstriyel derleme ve CI/CD otomasyonu
├── ruff.toml                   # SAST Python statik kod analiz kuralları
├── requirements.txt            # Lab Python bağımlılık konfigürasyonu
├── TODO.md                     # MITRE ATT&CK tabanlı Ar-Ge yol haritası
├── simulation.mp4              # Final Operasyonel Kanıt Videosu
├── src/
│   ├── bgt210_binary           # Polimorfik C Çekirdeği (Executable)
│   ├── bgt210_kurban.c         # ASLR & Heap Hopping kaynak kodu
│   ├── bgt210_monitor.py       # Sinyal enjeksiyon betiği
│   └── bgt210_spy.py           # Bellek casusluk simülasyon aracı
├── research/
│   └── BGT210_Akademik_Rapor.md
├── static/
│   └── css/cyberpunk.css       # SOC Dashboard stilleri
└── templates/
    └── dashboard.html          # Canlı Grafik ve Hex Matris Ön Yüzü
```

---

## 🌟 7. Projenin Özgünlüğü ve Akademik Önemi

Bu proje, siber güvenlik alanındaki "Statik analiz her şey değildir" felsefesini pratik olarak savunur. Hazır CrackMe egzersizlerini çözmek yerine; araştırmacının kendi gelişmiş laboratuvar ortamını, adli telemetri yazılımlarını ve anti-forensics savunma mekanizmalarını bizzat inşa etmesi projeyi özgün kılar.

Phase-II ile eklenen anlık çizgi grafik takipleri ve interaktif canlı bellek Hex matris simülasyonu, projenin akademik sunum kabiliyetini güçlendirmiştir. Bu yaklaşım, modern APT saldırılarının bellek içi (In-Memory/Fileless) çalışma mantıklarını anlamak ve defansif mekanizmalar geliştirmek adına kritik öneme sahiptir.

---

> Bu çalışma, İstinye Üniversitesi Bilişim Güvenliği Teknolojisi Bölümü BGT210 dersi final gereksinimleri doğrultusunda **Beyzanur Çakıcı** tarafından özgün olarak geliştirilmiştir. Akademik kaynak gösterilmeden kopyalanamaz.