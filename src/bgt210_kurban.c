/*
 * ============================================================
 * BGT210 - Tersine Mühendislik | Final Projesi
 * Proje Adı : Operasyon: Fantom Hafıza
 * Dosya     : bgt210_kurban.c
 * Amaç      : Linux'ta güvenli bellek yönetimi ve
 *             olay-tetiklemeli veri yaşam döngüsü demosu
 *
 * Mimari Özeti:
 *   1. Hassas veri hiçbir zaman düz metin (plaintext) olarak
 *      bellekte kalıcı şekilde tutulmaz.
 *   2. Maskeleme: basit XOR şifrelemesi (anahtar: 0x5A).
 *      Gerçek sistemlerde AES-GCM gibi kriptografik yöntemler
 *      tercih edilmelidir; burada pedagojik amaçla XOR kullanılmıştır.
 *   3. Sinyal işleme (SIGUSR1): veri yalnızca kısa bir pencerede
 *      (5 saniye) erişilebilir hale getirilir; süre dolunca
 *      güvenli temizleme yapılır.
 *   4. Bellek temizlemede volatile kullanımı: derleyicinin
 *      "kullanılmıyor" varsayımıyla optimizasyon yapıp
 *      memset çağrısını atlamasını engeller.
 *
 * Güvenlik Notu:
 *   Bu kod YALNIZCA eğitim amaçlıdır. Üretim ortamında
 *   mlock(), sodium_memzero() veya explicit_bzero()
 *   kullanılması önerilir.
 * ============================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

/* ── Sabitler ─────────────────────────────────────────────── */

/*
 * XOR_KEY: Maskeleme anahtarı.
 * Gerçek uygulamalarda bu değer rastgele üretilmeli ve
 * güvenli bir anahtar deposunda (HSM, TPM) saklanmalıdır.
 */
#define XOR_KEY         0x5A

/*
 * EXPOSE_WINDOW_SEC: Verinin açık (unmask) kalacağı süre.
 * Bu süre sonunda tampon güvenli biçimde sıfırlanır.
 */
#define EXPOSE_WINDOW_SEC 5

/* Tampon boyutu: veri + null-terminator */
#define BUF_SIZE        64


/* ── Veri Yaşam Döngüsü Yapısı ────────────────────────────── */

/*
 * SecureBuffer: hassas verinin yaşam döngüsünü yöneten yapı.
 *
 * Alanlar:
 *   masked_data  – XOR ile maskelenmiş baytlar
 *   data_len     – geçerli bayt sayısı (null hariç)
 *   is_exposed   – veri şu an açık mı? (1 = evet)
 *   expose_time  – verinin kaç saniyedir açık olduğu
 */
typedef struct {
    unsigned char masked_data[BUF_SIZE];
    size_t        data_len;
    volatile int  is_exposed;   /* volatile: derleyici optimizasyonunu engelle */
    time_t        expose_time;
} SecureBuffer;

/* Global tampon – sinyal handler'ından erişilebilmesi için global */
static SecureBuffer g_secure_buf;


/* ── Yardımcı Fonksiyonlar ────────────────────────────────── */

/*
 * secure_zero(): Bellek bölgesini güvenli biçimde sıfırlar.
 *
 * Neden sıradan memset() yetmez?
 *   Derleyici, "bu bellek daha sonra okunmuyor" diye
 *   memset() çağrısını kaldırabilir (dead-store elimination).
 *   volatile dönüştürme ile bu optimizasyon engellenir.
 *
 *   POSIX sistemlerde explicit_bzero() veya
 *   libsodium'daki sodium_memzero() daha güvenilirdir.
 */
static void secure_zero(void *ptr, size_t len)
{
    volatile unsigned char *p = (volatile unsigned char *)ptr;
    while (len--) {
        *p++ = 0;
    }
}

/*
 * xor_mask(): Veriyi XOR_KEY ile maskeler VEYA maskesini açar.
 *
 * XOR'un güzel özelliği: aynı işlem hem şifreleme hem de
 * deşifreleme için kullanılır (f(f(x)) = x).
 *
 * Parametreler:
 *   src    – giriş tamponu
 *   dst    – çıkış tamponu (src ile aynı olabilir)
 *   length – işlenecek bayt sayısı
 *   key    – XOR anahtarı
 */
static void xor_mask(const unsigned char *src,
                     unsigned char       *dst,
                     size_t               length,
                     unsigned char        key)
{
    for (size_t i = 0; i < length; i++) {
        dst[i] = src[i] ^ key;
    }
}

/*
 * init_secure_buffer(): Hassas veriyi maskeleyerek tampona yükler.
 *
 * Süreç: düz metin → XOR → masked_data
 * Düz metin bu fonksiyon bittikten sonra stack'ten silinir
 * (secure_zero ile temizlenir).
 */
