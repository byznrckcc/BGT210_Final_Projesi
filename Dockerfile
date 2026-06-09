# ============================================================
# STAGE 1: Gelişmiş Derleme ve Bağımlılık Laboratuvarı
# ============================================================
FROM ubuntu:22.04 AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
    gcc \
    libc6-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /stage
COPY src/bgt210_kurban.c ./src/

RUN gcc -Wall -Wextra -Werror -O0 -fPIE -pie -o src/bgt210_binary src/bgt210_kurban.c

# ============================================================
# STAGE 2: Güvenli Runtime & Saldırı Yüzeyi Azaltılmış Ortam
# ============================================================
FROM python:3.10-slim AS runner

ENV PYTHONDONTWRITEBYTECODE=1 \
    PYTHONUNBUFFERED=1 \
    FLASK_ENV=production

WORKDIR /app

RUN apt-get update && apt-get install -y --no-install-recommends \
    procps \
    && rm -rf /var/lib/apt/lists/*

RUN groupadd -g 10002 phantomgroup && \
    useradd -u 10002 -g phantomgroup -m -s /bin/bash phantomuser

COPY --from=builder --chown=phantomuser:phantomgroup /stage/src/bgt210_binary ./src/bgt210_binary
COPY --chown=phantomuser:phantomgroup src/bgt210_monitor.py ./src/bgt210_monitor.py
COPY --chown=phantomuser:phantomgroup src/bgt210_spy.py ./src/bgt210_spy.py
COPY --chown=phantomuser:phantomgroup scripts/healthcheck.sh ./scripts/healthcheck.sh
COPY --chown=phantomuser:phantomgroup app.py .
COPY --chown=phantomuser:phantomgroup requirements.txt .

RUN mkdir -p logs && chown -R phantomuser:phantomgroup /app && \
    chmod +x ./scripts/healthcheck.sh

USER phantomuser
EXPOSE 5000

HEALTHCHECK --interval=10s --timeout=3s --start-period=5s --retries=2 \
    CMD ["/bin/bash", "./scripts/healthcheck.sh"]

CMD ["python3", "app.py"]
