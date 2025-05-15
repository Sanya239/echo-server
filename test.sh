#!/bin/bash

truncate --size 1M ext2.img
mkfs.ext2 ext2.img
mkdir ext2
sudo mount -t ext2 ext2.img ext2
cd ext2

# Создание вложенной структуры папок
mkdir -p project/section1/subsectionA
mkdir -p project/section1/subsectionB
mkdir -p project/section2/subsectionC

# Создание файлов и запись текста
echo "Это файл A1" > project/section1/subsectionA/fileA1.txt
truncate -s 10K project/section1/subsectionA/fileA1.txt
echo "Это файл A2" > project/section1/subsectionA/fileA2.txt
echo "Это файл B1, в котором написан текст побольше, чем в файле А2 или А1" > project/section1/subsectionB/fileB1.txt
echo "C1" > project/section2/subsectionC/fileC1.txt
echo "Это файл C2, в котором написан ну просто огромный и содержательный текст. А потом этот файл ещё и ресайзнут до невероятных масштабов. Короче говоря файл-гигант!" > project/section2/subsectionC/fileC2.txt
truncate -s 100K project/section2/subsectionC/fileC1.txt

cd ..


shopt -s globstar  # Включаем поддержку **
for file in ext2/project/**/**; do
  if [ -f "$file" ]; then
    inode=$(stat -c '%i' "$file")
    md5=$(md5sum "$file" | awk '{print $1}')
    echo "Файл: $file | inode: $inode | md5: $md5"  >> stats.txt
  fi
done

