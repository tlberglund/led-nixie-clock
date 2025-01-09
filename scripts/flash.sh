. ./parse-board-type.sh
board_type=$(parse_pico_board ../CMakeLists.txt)

openocd -s ~/.pico-sdk/openocd/0.12.0+dev/scripts -f interface/cmsis-dap.cfg -f target/$board_type.cfg -c "adapter speed 5000; program ../build/strip_driver.elf"
