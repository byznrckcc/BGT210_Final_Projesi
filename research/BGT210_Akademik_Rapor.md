# BGT210 – TERSİNE MÜHENDİSLİK DERSİ DÖNEM FİNAL PROJESİ AKADEMİK RAPORU

## 🛰️ Operasyon: Fantom Hafıza (Phase-II)
### Çalışma Zamanı ASLR Entropi Hasadı ve Devingen Yığın Atlamalı (Heap Hopping) Polimorfik Bellek Güvenliği Yönetimi

---

## 👤 Akademik Künye

| Alan | Bilgi |
|---|---|
| **Araştırmacı** | Beyzanur Çakıcı (Öğrenci No: 2420191032) |
| **Bölüm** | Bilişim Güvenliği Teknolojisi |
| **Kurum** | İstinye Üniversitesi, Mühendislik ve Doğa Bilimleri Fakültesi |
| **Proje Danışmanı** | Öğr. Gör. Keyvan Arasteh |
| **Ders** | BGT210 - Tersine Mühendislik |

---

## 📄 Özet (Abstract)

Modern dinamik analiz araçları ve adli bilişim metodolojileri, hassas verilerin RAM üzerindeki kalıcılık ömrünü hedef almaktadır. Bu çalışmada; statik imza tarayıcılarını (`strings`), tersine mühendislik platformlarını (`Ghidra`, `IDA Pro`) ve kaba kuvvet bellek dökümü araçlarını (`gcore`, `dd`) proaktif olarak sabote eden, disk üzerinde dosya izi bırakmayan (Fileless) bir bellek içi savunma mimarisi tasarlanmıştır.

Geliştirilen sistem, her tetikleme döngüsünde Linux çekirdeğinin yığın adres uzayı entropisini hasat ederek anlık polimorfik şifreleme anahtarı üretir. Eş zamanlı yürütülen **Devingen Yığın Atlaması (Heap Hopping)** algoritması ile deşifre edilen verinin RAM üzerinde sabit adreste kilitli kalması engellenerek dinamik adres kancalama (`ptrace`/GDB hooks) süreçleri kör edilmiştir. Beş saniyelik mikro operasyon penceresi kapandığında `secure_zero()` fonksiyonu ile bellek alanları kalıcı olarak kazınmıştır. Tüm telemetri akışı asenkron bir SOC Dashboard mimarisi üzerinden gerçek zamanlı doğrulanmıştır.

---

## 1. Giriş ve Tehdit Modellemesi

Tersine mühendislik süreçlerinde statik analiz, ikili dosyanın (ELF/EXE) disk üzerindeki kod segmentlerinin decompile edilmesi esasına dayanırken; dinamik analiz, yazılımın runtime esnasındaki bellek haritasını (VMM/Procfs) inceler. APT aktörleri kritik yapılandırma bloklarını diskte gizlese dahi, süreç ayağa kalktığı an veriler RAM üzerinde şifresiz hale gelmektedir. Bu durum yazılımı dört temel tehdit vektörüne açık bırakır:

- **Cold-Boot Vektörü:** DRAM hücrelerinin sistem enerjisi kesilse dahi saniyelerce veri kalıntısı (remanence) bırakması.
- **Kaba Kuvvet Bellek Dökümü:** `gcore` veya `/proc/<pid>/mem` üzerinden sürecin RAM haritasının diske indirilmesi.
- **Core Dump Sızıntıları:** Sürecin beklenmedik çökmesi (SIGSEGV) durumunda bellek içeriğinin `coredump` dosyalarına yazılması.
- **Dinamik Bellek İzleme:** GDB veya benzeri araçlarla bellek adreslerine donanımsal kesme noktası (Hardware Breakpoint) konulması.

---

## 2. Matematiksel ve Teorik Altyapı

### 2.1 ASLR Entropi Tabanlı Çalışma Zamanı Anahtar Türetimi

