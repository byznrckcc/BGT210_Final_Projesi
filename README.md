Markdown

<p align="center">
  <img src="logo-istinye-renkli-2.svg" alt="İstinye Üniversitesi Logo" width="220">
</p>

# 🛰️ OPERASYON: FANTOM HAFIZA (PHASE-II)
### İleri Düzey ASLR Entropi Tabanlı Çalışma Zamanı Anahtar Türetimi ve Devingen Yığın Atlamalı (Heap Hopping) Canlı Bellek Adli Analiz Platformu

---

## 👤 GELİŞTİRİCİ VE AKADEMİK KÜNYE
* **Araştırmacı / Öğrenci:** Beyzanur Çakıcı
* **Öğrenci Numarası:** 2420191032
* **Bölüm:** Bilişim Güvenliği Teknolojisi Bölümü
* **Kurum:** İstinye Üniversitesi Mühendislik ve Doğa Bilimleri Fakültesi
* **Ders:** BGT210 - Tersine Mühendislik (Final Dönem Projesi)
* **Proje Danışmanı:** Öğr. Gör. Keyvan Arasteh

---

## 🖼️ SİBER OPERASYON MERKEZİ (SOC DASHBOARD) GÖRÜNTÜSÜ

<p align="center">
  <img src="Screenshot From 2026-06-09 19-25-17.png" alt="Phantom Memory Phase-II SOC Dashboard" width="100%" style="border: 2px solid #00f0ff; border-radius: 8px; box-shadow: 0 0 20px rgba(0,240,255,0.3);">
</p>

---

## 📝 1. PROJE ÖZETİ (ABSTRACT)

Geleneksel tersine mühendislik ve adli bilişim (Forensics) çalışmaları, çoğunlukla disk üzerindeki statik dosyaların (`EXE`, `ELF` v.b.) yapısını incelemeye odaklanır. Ancak modern gelişmiş kalıcı tehdit aktörleri (APT) ve yeni nesil zararlı yazılımlar, statik analiz araçlarından (`strings`, `Ghidra`, `IDA Pro`) kaçmak amacıyla kritik yapılandırma verilerini (C2 adresleri, kriptografik anahtarlar, kullanıcı parolaları) diskte açık metin (plaintext) olarak saklamazlar. Bu veriler yalnızca yazılımın çalışma anında (runtime) dinamik olarak RAM üzerinde inşa edilir.

**Operasyon: Fantom Hafıza (Phase-II)**, disk üzerinde hiçbir adli iz bırakmayan bu "hayalet verilerin" canlı bellek analiz yöntemleriyle nasıl tespit edilebileceğini uygulamalı olarak kanıtlarken, aynı zamanda analiz süreçlerini sabote eden anti-tersine mühendislik mekanizmalarını laboratuvar ortamında simüle eder. Proje; ilk teklif aşamasındaki manuel analiz vaatlerini eksiksiz yerine getirmekle kalmayıp, endüstriyel düzeyde bir **Polimorfik Bellek Koruma Sistemi** ve **Asenkron Web Telemetri Arayüzü (SOC Dashboard)** ile donatılmıştır.

---

## 🎯 2. SENARYO VE SİBER TEHDİT MODELLENMESİ

Sistem, hedef bir kurban yazılımın hassas bir parolayı/flag verisini sistem ayağa kalktığı andan itibaren koruma altında tuttuğu senaryoyu temel alır. Sistem mimarisi şu kritik savunma katmanlarından oluşur:

