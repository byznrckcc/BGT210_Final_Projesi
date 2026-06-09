# Operasyon: Fantom Hafıza — Canlı Bellek Adli Analizi
### Dijital Adli Bilişim ve Olay Müdahalesi (DFIR) Teknik Raporu

**Kurum:** İstinye Üniversitesi — Bilişim Güvenliği Teknolojisi  
**Ders:** Tersine Mühendislik BGT210
**Danışman:** Keyvan Arasteh  
**Rapor Türü:** Final Projesi Teknik Raporu  
**Hazırlayan:** Beyzanur Çakıcı
---

## Yönetici Özeti

Bu rapor, disk üzerinde kalıcı iz bırakmayan, yalnızca çalışma zamanı belleğinde (runtime memory) var olan gizli verilerin adli yöntemlerle tespitine ilişkin bir laboratuvar çalışmasını belgelemektedir. Simüle edilen tehdit senaryosunda, hedef süreç standart hata ayıklama araçlarına karşı anti-analysis korumaları barındırmaktadır. Çalışma; bu koruma katmanlarının DFIR perspektifinden kavramsal analizi, Linux çekirdeğinin `/proc` sanal dosya sistemi üzerinden gerçekleştirilen salt-okunur bellek triage metodolojisi ve endüstri standardı araçların bu tür senaryolardaki rolü üzerine odaklanmaktadır.

---

## 1. Gelişmiş Tehditlerde Anti-Analysis ve Anti-Debugging Mekanizmaları

### 1.1 Kavramsal Çerçeve

Gelişmiş kalıcı tehditler (APT) ve sofistike zararlı yazılımlar, analistlerin inceleme sürecini yavaşlatmak veya tamamen engellemek amacıyla çeşitli **anti-analysis** teknikleri uygulamaktadır. Bu teknikler genel olarak üç kategoride sınıflandırılabilir:

| Kategori | Açıklama | Örnek Teknikler |
|---|---|---|
| **Anti-Debugging** | Hata ayıklayıcı varlığını tespit edip davranışı değiştirme | ptrace kontrolü, IsDebuggerPresent, timing check |
| **Anti-Forensics** | Adli araçların erişimini engelleme veya yanıltma | Bellek şifreleme, sahte yığın izleri, süreç gizleme |
| **Anti-VM / Anti-Sandbox** | Sanal ortamda çalıştığını tespit edip eylemsiz kalma | CPUID denetimi, donanım parmak izi, gecikme döngüleri |

### 1.2 Linux'ta ptrace Sistemi ve Anti-Debugging Kullanımı

`ptrace` (process trace), Unix türevi işletim sistemlerinde hata ayıklayıcıların ve sistem izleme araçlarının bir süreci inceleyip kontrol edebilmesini sağlayan temel bir çekirdek sistem çağrısıdır. `gdb`, `strace`, `ltrace` ve `gcore` gibi araçlar bu arayüzü kullanır.

Zararlı yazılım geliştiricileri bu mekanizmayı tersine çevirerek **kendi kendini izleyen** (self-tracing) bir anti-debugging koruması oluşturabilir. Bunun klasik yöntemi `PTRACE_TRACEME` direktifinin kullanımıdır:

```
ptrace(PTRACE_TRACEME, 0, NULL, NULL)
```

Bu çağrı, işletim sistemine söz konusu sürecin izlenmeye hazır olduğunu bildirir; aynı zamanda **bir süreci yalnızca tek bir izleyicinin takip edebileceği** kuralını da devreye sokar. Eğer zararlı yazılım bu çağrıyı başarıyla gerçekleştirirse, kendisi zaten bir izleyici (tracer) tarafından takip edilir konuma geçer. Dışarıdan gelen bir hata ayıklayıcı aynı süreci ikinci kez izlemeye çalıştığında, çekirdek bu isteği aşağıdaki hata ile reddeder:

```
ptrace: Operation not permitted (errno: EPERM)
```

Bu hata mesajı, bir güvenlik açığını değil; Linux çekirdeğinin bilinçli tasarım kararını — bir sürecin en fazla bir izleyiciye sahip olabileceği ilkesini — yansıtmaktadır.

### 1.3 Standart Adli Araçların Bu Noktada Yetersiz Kalması

`gcore` ve benzer kullanıcı-uzayı (user-space) araçları, çalışma mekanizmaları gereği `ptrace` arayüzüne bağımlıdır. Bu araçlar bellekte okuma işlemi gerçekleştirebilmek için önce hedef sürece bağlanmak (`attach`) zorundadır. Yukarıda açıklanan self-tracing mekanizması bu bağlantıyı çekirdek düzeyinde engellediğinden, söz konusu araçlar işlevini yerine getiremez.