Sistem deterministik (sabit) anahtarlar kullanmaz. İşletim sisteminin süreç başladığında rastgele atadığı sanal bellek adres uzayından (ASLR) faydalanır. Anlık polimorfik anahtar ($K_{\text{dyn}}$); yığın adresi ($Addr_{\text{stack}}$) ile milisaniyelik zaman damgasının ($\text{Time}_{\text{ms}}$) XOR işlemi ve ardından bir karma fonksiyonundan geçirilmesiyle elde edilir:

$$K_{\text{dyn}} = \mathcal{H}(Addr_{\text{stack}} \oplus \text{Time}_{\text{ms}})$$

Deşifre işlemi simetrik polimorfizm ilkesine dayanır:

$$C_i = P_i \oplus K_{\text{dyn}}$$
$$P_i = C_i \oplus K_{\text{dyn}}$$

Burada $C_i$ şifreli byte bloğunu, $P_i$ düz metin (plaintext) verisini temsil eder. Her sinyal enjeksiyonunda anahtar tamamen değiştiği için saldırgan durağan bir deşifre anahtarı elde edemez.

### 2.2 Devingen Yığın Atlaması (Heap Hopping) Algoritması

Analistlerin en sık kullandığı yöntem, bellek dökümü alıp hedef verinin adres ofsetini sabitlemektir. Heap Hopping bu izleme tekniğini şu adımlarla bozar:

1. Rastgele bir delta ofset boyutu ($L_{\Delta}$) hesaplanır.
2. İşletim sisteminden ardışık olmayan iki bağımsız bellek bloğu istenir:

$$\text{Block}_A = \text{malloc}(S_{\text{base}})$$
$$\text{Block}_B = \text{malloc}(S_{\text{base}} + L_{\Delta})$$

3. Veri, aradaki boşluklar atlanarak $\text{Block}_B$ üzerine yazılır.
4. Eski adres bloğu ($\text{Block}_A$) anında `secure_zero()` ile imha edilerek `free()` ilan edilir.

Bu döngü tekrarlandıkça hassas veri RAM üzerinde sürekli farklı adreslere taşınır. GDB üzerinden sabit bir adrese konulan watchpoint bir sonraki döngüde geçersiz bir bellek sayfasına işaret eder.

---

## 3. Derleyici Optimizasyon Kalkanı: Dead-Store Elimination Analizi

Güvenli yazılım geliştirmede kritik bir zafiyet, hassas veri kullanıldıktan sonra `memset(ptr, 0, len)` ile temizlendiğinin sanılmasıdır. Modern derleyiciler (`GCC`, `Clang`) optimizasyon süreçlerinde (`-O2`, `-O3`) veri akış analizi yapar. Temizlenen bellek alanına bir daha erişim yapılmıyorsa bu işlem **Dead-Store** olarak değerlendirilir ve derleme aşamasında silinir. Bu durum hassas verilerin süreç kapanana kadar RAM'de açık kalmasına yol açar.

Bu derleyici hatasını engellemek için `volatile` pointer dönüşümü kullanılmıştır:

```c
static void secure_zero(void *ptr, size_t len) {
    volatile unsigned char *p = (volatile unsigned char *)ptr;
    while (len--) { *p++ = 0; }
}
```

`volatile` anahtar kelimesi, derleyiciye bu bellek alanına yapılan her atamanın optimizasyon süreçlerinden muaf tutularak donanıma doğrudan işlenmesi gerektiğini zorunlu kılar.

---

## 4. Referanslar

- CERT C Coding Standard: MEM03-C, SIG30-C, SIG31-C
- POSIX.1-2008: `sigaction(2)`, `mlock(2)`, `explicit_bzero(3)`
- Linux `proc(5)` kılavuz sayfaları
- MITRE ATT&CK: T1055 (Process Injection), T1620 (Reflective Code Loading)

---

> Bu çalışma, İstinye Üniversitesi BGT210 - Tersine Mühendislik dersi final gereksinimleri doğrultusunda **Beyzanur Çakıcı** tarafından özgün olarak geliştirilmiştir. Akademik kaynak gösterilmeden kopyalanamaz.