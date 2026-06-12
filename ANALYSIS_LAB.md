# 🔬 ResearchLab: Siber Saldırı Analiz Raporu

Bu rapor, ResearchLab bünyesinde gerçekleşen kritik siber güvenlik olaylarının, saldırı vektörlerinin ve teknik etkilerinin akademik dökümünü içermektedir.

---

## 📦 1. Tedarik Zinciri & Altyapı Saldırıları

### 🚨 Vercel Tedarik Zinciri İhlali (2026-04)
* **Etki Skoru:** 98/100 (Kritik)
* **Zafiyet Tanımı:** Üçüncü taraf bir yapay zeka aracı olan `Context.ai` üzerinden sızdırılan OAuth token'ları vasıtasıyla gerçekleşen sofistike bir tedarik zinciri saldırısıdır. CVE kodu bulunmayan saf bir OAuth güven suistimalidir.
* **Saldırı Vektörü:** Üçüncü taraf OAuth token gaspı → Google Çalışma Alanı erişimi → Dahili sistem numaralandırma.
* **Sonuç:** Müşterilerin hassas olmayan ortam değişkenleri (environment variables) açığa çıkmıştır.

### 🚨 React2Shell — Ön Bellek RCE (CVE-2025-55182)
* **Etki Skoru:** 99/100 (Kritik)
* **Zafiyet Tanımı:** React'in Uçuş (Flight) protokolünde bulunan ve CVSS 10.0 değeri alan kritik bir güvensiz nesne serileştirmeden çıkarma (deserialization) kusurudur. React Server Components uygulayan her sunucuyu yetkisiz uzaktan kod yürütmeye (RCE) açık hale getirmiştir.
* **Saldırı Vektörü:** Hazırlanmış HTTP POST isteği → Uçuş Protokolü Deserialization → Kötü Amaçlı Nesne Enjeksiyonu → Tam Sunucu RCE.
* **Sonuç:** Ulus devlet aktörleri tarafından binlerce canlı sistem üzerinde kitlesel sömürü başlatılmıştır.

### 🚨 "GhostContainer" Kubernetes Kaçış (CVE-2025-8831)
* **Etki Skoru:** 97/100 (Kritik)
* **Zafiyet Tanımı:** CRI-O konteyner çalışma zamanı (runtime) bileşeninde, eBPF yük doğrulaması sırasında meydana gelen kritik bir yarış koşulu (Race Condition) zafiyetidir.
* **Saldırı Vektörü:** Kötü amaçlı eBPF yükü → CRI-O doğrulama yarış durumu → Ana makine (Node) kök (root) erişimi.
* **Sonuç:** Saldırganların izole konteyner ortamından kaçarak doğrudan Kubernetes düğümlerinde tam yetki elde etmesi sağlanmıştır.

## 🔐 2. Kimlik Doğrulama & API Yetki Sömürüleri

### 🚨 Supabase / PostgREST Rol Kaçakçılığı (CVE-2026-1044)
* **Etki Skoru:** 92/100 (Yüksek)
* **Zafiyet Tanımı:** PostgREST'in belirli JWT (JSON Web Token) iddialarını (claims) hatalı ayrıştırmasından kaynaklanan bir mantık kusurudur.
* **Saldırı Vektörü:** Hazırlanmış JWT iddiası → PostgREST parser mantık kusuru → Rol kaçakçılığı → RLS (Row-Level Security) Bypass.
* **Sonuç:** Saldırganlar standart Supabase dağıtımlarında satır düzeyinde güvenliği atlayarak `postgres` süper kullanıcı rollerini ele geçirmiştir.

### 🚨 Keycloak SAML İmza Baypası (CVE-2025-4421)
* **Etki Skoru:** 94/100 (Yüksek)
* **Zafiyet Tanımı:** Keycloak 26.x sürümlerinde bulunan kritik bir XML imza sarmalama (XSW - XML Signature Wrapping) güvenlik açığıdır.
* **Saldırı Vektörü:** Manipüle edilmiş SAML iddiası → XML İmza Sarma (XSW) → SSO (Single Sign-On) Bypass.
* **Sonuç:** Saldırganların sahte SAML iddiaları üreterek kurumsal tekli oturum açma mekanizmalarını tamamen atlatması sağlanmıştır.

