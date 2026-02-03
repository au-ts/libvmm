set -e
# usage: ./iso_to_gpt_img.sh destination.img source.iso

truncate -s 72G "$1"
sfdisk --no-reread --no-tell-kernel "$1" <<EOF
label: dos

start=2048,size=71G
EOF

# it is extremely important that bs=512 and seek=2048, as seek is in unit of `bs`.
dd if="$2" of="$1" bs=4096 conv=notrunc,sync seek=256 status=progress