# BGT210 – TERSİNE MÜHENDİSLİK FİNAL PROJESİ AKADEMİK RAPORU

## 🛰️ Operasyon: Fantom Hafıza
### Güvenli Bellek Yönetimi ve Olay-Tetiklemeli Veri Yaşam Döngüsü Analizi

---

## 1. Giriş ve Motivasyon

Modern sistemlerde hassas veriler (kimlik bilgileri, şifreleme anahtarları, oturum tokenleri) RAM'de düz metin (plaintext) olarak kaldığı sürece çeşitli gelişmiş saldırı vektörlerine karşı savunmasız kalır. Bu çalışmada, tersine mühendislik süreçlerinde statik ve dinamik analizi zorlaştıran tehditlere karşı geliştirilen defansif programlama teknikleri incelenmiştir.

Odaklanılan temel tehdit vektörleri:

- **Cold-Boot Saldırıları:** DRAM hücrelerinin güç kesildikten sonra saniyeler ila dakikalarca veriyi kalıntı (remanence) olarak tutabilmesi riskidir.
- **Core Dump Analizi:** Sürecin çökmesi durumunda `/proc/<pid>/coredump` dosyası içinde hassas verilerin şifresiz olarak diske sızması riskidir.
- **Swap Sızıntısı:** İşletim sisteminin bellek sayfalarını diske yazarken şifresiz veriyi kalıcı depolama birimine taşıması riskidir.
- **Yan Kanal Gözlemleri:** Bellek erişim zamanlamalarının ve CPU önbellek durumlarının analiziyle kriptografik verilere dair bilgi sızdırılması riskidir.

---

## 2. Sistem Mimarisi ve IPC Akış Şeması

Proje, iki temel bileşenin POSIX sinyal hatları üzerinden asenkron süreçler arası iletişim (IPC) kurması esasına dayanmaktadır:

1. **bgt210_kurban (C İkili Süreci):** Hassas veriyi RAM'de sürekli maskeli tutan ve işletim sisteminden sinyal bekleyen çekirdek uygulama.
2. **bgt210_monitor.py (Python Telemetri Betiği):** Süreci procfs üzerinden izleyen ve doğrulamayı tetikleyen admin aracı.

```
┌─────────────────────────────────────────────┐
│             bgt210_kurban (C)               │
│                                             │
│  ┌──────────────┐    ┌──────────────────┐   │
│  │ SecureBuffer │    │  Signal Handler  │   │
│  │              │    │  (sigusr1_handler│   │
│  │ masked_data[]│◄───│  )               │   │
│  │ XOR(0x5A)    │    │                  │   │
│  │ is_exposed   │    │  1. Maskesini aç │   │
│  │ expose_time  │    │  2. Kullan (5sn) │   │
│  └──────────────┘    │  3. secure_zero  │   │
│                      │  4. Yeniden mask │   │
│                      └──────────────────┘   │
│                                             │
│  ┌──────────────────────────────────────┐   │
│  │  Ana Döngü: is_exposed izle, logla   │   │
│  └──────────────────────────────────────┘   │
└──────────────────────┬──────────────────────┘
                       │ SIGUSR1
                       │ os.kill(pid, SIGUSR1)
┌──────────────────────▼──────────────────────┐
│           bgt210_monitor.py                 │
│                                             │
│  ProcessFinder  → /proc taraması (PID)      │
│  ProcessStatus  → /proc/<pid>/status        │
│  EventRecorder  → zaman damgalı olay logu   │
│                                             │
│  ⚠ Bellek okuma / ptrace / /proc/mem YOK   │
└─────────────────────────────────────────────┘
```

---

## 3. Veri Yaşam Döngüsü

Hassas verinin süreç boyunca takip ettiği yaşam döngüsü kronolojik olarak aşağıda modellenmiştir:

```
Başlatma
   │
   ▼
[Düz Metin] ──XOR(0x5A)──► [masked_data] ── RAM'de kalıcı (maskeli)
                                  │
                          SIGUSR1 geldiğinde
                                  │
                                  ▼
                         [temp_plain] ← XOR(0x5A) ← [masked_data]
                              │
                         Kullan / Göster
                              │
                         sleep(5)
                              │
                         secure_zero(temp_plain)   ← Stack temizliği
                              │
                         is_exposed = 0
                              │
                         [masked_data] RAM'de hâlâ mevcut (maskeli)
                              │
                    Program sonlanırken
                              │
                         secure_zero(&g_secure_buf)
```

---

## 4. Teknik Bileşenlerin Analizi

### 4.1 XOR Maskeleme

Dinamik de-obfuscation süreçlerinde kullanılan temel döngü:

```c
dst[i] = src[i] ^ key;
```

- **Avantajları:** Hesaplama maliyeti O(n) seviyesinde olup son derece hızlıdır. Simetrik bir işlem olduğu için aynı fonksiyon hem maskeleme hem de maskeyi kaldırma amacıyla kullanılabilir.
- **Sınırlılıklar:** Kriptografik bir koruma sağlamaz; statik analizle anahtarı (`0x5A`) tespit eden bir analist veriyi tamamen deşifre edebilir.
- **Gerçek Dünya Alternatifi:** Endüstriyel standartlarda `AES-256-GCM` veya `ChaCha20-Poly1305` gibi kimlik doğrulamalı şifreleme modları tercih edilir.

