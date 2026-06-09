CC=gcc
CFLAGS=-Wall -Wextra -Werror -O0 -fPIE -pie
TARGET=src/bgt210_binary
SRC=src/bgt210_kurban.c

.PHONY: all compile clean lint docker-deploy run-core help

help:
	@echo "======================================================================Multi"
	@echo "🛰️  OPERASYON FANTOM HAFIZA - KOMUTA OTOMASYON MERKEZİ"
	@echo "======================================================================Multi"
	@echo " make compile         : C kaynak kodunu SecOps standartlarında derler."
	@echo " make clean           : Derlenmiş binary ve cache dosyalarını temizler."
	@echo " make lint            : Ruff motoru ile SAST güvenlik taraması yapar."
	@echo " make docker-deploy   : Sistemi izole konteyner sandbox modunda kaldırır."
	@echo " make run-core        : Çekirdek yazılımı arka planda (Daemon) başlatır."
	@echo "======================================================================Multi"

compile: $(SRC)
	@echo "[*] C Süreç Altyapısı Güvenli Modda Derleniyor..."
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)
	@chmod +x scripts/healthcheck.sh
	@echo "[+] Derleme Başarılı: $(TARGET)"

clean:
	@echo "[*] Kalıntı ve Önbellek Alanları Kazınıyor..."
	rm -f $(TARGET)
	find . -type d -name "__pycache__" -exec rm -rf {} +
	@echo "[+] Temizlik Tamamlandı."

lint:
	@echo "[*] Ruff Güvenlik Denetçisi (SAST) Çalıştırılıyor..."
	ruff check . --fix || true

docker-deploy:
	@echo "[*] Sandbox Ortamı Orkestre Ediliyor..."
	docker-compose up --build -d
un-core: compile
	@echo "[*] Fantom Hafıza Motoru Arka Planda Ateşleniyor..."
	./$(TARGET) &
