set -e
# usage: ./iso_to_gpt_img.sh destination.img source.iso

truncate -s 72G "$1"
sfdisk --no-reread --no-tell-kernel "$1" <<EOF
label: dos

start=2048,size=71G
EOF
dd if="$2" of="$1" bs=16M conv=notrunc,sync seek=2048 status=progress