1. **Anti-Strings (Statik Körlük):** Hassas veri disk üzerinde ham halde bulunmaz; ilk derleme aşamasında statik XOR maskesiyle kör edilir.
2. **Dinamik ASLR Entropi Key Derivasyonu:** Program tetiklendiğinde sabit bir şifre çözme anahtarı kullanmaz. İşletim sisteminin o an sürece atadığı rastgele yığın adres alanının (**Address Space Layout Randomization**) sayısal entropisini milisaniyelik zaman damgasıyla harmanlayarak çalışma zamanında dinamik bir anahtar türetir.
3. **Devingen Yığın Atlaması (Heap Hopping):** Çözülen veri RAM üzerinde sabit bir adreste kalmaz. Arka arkaya rastgele boyutlarda bellek blokları tahsis edilerek veri sürekli olarak farklı yığın (Heap) ofsetlerine fırlatılır. Bu durum, bellek izleyicilerin (GDB, Cheat Engine, HxD) sabit bir adresi dinlemesini imkansız hale getirir.
4. **Volatile Memory Burn:** Verinin açığa çıktığı 5 saniyelik kritik operasyon penceresi kapandığı an, derleyici optimizasyonlarına bağışıklığı olan `secure_zero()` fonksiyonu devreye girerek bellek alanını kalıcı olarak kazır ve sistemi ilk maskeli haline geri döndürür.

---

## 🛠️ 3. TEKNİK İŞ AKIŞI VE ALGORİTMİK ŞEMA (STEP-BY-STEP)

