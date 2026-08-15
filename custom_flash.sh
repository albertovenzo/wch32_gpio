# Use Moun River openocd
$MR_OPENOCD/openocd -f $MR_OPENOCD/wch-riscv.cfg -c "program build/zephyr/zephyr.bin verify reset exit"
#$MR_OPENOCD/openocd -f $MR_OPENOCD/wch-riscv-zephyr.cfg -c "program build/zephyr/zephyr.elf verify reset exit"
