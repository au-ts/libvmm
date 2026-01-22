set -e
# usage: ./iso_to_gpt_img.sh destination.img source.iso

truncate -s 32G "$1"
sfdisk --no-reread --no-tell-kernel "$1" <<EOF
label: dos

start=2048,size=10G
EOF
dd if="$2" of="$1" conv=notrunc seek=2048