### 🚨 AWS Cognito "Token Kaçakçılığı" Yetki Yükseltme
* **Etki Skoru:** 91/100 (Yüksek)
* **Zafiyet Tanımı:** OAuth akışı esnasında farklı AWS API Ağ Geçitleri (API Gateways) arasındaki hedef kitle (`aud`) iddialarının doğrulanmaması kusurudur.
* **Saldırı Vektörü:** Yakalanan Cognito JWT → "aud" değerinin admin ağ geçidi için değiştirilmesi (Swap) → API Gateway yetkilendirme baypası.
* **Sonuç:** Standart yetkideki kullanıcıların, yalnızca yöneticilere özel (admin-only) kritik uç noktalara (endpoints) erişmesine yol açmıştır.

## 🧠 3. Gelişmiş RCE & Çekirdek Bellek Saldırıları

### 🚨 Nginx HTTP/3 QUIC Bellek Taşması (CVE-2026-0211)
* **Etki Skoru:** 98/100 (Kritik)
* **Zafiyet Tanımı:** Nginx'in QUIC uygulamasında, internete açık yük dengeleyicilerinde kimlik doğrulama öncesi uzaktan kod yürütülmesine (Pre-auth RCE) yol açan bir yığın tampon taşması (Stack Buffer Overflow) zafiyetidir.
* **Saldırı Vektörü:** Kötü biçimlendirilmiş (Malformed) QUIC bağlantı kimliği → Nginx yığın tampon taşması → Kimlik doğrulama öncesi RCE.
* **Sonuç:** Saldırganların hedef sunucularda en yüksek yetkiyle kod çalıştırması tetiklenmiştir.

### 🚨 WebAssembly (Wasm) Sandbox Kaçışı (CVE-2025-7733)
* **Etki Skoru:** 96/100 (Kritik)
* **Zafiyet Tanımı:** JIT (Just-In-Time) derlenmiş WebAssembly tablolarının manipüle edilmesiyle, Chrome ve Edge tarayıcılarındaki Wasm doğrusal bellek kum havuzundan (sandbox) çıkılmasına izin veren bir zafiyettir.
* **Saldırı Vektörü:** Kötü amaçlı Wasm ikili dosyası → JIT derleyici masa manipülasyonu → Doğrusal hafıza kaçışı → Sıfır tıklama RCE.
* **Sonuç:** Kullanıcı etkileşimi gerektirmeden tarayıcı üzerinde tam sistem kontrolü elde edilmiştir.

### 🚨 iOS 19 "Silent Message" iMessage (CVE-2026-1199)
* **Etki Skoru:** 100/100 (Kritik)
* **Zafiyet Tanımı:** Apple ImageIO bileşeninde yüksek derecede sıkıştırılmış HEIC görüntülerini ayrıştırırken meydana gelen bir bellek bozulması (Memory Corruption) güvenlik açığıdır.
* **Saldırı Vektörü:** iMessage aracılığıyla kötü niyetli HEIC görüntü gönderimi → ImageIO bellek bozulması → Sıfır tıklamalı (Zero-Click) istismar zinciri.
* **Sonuç:** Devlet destekli aktörler tarafından hedef cihazlara kullanıcı ruhu duymadan casus yazılım (Spyware) enjekte edilmiştir.

### 🚨 Cloudflare Workers Bellek Sızıntısı (CVE-2025-9920)
* **Etki Skoru:** 95/100 (Kritik)
* **Zafiyet Tanımı:** Cloudflare Workers altyapısındaki V8 motorunun izole durum (Isolate Context) geçişleri sırasında meydana gelen bir bellek sızıntısı (Context Bleeding) hatasıdır.
* **Saldırı Vektörü:** Yüksek eşzamanlı V8 bağlam anahtarı → İzole durum kanaması → Çapraz kiracı (Cross-tenant) bellek okuma.
* **Sonuç:** Eşzamanlı

