#!/bin/bash
set -euo pipefail

# Проверка наличия бинарника
if [ ! -x "./fifo_echo_server" ]; then
    echo "Ошибка: ./fifo_echo_server не найден. Соберите проект." >&2
    exit 1
fi

# Временные файлы
FIFO="/tmp/test_fifo"
LOG="/tmp/server.log"
FG_FIFO="/tmp/fg_fifo"
FG_LOG="/tmp/fg.log"
BAD_FIFO="/tmp/bad_fifo"

# Функция очистки: удаляем временные файлы и завершаем фоновые процессы
cleanup() {
    rm -f "$FIFO" "$LOG" "$FG_FIFO" "$FG_LOG" "$BAD_FIFO"
    kill -TERM $(jobs -p) 2>/dev/null || true
}
trap cleanup EXIT

# Тест 1: Демон-режим и базовый эхо-функционал
echo "Тест 1: Демон-режим и эхо"
./fifo_echo_server -d -f "$FIFO" -l "$LOG" &
sleep 2
echo "Test message" > "$FIFO"
sleep 1
grep -q "Received 13 bytes: Test message" "$LOG" || { echo "FAIL: Тест 1"; exit 1; }
echo "PASS: Тест 1"
cleanup

# Тест 2: SIGUSR1 (статистика)
echo "Тест 2: SIGUSR1"
./fifo_echo_server -d -f "$FIFO" -l "$LOG" &
sleep 2
kill -USR1 "$(pgrep fifo_echo_server)"
sleep 1
echo "" > "$FIFO"  # Отправляем пустую строку, чтобы разблокировать сервер
sleep 1
grep -q "Statistics:" "$LOG" || { echo "FAIL: Тест 2"; exit 1; }
echo "PASS: Тест 2"
cleanup

# Тест 3: SIGTERM (немедленное завершение)
echo "Тест 3: SIGTERM"
./fifo_echo_server -d -f "$FIFO" -l "$LOG" &
sleep 2
kill -TERM "$(pgrep fifo_echo_server)"
sleep 2
(grep -q "Server shutdown" "$LOG" && grep -q "Received" "$LOG") || { echo "FAIL: Тест 3"; exit 1; }
[ ! -e "$FIFO" ] || { echo "FAIL: FIFO не удален"; exit 1; }
echo "PASS: Тест 3"
cleanup

# Тест 4: Foreground с SIGINT
echo "Тест 4: Foreground + SIGINT"
./fifo_echo_server -f "$FG_FIFO" -l "$FG_LOG" &
FG_PID=$!
sleep 2
echo "Foreground test before" > "$FG_FIFO"
sleep 1
kill -INT "$FG_PID"
echo "Foreground test after" > "$FG_FIFO"
sleep 2
(grep -q "FIFO closed" "$FG_LOG" && grep -q "Server shutdown" "$FG_LOG") || { echo "FAIL: Тест 4"; exit 1; }
echo "PASS: Тест 4"
cleanup

# Тест 5: Игнорирование SIGQUIT
echo "Тест 5: SIGQUIT"
./fifo_echo_server -f "$FIFO" &
sleep 1
kill -QUIT "$(pgrep fifo_echo_server)"
sleep 1
kill -TERM "$(pgrep fifo_echo_server)"
wait "$(pgrep fifo_echo_server)" || true
echo "PASS: Тест 5"
cleanup

# Тест 6: Демонизация через SIGHUP
echo "Тест 6: SIGHUP демонизация"
./fifo_echo_server -f "$FIFO" -l "$LOG" &
sleep 1
kill -HUP "$(pgrep fifo_echo_server)"
sleep 2
echo "" > "$FIFO"
sleep 1
if [ -f "$LOG" ] && grep -q "Daemon started" "$LOG"; then
    echo "PASS: Тест 6"
else
    echo "FAIL: Тест 6"
    exit 1
fi
kill -TERM "$(pgrep fifo_echo_server)"
cleanup

# Тест 7: Будильник (ALARM_INTERVAL)
echo "Тест 7: Будильник"
./fifo_echo_server -f "$FIFO" -l "$LOG" &
sleep 6  # Ждем немного больше 5 секунд
grep -q "Server active" "$LOG" || { echo "FAIL: Тест 7"; exit 1; }
kill -TERM "$(pgrep fifo_echo_server)"
echo "PASS: Тест 7"
cleanup

echo "Все тесты пройдены успешно!"