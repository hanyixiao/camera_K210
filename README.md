# discrption
> 这是一个摄像头项目，

### 1.硬件
硬件系统为k210开发板Sipeed_MAIX_DOCK，摄像头为gc0328,lcd 分辨率为320*240:
![](img/maix_dock.jpg)
### 2.软件
软件是基于基于LicheeDan_K210_examples修改而成。
* 关于LicheeDan_K210_examples可以查询github仓库：
         https://github.com/Lichee-Pi/LicheeDan_K210_examples
* 基于基于LicheeDan_K210_examples/dvp_ov项目修改而成，其中gc0382驱动代码来自项目MAXPY
        https://github.com/sipeed/MaixPy
### 3.软件编译
* 安装编译工具 https://kendryte.com/downloads/
* 解压编译工具到　/opt
* 确保　ls /opt/kendryte-toolchain/bin 下存在下面的工具
* make                           riscv64-unknown-elf-g++         riscv64-unknown-elf-gcov-tool      riscv64-unknown-elf-objdump
riscv64-unknown-elf-addr2line ...
* 编译方法:
```shell
mkdir build && cd build
cmake .. -DPROJ=camera -DTOOLCHAIN=/opt/kendryte-toolchain/bin 
make
```
* 烧录方法：
* 安装kflash
```shell
pip3 install kflash
kflash --help
```
* 烧录
```shell
kflash -p /dev/ttyUSB0 -b 1500000 camera.bin
```