## 🤖 4. Yapay Zeka, API & EDR Savunma Hattı Saldırıları

### 🚨 VSCode GitHub Copilot Prompt Injection (2025-08)
* **Etki Skoru:** 93/100 (Yüksek)
* **Zafiyet Tanımı:** `README.md` dosyalarının içerisine gizlenmiş talimat enjeksiyonları (Prompt Injection) vasıtasıyla geliştirici araçlarının manipüle edilmesidir.
* **Saldırı Vektörü:** `README.md` içine gizli istem → Copilot bağlam ayrıştırma (Context Parsing) → Görünmez VSCode terminal komut yürütme.
* **Sonuç:** Saldırganların hazırladığı zararlı bir depo (repository) klonlandığında, VSCode Copilot uzantısının kandırılarak yerel terminalde yetkisiz komut çalıştırması sağlanmıştır.

### 🚨 "LLM-Jack" RAG Veri Zehirlenmesi Saldırısı (2025-09)
* **Etki Skoru:** 90/100 (Yüksek)
* **Zafiyet Tanımı:** Şirket içi yapay zeka asistanlarının kullandığı RAG (Retrieval-Augmented Generation) vektör veri tabanlarına yönelik bir veri zehirlenmesi (Data Poisoning) saldırısıdır.
* **Saldırı Vektörü:** Kamu açık özgeçmişlere (PDF) görünmez metin eklenmesi → RAG vektör veri yutma → LLM istem manipülasyonu.
* **Sonuç:** Yapay zeka asistanının manipüle edilerek şirkete ait gizli İnsan Kaynakları (İK) verilerini dışarıya sızdırması (Data Exfiltration) sağlanmıştır.

### 🚨 CrowdStrike Falcon EDR "BlindSpot" Atlatması (2025-10)
* **Etki Skoru:** 93/100 (Yüksek)
* **Zafiyet Tanımı:** CrowdStrike Falcon tarafından güvenlik izlemesi için kullanılan Windows Çekirdek (Kernel) ETW (Event Tracing for Windows) kancalarının tersine çevrilmesi saldırısıdır.
* **Saldırı Vektörü:** Savunmasız imzalı sürücü kullanımı (BYOVD - Bring Your Own Vulnerable Driver) → Kernel hafızası yazma hakları → ETW kanca yama (patching) → EDR körlüğü.
* **Sonuç:** Gelişmiş zararlı yazılımların EDR sistemine yakalanmadan çekirdek seviyesinde (Kernel space) iz bırakmadan çalışması sağlanmıştır.

### 🚨 "Shadow-API" GraphQL Toplu Saldırı (Stripe/Shopify)
* **Etki Skoru:** 88/100 (Orta)
* **Zafiyet Tanımı:** Yanlış yapılandırılmış federasyon GraphQL ağ geçitleri (gateways) üzerinde sınırsız takma ad (alias) kullanımına izin veren bir API suistimalidir.
* **Saldırı Vektörü:** GraphQL sınırsız takma ad enjeksiyonu → Arka uç (Backend) kaynak tükenmesi → DoS / Cüzdan Reddi (DoW) ve veri kazıma.
* **Sonuç:** Saldırganlar, platformların oran sınırlarına (Rate limits) takılmadan kitlesel veri kazıma ve servis dışı bırakma saldırıları gerçekleştirmiştir.

### 🚨 Tesla Bilgi-Eğlence Bluetooth Root Erişimi (2025-12)
* **Etki Skoru:** 95/100 (Kritik)
* **Zafiyet Tanımı:** Tesla MCU (Media Control Unit) baz bandı (baseband) ürün yazılımının Bluetooth eşleştirme yığınında (pairing stack) bulunan bir yığın taşması (Stack Overflow) zafiyetidir.
* **Saldırı Vektörü:** Kötü niyetli Bluetooth eşleştirme talebi → Baseband yığın taşması → MCU Kök (Root) yetkisi → Araç kapı kilitlerinin uzaktan açılması.
* **Sonuç:** Saldırganların araca fiziksel temas kurmadan, sadece Bluetooth mesafesinden tam kontrol sağlamasına yol açmıştır.