Bu durum, DFIR analistlerinin sıklıkla karşılaştığı **"adli kör nokta"** senaryosunun tipik bir örneğidir: Tehdit aktörü, savunmacının araç setini hedef alarak analizin önünü tıkamaktadır. Yanıt, standart araçlarda ısrarcı olmak yerine **farklı bir erişim katmanına** geçmektir.

---

## 2. Canlı Sistem Hafıza Haritalaması ve Adli İnceleme

### 2.1 Linux /proc Sanal Dosya Sistemi Mimarisi

Linux çekirdeği, çalışan her süreç için `/proc/[PID]/` dizini altında sanal (bellekte tutulan, diskte bulunmayan) bir dosya sistemi oluşturur. Bu yapı, POSIX standartlarına uygun olarak çekirdeğin iç durumunu kullanıcı uzayına açan bir **arayüz katmanı** işlevi görür.

Bellek adli analizi açısından kritik öneme sahip iki sanal dosya şunlardır:

#### `/proc/[PID]/maps`
Sürecin sanal adres uzayının haritasını düz metin formatında sunar. Her satır bir bellek bölgesini (region/segment) tanımlar:

```
Başlangıç-Bitiş    İzinler  Offset  Aygıt  İnode  Yol/Açıklama
55a3b2000000-55a3b2001000  r-xp  00000000  08:01  1234567  /usr/bin/hedef
7fff8a000000-7fff8a021000  rwxp  00000000  00:00  0        [stack]
```

**İzin bayrakları** adli açıdan özellikle değerlidir:
- `r`: Okunabilir segment — bellek dökümü için uygun
- `w`: Yazılabilir segment — çalışma zamanı verilerini barındırabilir
- `x`: Çalıştırılabilir segment — kod bölgesi
- `p`/`s`: Özel (copy-on-write) / Paylaşımlı eşleme

#### `/proc/[PID]/mem`
Sürecin sanal bellek uzayına doğrudan erişim sağlayan ikili (binary) bir sanal aygıt dosyasıdır. Bu dosyada belirli bir adresten okuma yapabilmek için önce o adresin `/proc/[PID]/maps` üzerinden geçerli ve okunabilir olduğu doğrulanmalı, ardından `lseek()` ile hedef adrese konumlanılarak `read()` gerçekleştirilmelidir.

**Kritik ayrım:** `/proc/[PID]/mem` erişimi, `ptrace` sistemi çağrısını **kullanmaz**. Erişim kontrolü doğrudan çekirdek tarafından standart POSIX dosya izinleri ve süreç sahipliği üzerinden yönetilir. Bu mimari fark, adli analistin `ptrace` engelini devre dışı bırakmadan, bu yolu tamamen **atlamak** suretiyle belleğe ulaşmasını mümkün kılar.

### 2.2 Salt-Okunur Bellek Triage Metodolojisi

DFIR perspektifinden `/proc` tabanlı bellek incelemesi şu adımları izler:

**Aşama 1 — Süreç Tanımlama**  
Hedef sürecin PID değeri `ps`, `pgrep` veya `/proc` dizin listesi aracılığıyla tespit edilir.

**Aşama 2 — Adres Uzayı Haritalaması**  
`/proc/[PID]/maps` dosyası ayrıştırılarak okunabilir (`r` iznine sahip) bellek segmentlerinin başlangıç-bitiş adresleri kataloglanır. Adli öncelik sıralaması genel olarak şöyledir: yığın (heap) → yığın (stack) → anonim eşlemeler → yüklü kütüphaneler.

**Aşama 3 — Hedefli Bellek Okuma**  
`/proc/[PID]/mem` dosyası ikili modda açılır. Her segment için `lseek()` ile hedef adrese konumlanılır ve `read()` ile veri alınır. Segmentin tamamı ya da yalnızca ilgili kısımları alınabilir (hedefli triage).

**Aşama 4 — Artefakt Analizi**  
Elde edilen ham bellek blokları; dize (string) arama, entropi analizi, bilinen imza eşleştirmesi ve şüpheli veri kalıplarının tespiti amacıyla incelenir.

**Aşama 5 — Belgeleme**  
Triage zaman damgası, incelenen PID, okunan segment aralıkları ve tespit edilen artefaktlar kayıt altına alınır.

### 2.3 Adli Geçerlilik ve Sınırlılıklar

Bu yöntem, **canlı sistem adli bilişimi** (live forensics) kategorisinde değerlendirilir ve aşağıdaki özelliklerle tanımlanır:

