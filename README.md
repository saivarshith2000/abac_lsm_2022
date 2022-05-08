# abac_lsm_2022
Attribute-based Access Control Kernel Components (with enhanced policy evaluation methods) and performance evaluation scripts

This repository contains the source code and installation instructions for ABAC Kernel Components and performance evaluation.
The following kernel components are available
1. ABAC LSM with Object-Specific Rule List
2. ABAC LSM with Encoded Object-Specific Rule List
3. ABAC LSM with Object-Specific PolTree
4. ABAC LSM with Encoded Object-Specific PolTree


# Installation
1. Install system dependencies for building the kernel
```bash
# On Debian Systems
sudo apt-get install build-essential libncurses-dev bison flex libssl-dev libelf-dev

# CentOS/RHEL/Oracle/Scientific Linux
sudo yum groupinstall "Development Tools"
sudo yum install ncurses-devel bison flex elfutils-libelf-devel openssl-devel

# On Fedora
sudo dnf group install "Development Tools"
sudo dnf install ncurses-devel bison flex elfutils-libelf-devel openssl-devel

# For other distributions, please refer to your distributions manual/documentation
```

2. Get the long term release version 5.10.96 from [here](https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-5.10.96.tar.gz) and extract it.
```bash
wget https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-5.10.96.tar.gz
tar xvzf linux-5.10.96.tar.gz
cd linux-5.10.96
```

3. Clone this repository in another terminal
```bash
https://github.com/saivarshith2000/abac_lsm.git`
cd abac_lsm
```

4. Patch the downloaded kernel with code from this repository
```bash
# In the cloned repository directory
cp -r security ../linux-5.10.96/
```

5. Configure
```bash
# In kernel source directory

# To generate initial config based on your system's architecture (x86_64 here)
x86_64_defconfig

# To enable ABAC LSM
make menuconfig
```
Select Security Options
![Select Security Options](images/s1.png "Select Security Options")

Make sure that security file system and path-based security modules are enabled
![Make sure that security file system and path-based security modules are enabled](images/s2.png "Make sure that security file system and path-based security modules are enabled")

Make sure that ABAC module is selected
![Make sure that ABAC module is selected](images/s3.png "Make sure that ABAC module is selected")


6. Build
```bash
# In kernel source directory
make
(or)
make -j$(nproc) # To use all the cores of your system for concurrency
```

7. Install
```bash
# In kernel source directory
sudo make modules_install
sudo make install
```

8. Update Bootloader
```bash
# For grub2 on Debian Systems
sudo update-initramfs -c -k 5.10.96
sudo update-grub

# For grub2 on CentOS/RHEL/Oracle/Scientific and Fedora Linux
sudo grub2-mkconfig -o /boot/grub2/grub.cfg
sudo grubby --set-default /boot/vmlinuz-5.10.96

# For other bootloaders, please refer to your bootloader manual/documentation
```

9. Verify ABAC LSM is enabled
```bash
# Check for ABAC messages in kernel logs
dmesg | grep ABAC 

# Check for ABAC securityFS directory
ls /sys/kernel/security/abac/

# Check for ABAC in the list of active LSMs
cat /sys/kernel/security/lsm
```

# Performance Evaluation
The `perf_eval` directory contains python scripts for generating datasets and running experiments on the LSM's performance. The steps for evaluation are outlined below - 
1. To generate datasets we need to specify a base ABAC config. The base configs we used in our experiments are located in `perf_eval/config` directory. Please make sure to follow the same format (same keys, JSON format) and only change the values if necessary.
2. Once the base configs are defined, we need to generate individual configs using the `perf_eval/generate_configs.py` script. It goes through all the base configs in the `configs/` directory and generates individual configs for each.
3. Next, we need to generate raw datasets based on the invidual configs generated in the previous step using the `perf_eval/generate_raw.py` script.
4. Once the raw datasets are generated, we need to convert them into kernel recognizable format. This can be done using the `perf_eval/main.py` script. Note that this script is a wrapper around the `perf_eval/generate_rule_abacfs.py` & `perf_eval/generate_tree_abacfs.py` scripts. It generates 4 datasets (for 4 evaluation methods) per individual dataset.
5. Now that all data is generated, we need to boot into ABAC enabled kernel and run the `perf_eval/perf_runner.py` script. It iterates through all the available datasets and in each iteration loads the dataset into kernel and measures access time from userspace and kernel space.
6. The `perf_eval/perf_runner.py` script used in the previous step invokes `perf_eval/perf.py` which uses `perf_counter_ns()` method to obtain timestamps. This time includes sleep time of the perf script. If you do not want sleep times to be included, modify `import perf.py` to `import perf_no_sleep.py` in `perf_eval/perf_runner.py`.
7. The above scripts generates results in JSON format and stores them in the `results/` directory (created automatically).
8. Most of the scripts mentioned above can be used individually if you do not want to generate all datasets.