### 4.2 Güvenli Bellek Sıfırlama (Dead-Store Elimination Kalkanı)

Derleyiciler kod optimizasyonu yaparken (`-O2`, `-O3`), "programın ilerleyen adımlarında bir daha okunmayacak olan" bellek alanlarına yapılan yazma çağrılarını gereksiz görerek silebilir. Buna **Dead-Store Elimination** denir ve adli bilişim analizlerinde verinin RAM'de kalmasına neden olan önemli bir güvenlik açığıdır.

Bu durumu engellemek adına `volatile` işaretçi dönüşümü kullanılmıştır:

```c
static void secure_zero(void *ptr, size_t len) {
    volatile unsigned char *p = (volatile unsigned char *)ptr;
    while (len--) { *p++ = 0; }
}
```

`volatile` anahtar kelimesi, derleyiciye bu bellek alanına yapılacak her atamanın optimizasyon süreçlerinden muaf tutularak donanıma doğrudan işlenmesi gerektiğini zorunlu kılar.

### 4.3 Sinyal İşleme (POSIX sigaction)

Yazılımda, eski ve taşınabilirlik sorunları barındıran `signal()` fonksiyonu yerine modern `sigaction()` yapısı tercih edilmiştir.

| Özellik | `signal()` | `sigaction()` |
|---|---|---|
| Taşınabilirlik | Sınırlı / Belirsiz | POSIX Standartlarında |
| Handler Sıfırlama | Bazı sistemlerde otomatik sıfırlanır | `sa_flags` ile tam kontrol |
| Bloklanacak Sinyaller | Ayarlanamaz | `sa_mask` ile dinamik ayarlanabilir |
| Kesilen Çağrıları Sürdürme | Yok | `SA_RESTART` desteği mevcut |

> **Async-Signal-Safe Kısıtı:** Sinyal handler fonksiyonları çalışırken işletim sisteminin normal akışı kesilir. Bu nedenle handler içinde `printf()` veya `malloc()` gibi güvensiz fonksiyonlar çağrılamaz. Projede yalnızca alt seviye `write()` ve `sleep()` çağrıları kullanılmıştır.

### 4.4 /proc Tabanlı Gözlem Metodolojisi

Python monitör betiği, işletim sisteminin saf yeteneklerini kullanarak tamamen metin tabanlı adli loglama gerçekleştirir:

- `/proc/<pid>/status` — Sürecin anlık durumu, bellek tüketimi (`VmRSS`, `VmSize`) ve iş parçacığı sayıları.
- `/proc/<pid>/comm` — Sürecin sistemdeki doğrulanmış kısa adı.

> ⚠️ **Tasarım Kararı:** Monitör yazılımı, yetkisiz veri sızıntılarına benzememek adına `/proc/<pid>/mem`, `/proc/<pid>/maps` dosyalarına erişmez ve `ptrace()` API çağrısı gerçekleştirmez.

---

## 5. Güvenlik Kazanımları Matrisi

| Tehdit Vektörü | Azaltma Mekanizması | Etkinlik Derecesi |
|---|---|---|
| Bellekte Sürekli Düz Metin Kalması | Dinamik XOR Maskeleme | Orta (Pedagojik Seviye) |
| Core Dump Sızıntıları | `secure_zero` + `prctl(PR_SET_DUMPABLE, 0)` | Yüksek |
| Derleyici Optimizasyon Hataları | `volatile` Pointer Çevrimi | Yüksek |
| Uzun Süreli Bellek Erişimi | 5 Saniyelik Zaman Penceresi Kontrolü | Yüksek |
| Süreç Kapanış Kalıntıları | `secure_zero(&g_secure_buf)` | Yüksek |

---

## 6. Sınırlılıklar ve Gelecek Çalışmalar

- **.rodata Segment Sızıntısı:** Kod içerisindeki string literalleri derleme sonrasında ELF binary dosyasının `.rodata` bölümüne yazılır ve `strings` komutuyla statik olarak saptanabilir. Gerçek dünya senaryolarında bu veri runtime anında şifreli bir konfigürasyondan veya ağ üzerinden çekilmelidir.

- **Takas Alanı (Swap) Güvenliği:** RAM sayfalarının diske yazılmasını tamamen engellemek adına gelecek çalışmalarda `mlock()` veya `mlockall(MCL_CURRENT)` sistem çağrıları entegre edilmelidir.

- **Sinyal Güvenliği:** `sleep()` fonksiyonunun async-signal-safe olmamasından ötürü, handler yapılarında daha hassas zamanlamalar için `nanosleep()` mimarisine geçilmelidir.

---

## 7. Referanslar

- CERT C Coding Standard: MEM03-C, SIG30-C, SIG31-C
- POSIX.1-2008: `sigaction(2)`, `mlock(2)`, `explicit_bzero(3)`
- Linux `proc(5)` kılavuz sayfaları