- **Salt-okunur:** Hedef sürecin belleğine herhangi bir yazma işlemi gerçekleştirilmez, süreç askıya alınmaz, akışı değiştirilmez.
- **Bütünlük kısıtı:** Okunma süresi boyunca bellek değişebilir (race condition). Bu nedenle elde edilen veri, tam bir anlık görüntü (snapshot) değil, **olasılıksal bir triage kanıtı** olarak değerlendirilmelidir.
- **Yetki gerekliliği:** Erişim, hedef sürecin sahibiyle eşdeğer veya daha yüksek ayrıcalık gerektirmektedir (`root` ya da eşit UID).

---

## 3. Volatility ve Canlı Bellek Forensics Endüstri Standartları

### 3.1 Bellek Adli Bilişiminde Katmanlı Yaklaşım

Endüstri standardı bellek adli bilişimi, erişim karmaşıklığına göre kademeli bir araç hiyerarşisi benimser. Standart kullanıcı uzayı araçlarının yetersiz kaldığı durumlarda daha derin erişim katmanlarına geçilir:

```
Katman 1 (En az müdahaleci): /proc tabanlı triage
        ↓  (yetersiz kalırsa)
Katman 2: LiME ile çekirdek modülü bellek dökümü
        ↓  (yetersiz kalırsa)
Katman 3: Volatility ile çevrimdışı hafıza analizi
        ↓  (yetersiz kalırsa)
Katman 4: Donanım tabanlı erişim (DMA, JTAG)
```

### 3.2 LiME — Linux Memory Extractor

**LiME** (Linux Memory Extractor), çekirdeğe yüklenebilir bir modül (Loadable Kernel Module — LKM) olarak çalışır ve fiziksel RAM içeriğini kullanıcı tanımlı bir formatta (raw, padded, lime) diske veya ağ üzerinden aktarır. Anti-debugging kontrolleri gibi kullanıcı uzayı mekanizmalarını tamamen atlayarak doğrudan fiziksel belleğe erişir.

DFIR iş akışında LiME'nin tipik kullanım senaryosu şöyledir:
1. LiME modülü hedef sistem üzerinde derlenir veya önceden derlenmiş binary aktarılır.
2. Modül `insmod` ile çekirdeğe yüklenir; bellek dökümü ağ soketi veya yerel dosya olarak alınır.
3. Modül kaldırılır; elde edilen ham bellek görüntüsü analist ortamına aktarılır.

### 3.3 Volatility Framework

**Volatility**, ham bellek görüntüleri üzerinde çalışan, platform bağımsız (Windows, Linux, macOS) açık kaynaklı bir bellek adli analiz çerçevesidir. LiME ile alınan görüntüler doğrudan Volatility'ye girdi olarak verilebilir.

Bu proje bağlamında öne çıkan Volatility eklentileri şunlardır:

| Eklenti | İşlev |
|---|---|
| `linux.pslist` | Çalışan süreçleri listeler |
| `linux.proc_maps` | Seçili sürecin bellek eşlemesini çıkarır |
| `linux.malfind` | Yüksek entropili veya izinsiz çalıştırılabilir bölgeleri işaretler |
| `linux.strings` | Bellek bölgelerinde dize (string) arama yapar |
| `linux.dumpfiles` | Bellekten dosya/segment döker |

### 3.4 Araç Seçim Kriterleri

Kontrollü laboratuvar ortamında gerçekleştirilen bu çalışmada `/proc` tabanlı triage yönteminin tercih edilmesinin nedenleri:

- **Kurulum gerektirmez:** LiME, çekirdeğe modül yüklenmesini ve uyumlu binary'nin önceden hazırlanmasını gerektirir; bu adım zaman ve sistem erişimi gerektirir.
- **Eğitim odaklılık:** `/proc` arayüzü, Linux çekirdek bellek mimarisini doğrudan gözlemleyerek anlamayı sağlar; üst seviye araçların soyutladığı katmanı görünür kılar.
- **Orantılılık:** Tespit hedefi belirli bir süreçteyken sistemin tamamının belleğini dökmek gereksiz yere büyük adli artefakt üretir.

Gerçek bir olay müdahale senaryosunda, özellikle gelişmiş süreç gizleme (process hiding) veya çekirdek düzeyinde rookit varlığından şüpheleniliyorsa, LiME + Volatility kombinasyonu zorunlu hale gelecektir.

---

## 4. Kontrollü Laboratuvar Ortamı ve Etik Çerçeve

### 4.1 Çalışma Ortamı

Bu proje kapsamındaki tüm teknik faaliyetler aşağıdaki koşullar altında gerçekleştirilmiştir:

