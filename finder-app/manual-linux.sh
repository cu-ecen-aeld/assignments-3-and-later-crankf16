#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # TODO: Add your kernel build steps here - FINISHED

    echo "Begin kernerl build steps"
    
    # build - clean - Building the Linux Kernel Slide 12
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- mrproper
    echo "Finished Clean"
    # build - defconfig Slide 13
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- defconfig
    echo "Finished Defconfig"
    # build - vmlinux Slide 14
    make -j4 ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- all
   echo "Finished Vmlinux" 

fi

echo "Adding the Image in outdir"

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories - FINISHED
cd "$OUTDIR"
mkdir -p rootfs
cd rootfs
# From Linux Root Filesystems Slide 11
mkdir -p bin dev etc home lib lib64 proc sbin sys tmp usr var
mkdir -p usr/bin usr/lib usr/sbin
mkdir -p var/log

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}

    # TODO:  Configure busybox - Linux Root Filesystems Slide 14 - FINISHED
    make distclean
    make defconfig
    echo "Finished Busybox Configuration"
else
    cd busybox
fi

# TODO: Make and install busybox - Linux Root Filesystems Slide 14 - FINISHED
make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu-
make CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- install
echo "Finished Busybox Make & Install"

echo "Library dependencies"
${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "Shared library"

# TODO: Add library dependencies to rootfs - Linux Root Filesystems Slide 15 - Finished
# cp  -r /toolchain/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/lib/     ${OUTDIR}/rootfs
# cp  -r /toolchain/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/lib64/   ${OUTDIR}/rootfs
cd $(pwd)
cp -r ./lib/* ${OUTDIR}/rootfs/lib/
cp -r ./lib64/* %{OUTDIR}/rootfs/lib64/
echo "Dependencies added to rootfs"

# TODO: Make device nodes - Linux Root Filesystems Slide 16 - Finished
sudo mknod -m 666 ${OUTDIR}/rootfs/dev/null c 1 3
sudo mknod -m 666 ${OUTDIR}/rootfs/dev/console c 5 1
echo "Finished making device nodes"

# TODO: Clean and build the writer utility - Embedded Linux Toolchain Slide 20 - Finished
cd ${OUTDIR}/finder
make clean
make CROSS_COMPILE=aarch64-none-linux-gnu-
echo "Finished clean & build of writer utility"


# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
cp writer ${OUTDIR}/rootfs/home/
cp finde*.sh ${OUTDIR}/rootfs/home/
mkdir -p ${OUTDIR}/rootfs/home/conf
cp conf/*.txt ${OUTDIR}/rootfs/home/conf/
cp autorun-qemu.sh ${OUTDIR}/rootfs/home/
echo "Finished file copy"

# TODO: Chown the root directory - Linux Root Filesystem Slide 17 - Finished
cd ${OUTDIR}/rootfs
sudo chown -R root ${OUTDIR}/rootfs/
echo "Finished chown"

# TODO: Create initramfs.cpio.gz - Finished
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
cd ..
gzip -f initramfs.cpio
cp initramfs.cpio.gz ${OUTDIR}/Image/
echo "All done!!!"
