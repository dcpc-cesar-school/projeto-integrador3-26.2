#!/usr/bin/env bash
# monitor.sh - mede CPU, memoria e threads de um processo e salva em CSV
# Uso: ./monitor.sh <PID|nome> [intervalo_s] [amostras] [limite_cpu]
export LC_ALL=C                      # ponto decimal (evita "12,5" no CSV)
ALVO=$1; INTERVALO=${2:-1}; AMOSTRAS=${3:-30}; LIMITE=${4:-80}
[ -z "$ALVO" ] && { echo "Uso: $0 <PID|nome> [intervalo] [amostras] [limite]"; exit 1; }
# 1) Descobrir o PID
if [[ "$ALVO" =~ ^[0-9]+$ ]]; then PID=$ALVO; else PID=$(pgrep -o "$ALVO"); fi
[ -z "$PID" ] && { echo "Processo '$ALVO' nao encontrado"; exit 1; }
# 2) Preparar o arquivo de saida
NOME=$(ps -p "$PID" -o comm=)
ARQ="medicoes_${NOME}_$(date +%Y%m%d_%H%M%S).csv"
echo "timestamp,pid,cpu,mem,rss_kb,threads,estado" > "$ARQ"
echo "Monitorando $NOME (PID $PID) -> $ARQ"
# 3) Laco de medicoes
for ((i=1; i<=AMOSTRAS; i++)); do
  kill -0 "$PID" 2>/dev/null || { echo "Processo terminou"; break; }
  read CPU MEM <<< "$(top -b -n 1 -p "$PID" | tail -1 | awk '{print $9, $10}')"
  read RSS THR EST <<< "$(ps -p "$PID" -o rss=,nlwp=,stat=)"
  echo "$(date +%T),$PID,$CPU,$MEM,$RSS,$THR,$EST" >> "$ARQ"
  # 4) Gerenciar: alerta se passar do limite
  if awk -v c="$CPU" -v l="$LIMITE" 'BEGIN{exit !(c>l)}'; then
    echo "[ALERTA] $(date +%T) CPU=$CPU% > $LIMITE%" | tee -a alertas.log
  fi
  sleep "$INTERVALO"
done
# 5) Resumo estatistico
echo "--- Resumo ---"
awk -F, 'NR>1 {c+=$3; m+=$5; n++; if($3>mx) mx=$3; if($6>t) t=$6}
  END {printf "Amostras: %d\n", n
       printf "CPU media: %.1f%%  max: %.1f%%\n", c/n, mx
       printf "RSS medio: %.0f KB\nMax threads: %d\n", m/n, t}' "$ARQ"