- **İzole ağ:** İnternetten ve kurumsal ağdan tamamen yalıtılmış sanal makine ortamı
- **Sahip olunan sistemler:** Analiz edilen tüm süreçler ve sistem kaynakları araştırmacı tarafından oluşturulmuş ve işletilmiştir; üçüncü şahıslara ait herhangi bir sistem incelenmemiştir
- **Kontrollü kapsam:** Geliştirilen araç ve teknikler yalnızca bu proje kapsamında tanımlanan simülasyon senaryosuna uygulanmıştır
- **Veri imhası:** Çalışma kapsamında elde edilen tüm bellek dökümü artefaktları proje tamamlandığında güvenli biçimde imha edilecektir

### 4.2 Yasal ve Etik Çerçeve

Türkiye'de Bilişim Suçları kapsamında 5237 sayılı Türk Ceza Kanunu'nun 243. ve 244. maddeleri, izin alınmadan bilişim sistemlerine erişimi ve bu sistemlere zarar vermeyi suç olarak tanımlamaktadır. Bu çalışma söz konusu düzenlemelerin kapsamı dışında kalmaktadır; zira:

- Yalnızca araştırmacının kendisine ait sistemler analiz edilmiştir.
- Hiçbir üretim sistemi, gerçek kullanıcı verisi veya üçüncü taraf altyapısı bu çalışmaya dahil edilmemiştir.
- Tüm faaliyetler İstinye Üniversitesi akademik çerçevesinde, danışman gözetiminde yürütülmüştür.

### 4.3 Sorumlu Araştırma İlkeleri

Bu çalışma; **"responsible disclosure"** ve **"ethical hacking"** ilkelerine uygun olarak, yalnızca savunma kapasitesini artırmaya yönelik bilgi üretme amacıyla tasarlanmıştır. Çalışmada elde edilen bulgular ve geliştirilen metodoloji:

- Zararlı yazılım analizi ve olay müdahalesi alanında savunmacı yetkinlik kazandırmaya
- Anti-analysis tekniklerinin adli bilişim eğitimi bağlamında anlaşılmasına
- Siber güvenlik araştırmacılarının benzer senaryolara hazırlıklı olmasına

katkı sağlamak amacıyla belgelenmiştir.

---

## 5. Sonuç ve Öneriler

Bu çalışma, gelişmiş tehdit aktörlerinin kullandığı anti-analysis mekanizmalarının standart adli araç setini nasıl devre dışı bırakabileceğini ve bu durumda DFIR analistlerinin Linux çekirdeğinin farklı erişim katmanlarını nasıl kullanabileceğini göstermiştir. Elde edilen bulgular üç temel çıkarımla özetlenebilir:

**1. Derinlemesine Savunma İhtiyacı:** Tek bir araç veya yönteme bağımlı adli süreçler, hedefe yönelik anti-analysis teknikleri karşısında yetersiz kalmaktadır. DFIR ekiplerinin katmanlı araç hiyerarşisi geliştirmesi kritik önem taşımaktadır.

**2. Çekirdek Mimarisi Bilgisinin Değeri:** `/proc` sanal dosya sistemi gibi temel Linux çekirdek mekanizmalarının derinlemesine anlaşılması, yüksek seviyeli araçların soyutladığı sorunları çözme kapasitesi sunmaktadır.

**3. Canlı Adli Bilişimin Önemi:** Diskte iz bırakmayan tehdit kategorisinin yaygınlaşmasıyla birlikte, geleneksel disk imajı temelli adli bilişim tek başına yetersiz kalmaktadır. Canlı bellek triage, modern DFIR metodolojisinin vazgeçilmez bir bileşeni haline gelmiştir.

---

## Referanslar

1. Russinovich, M., & Solomon, D. (2012). *Windows Internals*. Microsoft Press.
2. Ligh, M. H., Case, A., Levy, J., & Walters, A. (2014). *The Art of Memory Forensics*. Wiley.
3. Carrier, B. (2005). *File System Forensic Analysis*. Addison-Wesley.
4. Volatility Foundation. (2023). *Volatility 3 Documentation*. https://volatility3.readthedocs.io
5. Linux Kernel Documentation. (2024). *proc filesystem*. https://kernel.org/doc/html/latest/filesystems/proc.html
6. Kornblum, J. (2002). *Linux Memory Forensics*. SANS Institute Reading Room.
7. Android Open Source Project. (2023). *LiME — Linux Memory Extractor*. https://github.com/504ensicsLabs/LiME
8. NIST SP 800-86. (2006). *Guide to Integrating Forensic Techniques into Incident Response*. National Institute of Standards and Technology.

---

*Bu rapor, İstinye Üniversitesi Bilişim Güvenliği Teknolojisi programı final projesi kapsamında, tamamen akademik ve savunma odaklı amaçlarla hazırlanmıştır.*
