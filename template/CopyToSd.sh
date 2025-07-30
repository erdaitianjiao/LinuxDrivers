#!/bin/sh

# 目标目录
TARGET_DIR="/media/tianjiao/d90c8399-2f8b-4e97-8e3f-c2034a79d329/lib/modules/4.1.15"

# 复制连个文件
if [ $# -ne 2 ]; then
    echo "用法: $0 <源文件名> <目标文件名>"
    exit 1
fi

# 复制
sudo cp "$1"  "$2" "${TARGET_DIR}" 


# 对比两个文件的区别
echo "本地目录\n"

ls $1 $2 -al

echo "复制目录\n"

ls "${TARGET_DIR}/$1" "${TARGET_DIR}/$2" -al
