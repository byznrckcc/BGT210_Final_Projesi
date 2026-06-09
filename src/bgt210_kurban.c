#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ptrace.h>

// "BGT210_SECRET_FLAG_2026" ifadesinin XOR (0x5A) ile maskelenmiş yeni byte dizisi
unsigned char obfuscated_flag[] = {
    0x18, 0x1d, 0x0e, 0x68, 0x6b, 0x6a, 0x05, 0x09, 
    0x1f, 0x19, 0x08, 0x1f, 0x0e, 0x05, 0x1c, 0x16, 
    0x1b, 0x1d, 0x05, 0x68, 0x6a, 0x68, 0x6c
};
int flag_len = 23;

int main() {
    // BGT210 Koruma Katmanı: Anti-Debugging (ptrace izleme engeli)
    if (ptrace(PTRACE_TRACEME, 0, 1, 0) < 0) {
        printf("[-] HATA: Debugger algılandı! Güvenlik protokolü gereği hafıza kilitleniyor.\n");
        exit(1);
    }

    printf("[+] BGT210 TERSİNE MÜHENDİSLİK FİNAL PROJESİ\n");
    printf("[+] Süreç Adı: bgt210_binary | PID: %d\n", getpid());
    printf("[*] Protokol: Gizli veri RAM üzerinde dinamik olarak çözülüyor...\n");

    // RAM üzerinde yer tahsis et
    char *secret_buffer = malloc(flag_len + 1);
    memcpy(secret_buffer, obfuscated_flag, flag_len);
    
    // Runtime De-obfuscation (Bellek içinde anlık şifre çözme)
    for(int i = 0; i < flag_len; i++) {
        secret_buffer[i] ^= 0x5A;
    }
    secret_buffer[flag_len] = '\0';

    printf("[*] Durum: Korunmuş veri RAM'e yazıldı. Statik analiz araçları şu an kör.\n");
    printf("[*] Uyarı: Canlı hafıza triage analizi için sistem 'sleep' modunda bekletiliyor...\n");

    // Bellekte kalması için sonsuz döngü
    while(1) {
        sleep(5);
    }

    free(secret_buffer);
    return 0;
}
