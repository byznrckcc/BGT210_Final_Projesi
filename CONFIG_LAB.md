# 🔒 ConfigLab: Sunucu Yapılandırma ve Sertleştirme Raporu

Bu laboratuvar çalışması, üretim ortamındaki (production) sunucuların güvenliğini
artırmak, kaynak tükenmesi (DoS) saldırılarını engellemek, yığın/tampon taşmalarını
önlemek ve hassas verileri korumak amacıyla yapılan 18 modüllük katı yapılandırma
adımlarını içerir.

---

## 🛠️ Modül 01: Hafıza ve Kaynak Sınırları (Memory Limits)

Sunucunun RAM kaynaklarının tek bir PHP süreci tarafından tüketilerek sistemin
çökmesini (Denial of Service - DoS) engellemek amacıyla sınırlandırma yapılmıştır.

- **Dosya:** `/etc/php/8.4/fpm/php.ini`

```ini
memory_limit = 256M
```

---

## 📦 Modül 02 & 03: Dosya Yükleme ve POST Veri Boyut Sınırları

Sunucuya yüklenecek dosyaların ve HTTP POST isteklerinin boyutları sınırlandırılarak
ağ bant genişliği ve disk alanı koruma altına alınmıştır.

- **Dosya:** `/etc/php/8.4/fpm/php.ini`

```ini
upload_max_filesize = 32M
post_max_size = 64M
```

---

## ⚡ Modül 04: OPcache Yapılandırması ve Güvenliği

PHP kodlarının her istekte yeniden derlenmesini önleyerek byte-code önbelleğe
alınması ve bu bellek alanının manipülasyonlara karşı korunması sağlanmıştır.

- **Dosya:** `/etc/php/8.4/fpm/php.ini`

```ini
opcache.enable = 1
opcache.memory_consumption = 128
opcache.interned_strings_buffer = 8
opcache.max_accelerated_files = 10000
opcache.validate_timestamps = 0
```

---

## 🔐 Modül 05 & 06: Katı Oturum Yönetimi ve Cookie Sertleştirme

Kullanıcı oturumlarının (sessions) tarayıcı tarafında XSS ve CSRF gibi web
zafiyetleriyle çalınmasını (Session Hijacking) engellemek için çerez güvenlik
bayrakları zorunlu kılınmıştır.

- **Dosya:** `/etc/php/8.4/fpm/php.ini`

```ini
session.cookie_httponly = 1
session.cookie_secure = 1
session.cookie_samesite = "Strict"
session.use_strict_mode = 1
```

---

## 📁 Modül 07: open_basedir ile Dosya Sistemi İzolasyonu

PHP süreçlerinin sadece belirlenen güvenli dizinler altında çalışması zorunlu
kılınarak sistem kök dizinine erişimler (Path Traversal / Dosya Okuma) engellenmiştir.

- **Dosya:** `/etc/php/8.4/fpm/php.ini`

```ini
open_basedir = "/var/www/html/:/tmp/"
```

---

## 🚫 Modül 08 & 09: Hata Gizleme ve Tehlikeli Fonksiyonların Kapatılması

Saldırganların sistem mimarisini öğrenmesini (Information Disclosure) engellemek ve
kod enjeksiyonu durumunda işletim sisteminde komut çalıştırılmasını (RCE) önlemek
için kritik fonksiyonlar yasaklanmıştır.

- **Dosya:** `/etc/php/8.4/fpm/php.ini`

```ini
display_errors = Off
log_errors = On
error_log = /var/log/php_errors.log
disable_functions = exec,passthru,shell_exec,system,proc_open,popen,curl_exec,curl_multi_exec,parse_ini_file,show_source
```

---

## 🌐 Modül 10 & 11: Uzaktan Dosya Dahil Etme ve Sürüm Gizleme Koruması

Sunucunun dış dünyadan kontrolsüz kod çekmesini (RFI - Remote File Inclusion) ve
PHP sürüm bilgisini HTTP başlıklarında sızdırmasını engellemek için yapılan ayarlardır.

- **Dosya:** `/etc/php/8.4/fpm/php.ini`

```ini
allow_url_fopen = Off
allow_url_include = Off
expose_php = Off
```

---

## 🛡️ Modül 12: Dosya Sistemi İzinlerinin Sertleştirilmesi (Linux ACL)

Web sunucusunun kod dosyalarını sadece okuma yetkisinin olması, zararlı bir scriptin
kaynak kodları değiştirmesini engeller.

- **Komut Dosyası (Linux CLI):**

```bash
sudo chown -R root:www-data /var/www/html/
sudo find /var/www/html/ -type d -exec chmod 750 {} \;
sudo find /var/www/html/ -type f -exec chmod 640 {} \;
```

---

## 🚀 Modül 13 & 14: Nginx Bilgi Sızıntısı ve HTTP Güvenlik Başlıkları

Nginx katmanında versiyon gizleme ve tarayıcı tabanlı saldırıları (Clickjacking, XSS)
önlemek için HTTP header korumaları eklenmiştir.

- **Dosya:** `/etc/nginx/nginx.conf`

```nginx
server_tokens off;
add_header X-Frame-Options "DENY" always;
add_header X-Content-Type-Options "nosniff" always;
add_header X-XSS-Protection "1; mode=block" always;
add_header Content-Security-Policy "default-src 'self';" always;
```

---

## ⚡ Modül 15: Nginx Tampon Taşması (Buffer Overflow) Koruması

Büyük boyutlu sahte paketlerle sunucu bellek hatlarını şişirmeye yönelik DoS
girişimlerini sınırlandırma adımıdır.

- **Dosya:** `/etc/nginx/nginx.conf`

```nginx
client_body_buffer_size 10k;
client_header_buffer_size 1k;
client_max_body_size 8m;
large_client_header_buffers 2 1k;
```

---

## 🔑 Modül 16 & 17: SSH Çekirdek Sertleştirmesi ve UFW Güvenlik Duvarı

Sunucu yönetim portunun (SSH) brute-force saldırılarına karşı korunması ve sadece
belirli servis portlarının dış dünyaya açılması adımıdır.

- **Dosya:** `/etc/ssh/sshd_config`

```text
Port 2222
PermitRootLogin no
MaxAuthTries 3
```

- **Linux CLI:**

```bash
sudo ufw default deny incoming
sudo ufw default allow outgoing
sudo ufw allow 2222/tcp
sudo ufw allow 80/tcp
sudo ufw allow 443/tcp
sudo ufw enable
```

---

## 🐳 Modül 18: Docker Sandbox İzolasyonu ve Non-Root Konteyner Düzeni

Uygulamanın Docker konteyneri içinde root yetkileriyle çalıştırılması engellenmiş,
olası bir RCE durumunda saldırganın doğrudan host işletim sistemine sızması
(Container Breakout) engellenmiştir.

- **Dosya:** `Dockerfile`

```dockerfile
# Güvenli Multi-Stage ve Non-Root Kullanıcı Yapısı
FROM php:8.4-fpm-alpine
RUN addgroup -S securitygroup && adduser -S securityuser -G securitygroup
USER securityuser
EXPOSE 80
```