static void init_secure_buffer(SecureBuffer *buf,
                                const char   *plaintext)
{
    size_t len = strlen(plaintext);
    if (len >= BUF_SIZE) {
        len = BUF_SIZE - 1;  /* taşma koruması */
    }

    /* 1. Maskeleme: düz metni XOR ile şifrele */
    xor_mask((const unsigned char *)plaintext,
             buf->masked_data,
             len,
             XOR_KEY);

    buf->data_len   = len;
    buf->is_exposed = 0;
    buf->expose_time = 0;

    printf("[INIT] Hassas veri bellege yuklendi (maskeli).\n");
    printf("[INIT] Veri uzunlugu: %zu bayt\n", len);
    printf("[INIT] Maskelenmis ilk 4 bayt: %02X %02X %02X %02X ...\n",
           buf->masked_data[0], buf->masked_data[1],
           buf->masked_data[2], buf->masked_data[3]);
}


/* ── Sinyal İşleyici ──────────────────────────────────────── */

/*
 * sigusr1_handler(): SIGUSR1 sinyali geldiğinde çalışır.
 *
 * Sinyal handler'larında dikkat edilmesi gerekenler:
 *   - Yalnızca async-signal-safe fonksiyonlar çağrılabilir.
 *     (malloc, printf gibi fonksiyonlar güvensizdir;
 *      burada write() ve sleep() kullanılmıştır.)
 *   - Global değişkenler 'volatile' olarak tanımlanmalıdır.
 *
 * Veri Yaşam Döngüsü bu handler içinde yönetilir:
 *   1. Maskeyi aç (XOR geri al)
 *   2. Veriyi kullan / göster
 *   3. EXPOSE_WINDOW_SEC saniye bekle
 *   4. Güvenli sıfırlama (secure_zero)
 *   5. Veriyi yeniden maskele
 */
static void sigusr1_handler(int signum)
{
    (void)signum;  /* kullanılmayan parametre uyarısını bastır */

    /* async-signal-safe: write() kullanıyoruz */
    const char *msg1 = "\n[HANDLER] SIGUSR1 alindi. Veri maskesi kaldiriliyor...\n";
    write(STDOUT_FILENO, msg1, strlen(msg1));

    /* Geçici tampon: stack üzerinde, fonksiyon bitince otomatik silinir */
    unsigned char temp_plain[BUF_SIZE];

    /* ── Aşama 1: Maskeyi Kaldır ── */
    xor_mask(g_secure_buf.masked_data,
             temp_plain,
             g_secure_buf.data_len,
             XOR_KEY);
    temp_plain[g_secure_buf.data_len] = '\0';  /* null-terminate */

    g_secure_buf.is_exposed  = 1;
    g_secure_buf.expose_time = time(NULL);

    /* Veriyi göster (gerçek uygulamada bu adım güvenli kanal üzerinden olur) */
    const char *msg2 = "[HANDLER] Cozulmus veri (gizli pencere acik):\n";
    write(STDOUT_FILENO, msg2, strlen(msg2));
    write(STDOUT_FILENO, "  >> ", 5);
    write(STDOUT_FILENO, temp_plain, g_secure_buf.data_len);
    write(STDOUT_FILENO, "\n", 1);

    /* ── Aşama 2: Açık Pencere – EXPOSE_WINDOW_SEC saniye ── */
    const char *msg3 = "[HANDLER] Pencere acik. 5 saniye sonra temizlenecek...\n";
    write(STDOUT_FILENO, msg3, strlen(msg3));
    sleep(EXPOSE_WINDOW_SEC);

    /* ── Aşama 3: Güvenli Sıfırlama ── */
    /*
     * Önce düz metin tamponunu (temp_plain) temizle.
     * Bu, stack'te kalan veri izlerini siler.
     */
    secure_zero(temp_plain, sizeof(temp_plain));

    const char *msg4 = "[HANDLER] Guvenli sifirlama yapiliyor (secure_zero)...\n";
    write(STDOUT_FILENO, msg4, strlen(msg4));

    /* ── Aşama 4: Yeniden Maskeleme ── */
    /*
     * masked_data zaten XOR'lu hali tutuyor, değişmedi.
     * is_exposed bayrağını sıfırlıyoruz.
     * Gerçek bir sistemde bu noktada yeni bir anahtar
     * üretilip veri yeniden şifrelenebilir (key rotation).
     */
    g_secure_buf.is_exposed  = 0;
    g_secure_buf.expose_time = 0;

    const char *msg5 = "[HANDLER] Veri yeniden maskeli duruma dondu.\n";
    write(STDOUT_FILENO, msg5, strlen(msg5));

    const char *msg6 = "[HANDLER] Veri yasam dongusu tamamlandi.\n\n";
    write(STDOUT_FILENO, msg6, strlen(msg6));
}


