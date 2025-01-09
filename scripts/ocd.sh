. ./parse-board-type.sh
board_type=$(parse_pico_board ../CMakeLists.txt)

~/.pico-sdk/openocd/0.12.0+dev/openocd \
  -f rp2040.cfg \
  -c "init"