```text
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

🔍 Ayrıntılı Aşama Analizleri

    Aşama 1 (Yazılım Geliştirme): Çekirdek yazılım POSIX C standartlarında, işletim sistemi sinyal mekanizmalarını (sigaction) doğrudan dinleyecek mimaride kodlanmıştır.

    Aşama 2 (Statik Analiz): Derlenen ikilinin disk üzerindeki zafiyet haritası çıkarılmıştır. .rodata segment sızıntıları manuel olarak incelenmiştir.

    Aşama 3 (Manuel Dinamik Analiz): Linux platformunda GDB Debugger ve procfs sanal dosya sistemi kullanılarak, canlı RAM dökümü üzerinden bellek sayfaları haritalanmıştır. Windows bacağında ise Process Hacker ile alınan dump, HxD Hex Editor ile ham byte seviyesinde analiz edilmiştir.

    Aşama 4 (Adli Doğrulama): Tetiklenen SIGUSR1 sinyaliyle birlikte dinamik bellek deşifre süreçleri, yığın kaymaları ve bellek kazıma (volatile imha) döngüleri doğrulanmıştır.

    Aşama 5 (SOC Otomasyonu): Tüm bu süreçler manuel komut satırından kurtarılarak, Python Flask tabanlı, anlık veri grafik hatlarına sahip bir siber harekat merkezine (Dashboard) bağlanmıştır.

🧰 4. DOĞRULANMIŞ KULLANILACAK ARAÇ SETİ (TOOLKIT MATRIX)
Analiz Katmanı	Kullanılan Araç / Altyapı	Operasyonel Amacı ve Fonksiyonu
İzole Laboratuvar	Kali Linux v2026.1 / Windows 11	Saldırı yüzeyi azaltılmış, izole adli analiz sanal ortamı.
Statik Analiz	Ghidra v11.x & Linux strings	Kod yapısını, kontrol grafik akışlarını ve .rodata sızıntılarını tespit etme.
Dinamik Hata Ayıklama	GDB (GNU Debugger)	Canlı sürece çalışma anında kancalanma (attach) ve hafıza adresi tarama.
Windows Adli Bilişim	HxD Hex Editor & Process Hacker	Canlı Windows RAM dökümü çıkarma ve ham Hex byte blok takibi.
Orkestrasyon & GUI	Python 3.10+, Flask API, Chart.js	Gelişmiş Phase-II Siber Operasyon Paneli ve Canlı Telemetri Grafikleri.
CI/CD & Otomasyon	Makefile & Docker Subsystem	Çok aşamalı güvenli derleme (Multi-stage) ve sandbox test yatağı.
🔬 5. MANUEL ADLİ BELLEK ANALİZİ UYGULAMA REHBERİ

Proje teklifinde vaat edilen manuel analiz prosedürlerinin Linux çekirdeği üzerindeki pratik komut serisi aşağıda listelenmiştir:
1. strings İle Statik İpucu Avı
Bash

strings src/bgt210_binary | grep BGT210
# Bulgular: Sabit string literalleri ifşa olur ancak runtime anındaki 
# dinamik polimorfik anahtarlar ve heap hopping adresleri diskte ASLA görünmez.

2. GDB İle Çalışma Anında Bellek Haritalama (Memory Mapping)

Kurban süreç arka planda çalışırken (PID: 14374), GNU Debugger ile sürece runtime anında bağlanılır:
Bash

sudo gdb src/bgt210_binary 14374

Sürecin RAM üzerinde kapladığı sanal bellek adres aralıklarını ve Heap/Stack ofsetlerini listelemek için çekirdek haritası istenir:
Plaintext

(gdb) info proc mappings

Elde edilen başlangıç ve bitiş adresleri arasında (0x562dcab7a000 - 0x562dcab7b000) gizli hayalet verinin ham byte araması gerçekleştirilir:
Plaintext

(gdb) find /b 0x562dcab7a000, 0x562dcab7b000, "BGT210"
# Çıktı: Verinin RAM üzerindeki tam ofset adresi ve deşifre edilmiş içeriği listelenir.

📂 6. PROJE KLASÖR HİYERARŞİSİ
Plaintext

BGT210_Final_Projesi/
├── app.py                      # Flask REST API & Çekirdek Süreç Takip Motoru
├── Dockerfile                  # Multi-stage, non-root korumalı SecOps imajı
├── docker-compose.yml          # Network-isolated Sandbox orkestrasyonu
├── Makefile                    # Endüstriyel derleme ve CI/CD otomasyonu
├── ruff.toml                   # SAST Python statik kod analiz kuralları
├── requirements.txt            # Lab Python bağımlılık konfigürasyonu
├── TODO.md                     # MITRE ATT&CK tabanlı Ar-Ge yol haritası
├── simulation.mp4              # Oynatılabilir Final Operasyonel Kanıt Videosu
├── src/
│   ├── bgt210_binary           # Polimorfik C Çekirdeği (Executable)
│   ├── bgt210_kurban.c         # ASLR & Heap Hopping kaynak kodu
│   ├── bgt210_monitor.py       # Manuel testler için sinyal enjeksiyon betiği
│   └── bgt210_spy.py           # Bellek casusluk simülasyon aracı
├── research/
│   └── BGT210_Akademik_Rapor.md # Çekirdek mimarisi derin akademik analiz raporu
├── static/
│   └── css/cyberpunk.css       # Phase-II Siber Gösterge Paneli stilleri
└── templates/
    └── dashboard.html          # Gelişmiş Canlı Grafik ve Hex Matris Ön Yüzü

🌟 7. PROJENİN ÖZGÜNLÜĞÜ VE AKADEMİK ÖNEMİ

Bu proje, siber güvenlik dünyasında sıklıkla dile getirilen "Statik analiz her şey değildir" felsefesini pratik ve sarsıcı bir biçimde savunur. Piyasada hazır bulunan basit "CrackMe" egzersizlerini çözmek yerine; araştırmacının kendi gelişmiş laboratuvar ortamını, adli telemetri yazılımlarını ve anti-forensics savunma mekanizmalarını bizzat kendisinin inşa etmesi projeyi benzersiz kılar.

Phase-II aşamasıyla eklenen anlık çizgi grafik takipleri ve interaktif canlı bellek Hex matris simülasyonu, projenin akademik sunum kabiliyetini ve görsel gücünü zirveye taşımıştır. Bu yaklaşım, modern APT saldırılarının bellek içi (In-Memory/Fileless) çalışma mantıklarını anlamak ve onlara karşı defansif mekanizmalar geliştirmek adına kritik bir öneme sahiptir.

Bu çalışma, İstinye Üniversitesi Bilişim Güvenliği Teknolojisi Bölümü BGT210 dersi final gereksinimleri doğrultusunda Beyzanur Çakıcı tarafından özgün olarak geliştirilmiştir. Akademik kaynak gösterilmeden kopyalanamaz.


---

Lokaldeki `README.md` dosyasını bu devasa içerikle mühürledikten sonra tek yapman gereken Git üzerinden repoya son pushesini göndermek patron:

```bash
git add README.md
git commit -m "Docs: Complete extensive enterprise-grade documentation deployment with dynamic placeholders"
git push origin main