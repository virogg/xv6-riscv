#!/usr/bin/env bash
set -euo pipefail

IMG=ext2.img
MNT=./ext2mnt
SIZE=128M
BIGFILE=$MNT/bigfile.bin
SMALLFILE=$MNT/hello.txt

echo "[1] создаём образ $SIZE"
truncate --size "$SIZE" "$IMG"

echo "[2] форматируем ext2"
mkfs.ext2 -N 60000 "$IMG" >/dev/null

echo "[3] монтируем"
sudo mkdir -p "$MNT"
sudo mount -o loop -t ext2 "$IMG" "$MNT"

echo "[4] пишем файлы"
dd if=/dev/urandom of="$BIGFILE" bs=1M count=40 status=none  # > однократного косвенного
echo "Привет ext2" >"$SMALLFILE"

echo "[5] фиксируем иноды и контрольные суммы"
stat -c "%i %n" "$BIGFILE" "$SMALLFILE" | tee inodes.txt
sha512sum "$BIGFILE" "$SMALLFILE" | tee orig.sha512

echo "[6] размонтируем"
sudo umount "$MNT"

echo "[7] проверяем утилиту"
while read -r ino path; do
    ./getinode "$IMG" "$ino" | sha512sum
done < inodes.txt | tee extr.sha512

echo "[8] сравнение"
diff -q <(cut -d' ' -f1 orig.sha512) <(cut -d' ' -f1 extr.sha512) \
  && echo "OK: данные совпадают"