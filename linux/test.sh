#!/bin/bash

set -euo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

IMG="sparse_test.img"
MNT="sparse_mnt"
SIZE="1G"

cleanup() {
  echo -e "${YELLOW}Очистка временных файлов...${NC}"
  sudo umount "${MNT}" 2>/dev/null || :
  rm -f "${IMG}" extracted_* sha512sums.txt
  rmdir "${MNT}" 2>/dev/null || :
}
trap cleanup EXIT

info() {
  echo -e "${BLUE}[INFO]${NC} $1"
}

success() {
  echo -e "${GREEN}[OK]${NC} $1"
}

error() {
  echo -e "${RED}[ERROR]${NC} $1"
}

setup_filesystem() {
  info "Создание образа файловой системы размером ${SIZE}"
  truncate --size "${SIZE}" "${IMG}"

  info "Форматирование файловой системы ext2"
  mkfs.ext2 -F "${IMG}" > /dev/null

  info "Создание точки монтирования"
  mkdir -p "${MNT}"

  info "Монтирование файловой системы"
  sudo mount -o loop "${IMG}" "${MNT}"
  sudo chown "$(id -u):$(id -g)" "${MNT}"
}

create_test_files() {
  info "Создание тестовых разреженных файлов..."

  # Тест 1: Обычный файл с данными (контрольный)
  info "1. Создание обычного файла (не разреженный)"
  dd if=/dev/urandom of="${MNT}/regular.bin" bs=4K count=2 status=none

  # Тест 2: Файл с небольшой дырой между двумя блоками данных
  info "2. Создание файла с дырой между блоками данных"
  dd if=/dev/urandom of="${MNT}/sparse_between.bin" bs=4K count=1 status=none
  dd if=/dev/urandom of="${MNT}/sparse_between.bin" bs=4K count=1 seek=5 status=none conv=notrunc

  # Тест 3: Файл с несколькими дырами
  info "3. Создание файла с несколькими дырами"
  dd if=/dev/urandom of="${MNT}/multi_holes.bin" bs=4K count=1 status=none
  dd if=/dev/urandom of="${MNT}/multi_holes.bin" bs=4K count=1 seek=3 status=none conv=notrunc
  dd if=/dev/urandom of="${MNT}/multi_holes.bin" bs=4K count=1 seek=7 status=none conv=notrunc
  dd if=/dev/urandom of="${MNT}/multi_holes.bin" bs=4K count=1 seek=10 status=none conv=notrunc

  # Тест 4: Файл с дырой в начале
  info "4. Создание файла с дырой в начале"
  dd if=/dev/zero of="${MNT}/hole_start.bin" bs=4K count=3 seek=4 status=none

  # Тест 5: Файл с дырой в конце
  info "5. Создание файла с дырой в конце"
  dd if=/dev/urandom of="${MNT}/hole_end.bin" bs=4K count=3 status=none
  truncate -s 28K "${MNT}/hole_end.bin"  # Увеличиваем размер, создавая дыру

  # Тест 6: Файл, состоящий только из нулей (все блоки - дыры)
  info "6. Создание файла только из дыр"
  truncate -s 32K "${MNT}/all_holes.bin"

  # Тест 7: Большой разреженный файл с дырой, превышающей прямую адресацию
  info "7. Создание файла с дырой, превышающей прямую адресацию"
  dd if=/dev/urandom of="${MNT}/large_hole.bin" bs=4K count=2 status=none
  dd if=/dev/urandom of="${MNT}/large_hole.bin" bs=4K count=1 seek=20 status=none conv=notrunc

  # Тест 8: Очень большой разреженный файл с дырой, требующей двойной непрямой адресации
  info "8. Создание файла с дырой, требующей двойной непрямой адресации"
  dd if=/dev/urandom of="${MNT}/huge_hole.bin" bs=4K count=1 status=none
  # Создаем блок после большого смещения (требует двойной непрямой адресации)
  dd if=/dev/urandom of="${MNT}/huge_hole.bin" bs=4K count=1 seek=50000 status=none conv=notrunc

  # Тест 9: Файл с комбинацией больших и маленьких дыр
  info "9. Создание файла с комбинацией больших и маленьких дыр"
  dd if=/dev/urandom of="${MNT}/mixed_holes.bin" bs=4K count=1 status=none
  dd if=/dev/urandom of="${MNT}/mixed_holes.bin" bs=4K count=1 seek=3 status=none conv=notrunc
  dd if=/dev/urandom of="${MNT}/mixed_holes.bin" bs=4K count=1 seek=1000 status=none conv=notrunc
  dd if=/dev/urandom of="${MNT}/mixed_holes.bin" bs=4K count=1 seek=1005 status=none conv=notrunc

  info "Использование дискового пространства (реальные блоки):"
  # shellcheck disable=SC2012
  ls -lhs "${MNT}"/*.bin | awk '{printf "%-20s %8s %8s\n", $10, $1, $5}'
}

gather_info() {
  info "Сбор информации о файлах и вычисление контрольных сумм"
  echo -e "FILENAME\tINODE\tSHA512" > sha512sums.txt

  for file in "${MNT}"/*.bin; do
    filename=$(basename "$file")
    # shellcheck disable=SC2012
    inode=$(ls -i "$file" | awk '{print $1}')
    checksum=$(sha512sum "$file" | awk '{print $1}')
    echo -e "${filename}\t${inode}\t${checksum}" >> sha512sums.txt
    info "Файл: $filename, inode: $inode"
  done
}

test_extraction() {
  info "Размонтирование файловой системы"
  sudo umount "${MNT}"

  info "Тестирование извлечения данных"

  success_count=0
  total_count=0

  while IFS=$'\t' read -r filename inode original_checksum; do
    if [ "$filename" = "FILENAME" ]; then
      continue
    fi

    total_count=$((total_count + 1))

    info "Извлечение данных из файла: $filename (inode: $inode)"
    ./getinode "${IMG}" "${inode}" > "extracted_${filename}"

    extracted_checksum=$(sha512sum "extracted_${filename}" | awk '{print $1}')

    if [ "$original_checksum" = "$extracted_checksum" ]; then
      success "Контрольная сумма совпадает для $filename"
      success_count=$((success_count + 1))
    else
      error "Контрольная сумма НЕ совпадает для $filename"
      error "  Оригинал:  $original_checksum"
      error "  Извлечено: $extracted_checksum"

      orig_size=$(stat -c "%s" "${MNT}/${filename}" 2>/dev/null || echo "N/A")
      extracted_size=$(stat -c "%s" "extracted_${filename}")
      info "Размер оригинала: $orig_size байт, размер извлеченного: $extracted_size байт"

      if [ -f "${MNT}/${filename}" ]; then
        info "Первые 100 байт оригинала (в шестнадцатеричном формате):"
        hexdump -C -n 100 "${MNT}/${filename}"
      fi

      info "Первые 100 байт извлеченного файла (в шестнадцатеричном формате):"
      hexdump -C -n 100 "extracted_${filename}"
    fi
  done < sha512sums.txt

  echo
  info "Итоги тестирования:"
  info "Всего тестов: $total_count"
  info "Успешно: $success_count"

  if [ "$success_count" -eq "$total_count" ]; then
    success "ВСЕ ТЕСТЫ ПРОЙДЕНЫ УСПЕШНО!"
  else
    error "ОБНАРУЖЕНЫ ОШИБКИ: $((total_count - success_count)) из $total_count тестов не пройдены"
    return 1
  fi
}

echo -e "${YELLOW}=== ТЕСТИРОВАНИЕ ОБРАБОТКИ РАЗРЕЖЕННЫХ ФАЙЛОВ ===${NC}"

setup_filesystem

create_test_files

gather_info

test_extraction

echo -e "${YELLOW}=== ТЕСТИРОВАНИЕ ЗАВЕРШЕНО ===${NC}"