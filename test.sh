#!/bin/bash

truncate --size 1M ext2.img
mkfs.ext2 ext2.img
mkdir ext2
sudo mount -t ext2 ext2.img ext2
cd ext2

# Создание вложенной структуры папок
sudo mkdir -p project/section1/subsectionA
sudo mkdir -p project/section1/subsectionB
sudo mkdir -p project/section2/subsectionC

# Создание файлов и запись текста
sudo echo "Это файл A1" > project/section1/subsectionA/fileA1.txt
sudo truncate -s 10K project/section1/subsectionA/fileA1.txt
sudo echo "Это файл A2" > project/section1/subsectionA/fileA2.txt
sudo echo "Это файл B1, в котором написан текст побольше, чем в файле А2 или А1" > project/section1/subsectionB/fileB1.txt
sudo echo "C1" > project/section2/subsectionC/fileC1.txt
sudo echo "Это файл C2, в котором написан ну просто огромный и содержательный текст(нет). Короче говоря файл-гигант!" > project/section2/subsectionC/fileC2.txt
sudo head -c 40K /dev/urandom >> project/section2/subsectionC/fileC2.txt
sudo truncate -s 1000K project/section2/subsectionC/fileC2.txt

cd ..


shopt -s globstar  # Включаем поддержку **

inode_numbers=()
test_array=()
echo "${test_array[@]}"
echo "Реальные данные" > stats.txt
for file in ext2/project/**/**; do
  if [ -f "$file" ]; then
    inode=$(stat -c '%i' "$file")
    inode_numbers+=("$inode")
    sha512=$(sha512sum "$file" | awk '{print $1}')
    echo "Файл: $file | inode: $inode | sha512: $sha512"  >> stats.txt
  fi
done
umount ext2
echo "Файлы сгенерированы"

gcc -o main main.c
echo "Вывод утилиты" >> stats.txt
for inode in ${inode_numbers[@]}; do
      echo "$inode"
      sha512=$(./main "ext2.img" "$inode" | sha512sum  | awk '{print $1}')
      echo "Файл2: $file | inode: $inode | sha512: $sha512"  >> stats.txt
done