/* ── Ana Program ──────────────────────────────────────────── */

int main(void)
{
    printf("============================================\n");
    printf(" BGT210 | Operasyon: Fantom Hafiza\n");
    printf(" Guvenli Bellek Yonetimi Demosu\n");
    printf("============================================\n\n");

    /* PID bilgisi: monitor betiğinin sinyal göndermesi için */
    printf("[MAIN] Proses PID: %d\n", getpid());
    printf("[MAIN] SIGUSR1 bekleniyor...\n\n");

    /* ── Hassas Veriyi Yükle ── */
    /*
     * "BGT210_SECRET_FLAG_2026" dizisi sadece burada açık metin olarak
     * geçiyor; init_secure_buffer() içine girildiğinde hemen XOR'lanıp
     * masked_data'ya yazılır.
     *
     * NOT: Derleme sonrası binary içinde bu string literal hâlâ
     * .rodata segmentinde görünebilir. Gerçek gizlilik için veri
     * runtime'da ayrı bir kanaldan alınmalı veya obfuscation
     * teknikleri uygulanmalıdır (bu da tersine mühendisliğin
     * incelediği bir konudur).
     */
    init_secure_buffer(&g_secure_buf, "BGT210_SECRET_FLAG_2026");

    /* ── Sinyal Handler'ını Kaydet ── */
    /*
     * sigaction() tercih edilir; signal() taşınabilirlik
     * sorunları yaşayabilir.
     * SA_RESTART: sinyal sonrasında kesilen sistem çağrılarını
     * otomatik yeniden başlatır.
     */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigusr1_handler;
    sa.sa_flags   = SA_RESTART;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("[HATA] sigaction");
        return EXIT_FAILURE;
    }

    printf("[MAIN] Sinyal handler kayitli (SIGUSR1 = %d)\n", SIGUSR1);
    printf("[MAIN] Sinyal gonderme: kill -USR1 %d\n\n", getpid());

    /* ── Ana Döngü ── */
    /*
     * Gerçek bir uygulamada burada iş mantığı yürütülür.
     * Demoda sadece durum bilgisi yazdırıyoruz.
     *
     * pause() kullanılmadı; yerine periyodik durum kontrolü
     * yapıldı — bu, is_exposed bayrağını izlememizi sağlar.
     */
    int iteration = 0;
    while (1) {
        sleep(2);
        iteration++;

        if (g_secure_buf.is_exposed) {
            printf("[MAIN] [%d] Durum: VERİ ACIK (expose_time=%ld)\n",
                   iteration, (long)g_secure_buf.expose_time);
        } else {
            printf("[MAIN] [%d] Durum: Veri maskeli / guvenli\n", iteration);
        }

        /* Programı sonlandırmak için: Ctrl+C veya SIGTERM */
        if (iteration >= 60) {
            printf("[MAIN] Demo suresi doldu. Cikiliyor...\n");
            break;
        }
    }

    /* ── Çıkış Temizliği ── */
    /*
     * Program sonlanmadan önce tüm hassas tamponları temizle.
     * Bu, swap/core dump sızıntılarını kısmen azaltır.
     * Tam koruma için mlock() + munlock() + madvise(MADV_DONTDUMP)
     * kombinasyonu önerilir.
     */
    secure_zero(&g_secure_buf, sizeof(g_secure_buf));
    printf("[MAIN] Bellek temizligi tamamlandi. Program sonlaniyor.\n");

    return EXIT_SUCCESS;
}

/*
 * ============================================================
 * DERLEME & ÇALIŞTIRMA
 *
 *   gcc -Wall -Wextra -O0 -o bgt210_kurban bgt210_kurban.c
 *   ./bgt210_kurban &
 *   kill -USR1 <PID>
 *
 * -O0: Optimizasyonu kapat (secure_zero'nun atlanmaması için).
 *      Gerçek uygulamalarda explicit_bzero() kullanın.
 *
 * GÜVENLIK SINIRLILIKLARI:
 *   1. XOR, kriptografik olarak güçlü DEĞİLDİR.
 *      Pedagojik amaçla seçilmiştir.
 *   2. .rodata'daki string literal sorununu çözmez.
 *   3. swap alanına veri yazılmasını engellemez;
 *      bunun için mlock() gereklidir.
 *   4. Core dump koruması için /proc/sys/kernel/core_pattern
 *      ve prctl(PR_SET_DUMPABLE, 0) incelenebilir.
 * ============================================================
 */
