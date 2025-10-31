UNIX BUILD NOTES
====================
Some notes on how to build Bitcoin Classic in Unix.

(for OpenBSD specific instructions, see [build-openbsd.md](build-openbsd.md))

Note
---------------------
Always use absolute paths to configure and compile bitcoin and the dependencies,
for example, when specifying the path of the dependency:

        ../dist/configure --enable-cxx --disable-shared --with-pic --prefix=$BDB_PREFIX

Here BDB_PREFIX must be an absolute path - it is defined using $(pwd) which ensures
the usage of the absolute path.

Recommended Build Environment (DevContainer)
---------------------

For reproducible and portable builds, Bitcoin Classic uses a DevContainer environment. This ensures consistent compilation across different systems and produces portable executables.

### Using DevContainer (Recommended)

1. **Prerequisites:**
   - Visual Studio Code with Remote-Containers extension
   - Docker installed and running

2. **Setup:**
   ```bash
   git clone https://github.com/Bitcoinclassicxbt/classic2.git
   cd classic2
   code .  # Open in VS Code
   # When prompted, click "Reopen in Container"
   ```

3. **Build in DevContainer:**
   The devcontainer provides a controlled Ubuntu environment with all dependencies pre-installed, ensuring portable executables.

Quick Start Build Instructions
---------------------

### For Linux (x86_64) - DevContainer Environment

```bash
# Open DevContainer in VS Code, then in terminal:

# Build dependencies
cd depends
make HOST=x86_64-pc-linux-gnu -j$(nproc)

# Build Bitcoin Classic
cd ..
./autogen.sh 
./configure --prefix=$(pwd)/depends/x86_64-pc-linux-gnu \
    CXXFLAGS="-Wno-deprecated-declarations -Wno-placement-new -O2" \
    CFLAGS="-Wno-deprecated-declarations -Wno-placement-new -O2"

make -j$(nproc)
```

### For Windows (x86_64) Cross-compilation - DevContainer Environment

```bash
# In DevContainer terminal:

# Build dependencies
cd depends
make HOST=x86_64-w64-mingw32 -j$(nproc)

# Build Bitcoin Classic
cd ..
./autogen.sh
./configure --prefix=$(pwd)/depends/x86_64-w64-mingw32

make -j$(nproc)
```

### For Local Development (Alternative)

If not using DevContainer, you can build locally, but results may vary depending on your system configuration:

### Standard Build (Alternative)

```bash
./autogen.sh
./configure
make
make install # optional
```

This will build bitcoin-qt as well if the dependencies are met.

Dependencies
---------------------

These dependencies are required:

 Library     | Purpose          | Description
 ------------|------------------|----------------------
 libssl      | Crypto           | Random Number Generation, Elliptic Curve Cryptography
 libboost    | Utility          | Library for threading, data structures, etc
 libevent    | Networking       | OS independent asynchronous networking

Optional dependencies:

 Library     | Purpose          | Description
 ------------|------------------|----------------------
 miniupnpc   | UPnP Support     | Firewall-jumping support
 libdb4.8    | Berkeley DB      | Wallet storage (only needed when wallet enabled)
 qt          | GUI              | GUI toolkit (only needed when GUI enabled)
 protobuf    | Payments in GUI  | Data interchange format used for payment protocol (only needed when GUI enabled)
 libqrencode | QR codes in GUI  | Optional for generating QR codes (only needed when GUI enabled)
 univalue    | Utility          | JSON parsing and encoding (bundled version will be used unless --with-system-univalue passed to configure)
 libzmq3     | ZMQ notification | Optional, allows generating ZMQ notifications (requires ZMQ version >= 4.x)

For the versions used in the release, see [release-process.md](release-process.md) under *Fetch and build inputs*.

Bitcoin Classic Specific Notes
---------------------

### DevContainer Benefits
- **Reproducible Builds**: Same environment every time, regardless of host OS
- **Portable Executables**: Compiled in controlled environment for maximum compatibility
- **Pre-configured Dependencies**: All build tools and libraries pre-installed
- **Cross-platform**: Works on Windows, macOS, and Linux hosts
- **Isolation**: No conflicts with host system packages

### Compiler Warnings Fix
Bitcoin Classic includes fixes for common compiler warnings when building with newer GCC versions:

- `-Wno-deprecated-declarations`: Suppresses Boost auto_ptr deprecation warnings
- `-Wno-placement-new`: Fixes placement new warnings in Boost libraries
- `-O2`: Optimization level for release builds

### DevContainer Configuration
The `.devcontainer/devcontainer.json` configures:
- Ubuntu-based container with build essentials
- Cross-compilation toolchains for Windows
- All required dependencies pre-installed
- Optimized for Bitcoin Classic builds

### Windows Cross-compilation Prerequisites
The DevContainer includes pre-installed Windows cross-compilation tools. For local builds without DevContainer, install:

```bash
# Ubuntu/Debian
sudo apt-get install g++-mingw-w64-x86-64 mingw-w64-x86-64-dev

# Fedora
sudo dnf install mingw64-gcc-c++
```

### Creating Release Packages

After building, you can create release packages:

```bash
# From the src directory
cd src

# Create Bitcoin Classic release packages
mkdir -p bitcoin-classic-0.14.0-linux-x86_64
cp bitcoind bitcoin-cli bitcoin-tx qt/bitcoin-qt bitcoin-classic-0.14.0-linux-x86_64/
tar -czf bitcoin-classic-0.14.0-linux-x86_64.tar.gz bitcoin-classic-0.14.0-linux-x86_64/

# For Windows (if cross-compiled)
mkdir -p bitcoin-classic-0.14.0-windows-x86_64
cp bitcoind.exe bitcoin-cli.exe bitcoin-tx.exe qt/bitcoin-qt.exe bitcoin-classic-0.14.0-windows-x86_64/
zip -r bitcoin-classic-0.14.0-windows-x86_64.zip bitcoin-classic-0.14.0-windows-x86_64/
```

Memory Requirements
--------------------

C++ compilers are memory-hungry. It is recommended to have at least 1.5 GB of
memory available when compiling Bitcoin Classic. On systems with less, gcc can be
tuned to conserve memory with additional CXXFLAGS:

    ./configure CXXFLAGS="--param ggc-min-expand=1 --param ggc-min-heapsize=32768"

Dependency Build Instructions: Ubuntu & Debian
----------------------------------------------
Build requirements:

    sudo apt-get install build-essential libtool autotools-dev automake pkg-config libssl-dev libevent-dev bsdmainutils

For cross-compilation to Windows:

    sudo apt-get install g++-mingw-w64-x86-64 mingw-w64-x86-64-dev

Options when installing required Boost library files:

1. On at least Ubuntu 14.04+ and Debian 7+ there are generic names for the
individual boost development packages, so the following can be used to only
install necessary parts of boost:

        sudo apt-get install libboost-system-dev libboost-filesystem-dev libboost-chrono-dev libboost-program-options-dev libboost-test-dev libboost-thread-dev

2. If that doesn't work, you can install all boost development packages with:

        sudo apt-get install libboost-all-dev

BerkeleyDB is required for the wallet. db4.8 packages are available [here](https://launchpad.net/~bitcoin/+archive/bitcoin).
You can add the repository and install using the following commands:

    sudo apt-get install software-properties-common
    sudo add-apt-repository ppa:bitcoin/bitcoin
    sudo apt-get update
    sudo apt-get install libdb4.8-dev libdb4.8++-dev

Ubuntu and Debian have their own libdb-dev and libdb++-dev packages, but these will install
BerkeleyDB 5.1 or later, which break binary wallet compatibility with the distributed executables which
are based on BerkeleyDB 4.8. If you do not care about wallet compatibility,
pass `--with-incompatible-bdb` to configure.

See the section "Disable-wallet mode" to build Bitcoin Classic without wallet.

Optional:

    sudo apt-get install libminiupnpc-dev (see --with-miniupnpc and --enable-upnp-default)

ZMQ dependencies:

    sudo apt-get install libzmq3-dev (provides ZMQ API 4.x)

Dependencies for the GUI: Ubuntu & Debian
-----------------------------------------

If you want to build Bitcoin-Qt, make sure that the required packages for Qt development
are installed. Either Qt 5 or Qt 4 are necessary to build the GUI.
If both Qt 4 and Qt 5 are installed, Qt 5 will be used. Pass `--with-gui=qt4` to configure to choose Qt4.
To build without GUI pass `--without-gui`.

To build with Qt 5 (recommended) you need the following:

    sudo apt-get install libqt5gui5 libqt5core5a libqt5dbus5 qttools5-dev qttools5-dev-tools libprotobuf-dev protobuf-compiler

Alternatively, to build with Qt 4 you need the following:

    sudo apt-get install libqt4-dev libprotobuf-dev protobuf-compiler

libqrencode (optional) can be installed with:

    sudo apt-get install libqrencode-dev

Once these are installed, they will be found by configure and a bitcoin-qt executable will be
built by default.

Dependency Build Instructions: Fedora
-------------------------------------
Build requirements:

    sudo dnf install gcc-c++ libtool make autoconf automake openssl-devel libevent-devel boost-devel libdb4-devel libdb4-cxx-devel

For cross-compilation to Windows:

    sudo dnf install mingw64-gcc-c++

Optional:

    sudo dnf install miniupnpc-devel

To build with Qt 5 (recommended) you need the following:

    sudo dnf install qt5-qttools-devel qt5-qtbase-devel protobuf-devel

libqrencode (optional) can be installed with:

    sudo dnf install qrencode-devel

Notes
-----
The release is built with GCC and then "strip bitcoind" to strip the debug
symbols, which reduces the executable size by about 90%.

miniupnpc
---------

[miniupnpc](http://miniupnp.free.fr/) may be used for UPnP port mapping.  It can be downloaded from [here](
http://miniupnp.tuxfamily.org/files/).  UPnP support is compiled in and
turned off by default.  See the configure options for upnp behavior desired:

        --without-miniupnpc      No UPnP support miniupnp not required
        --disable-upnp-default   (the default) UPnP support turned off by default at runtime
        --enable-upnp-default    UPnP support turned on by default at runtime

Berkeley DB
-----------
It is recommended to use Berkeley DB 4.8. If you have to build it yourself:

```bash
BITCOIN_ROOT=$(pwd)

# Pick some path to install BDB to, here we create a directory within the bitcoin directory
BDB_PREFIX="${BITCOIN_ROOT}/db4"
mkdir -p $BDB_PREFIX

# Fetch the source and verify that it is not tampered with
wget 'http://download.oracle.com/berkeley-db/db-4.8.30.NC.tar.gz'
echo '12edc0df75bf9abd7f82f821795bcee50f42cb2e5f76a6a281b85732798364ef  db-4.8.30.NC.tar.gz' | sha256sum -c
# -> db-4.8.30.NC.tar.gz: OK
tar -xzvf db-4.8.30.NC.tar.gz

# Build the library and install to our prefix
cd db-4.8.30.NC/build_unix/
#  Note: Do a static build so that it can be embedded into the executable, instead of having to find a .so at runtime
../dist/configure --enable-cxx --disable-shared --with-pic --prefix=$BDB_PREFIX
make install

# Configure Bitcoin Classic to use our own-built instance of BDB
cd $BITCOIN_ROOT
./autogen.sh
./configure LDFLAGS="-L${BDB_PREFIX}/lib/" CPPFLAGS="-I${BDB_PREFIX}/include/" # (other args...)
```

**Note**: You only need Berkeley DB if the wallet is enabled (see the section *Disable-Wallet mode* below).

Boost
-----
If you need to build Boost yourself:

        sudo su
        ./bootstrap.sh
        ./bjam install

Security
--------
To help make your Bitcoin Classic installation more secure by making certain attacks impossible to
exploit even if a vulnerability is found, binaries are hardened by default.
This can be disabled with:

Hardening Flags:

        ./configure --enable-hardening
        ./configure --disable-hardening

Hardening enables the following features:

* Position Independent Executable
    Build position independent code to take advantage of Address Space Layout Randomization
    offered by some kernels. Attackers who can cause execution of code at an arbitrary memory
    location are thwarted if they don't know where anything useful is located.
    The stack and heap are randomly located by default but this allows the code section to be
    randomly located as well.

    On an AMD64 processor where a library was not compiled with -fPIC, this will cause an error
    such as: "relocation R_X86_64_32 against `......' can not be used when making a shared object;"

    To test that you have built PIE executable, install scanelf, part of paxutils, and use:

        scanelf -e ./bitcoin

    The output should contain:

     TYPE
    ET_DYN

* Non-executable Stack
    If the stack is executable then trivial stack based buffer overflow exploits are possible if
    vulnerable buffers are found. By default, Bitcoin Classic should be built with a non-executable stack
    but if one of the libraries it uses asks for an executable stack or someone makes a mistake
    and uses a compiler extension which requires an executable stack, it will silently build an
    executable without the non-executable stack protection.

    To verify that the stack is non-executable after compiling use:
    `scanelf -e ./bitcoin`

    the output should contain:
        STK/REL/PTL
        RW- R-- RW-

    The STK RW- means that the stack is readable and writeable but not executable.

Disable-wallet mode
--------------------
When the intention is to run only a P2P node without a wallet, Bitcoin Classic may be compiled in
disable-wallet mode with:

    ./configure --disable-wallet

In this case there is no dependency on Berkeley DB 4.8.

Mining is also possible in disable-wallet mode, but only using the `getblocktemplate` RPC
call not `getwork`.

Additional Configure Flags
--------------------------
A list of additional configure flags can be displayed with:

    ./configure --help

Setup and Build Example: Arch Linux
-----------------------------------
This example lists the steps necessary to setup and build a command line only, non-wallet distribution of the latest changes on Arch Linux:

    pacman -S git base-devel boost libevent python
    git clone https://github.com/Bitcoinclassicxbt/classic2.git
    cd classic2/
    ./autogen.sh
    ./configure --disable-wallet --without-gui --without-miniupnpc
    make check

Note:
Enabling wallet support requires either compiling against a Berkeley DB newer than 4.8 (package `db`) using `--with-incompatible-bdb`,
or building and depending on a local version of Berkeley DB 4.8. The readily available Arch Linux packages are currently built using
`--with-incompatible-bdb` according to the [PKGBUILD](https://projects.archlinux.org/svntogit/community.git/tree/bitcoin/trunk/PKGBUILD).
As mentioned above, when maintaining portability of the wallet between the standard Bitcoin Core distributions and independently built
node software is desired, Berkeley DB 4.8 must be used.

ARM Cross-compilation
-------------------
These steps can be performed on, for example, an Ubuntu VM. The depends system
will also work on other Linux distributions, however the commands for
installing the toolchain will be different.

Make sure you install the build requirements mentioned above.
Then, install the toolchain and curl:

    sudo apt-get install g++-arm-linux-gnueabihf curl

To build executables for ARM:

    cd depends
    make HOST=arm-linux-gnueabihf NO_QT=1
    cd ..
    ./configure --prefix=$PWD/depends/arm-linux-gnueabihf --enable-glibc-back-compat --enable-reduce-exports LDFLAGS=-static-libstdc++
    make

For further documentation on the depends system see [README.md](../depends/README.md) in the depends directory.

Troubleshooting
---------------

### Troubleshooting

1. **DevContainer Issues**:
   - Ensure Docker is running and has sufficient resources (4GB+ RAM recommended)
   - Try rebuilding the container: `Ctrl+Shift+P` → "Remote-Containers: Rebuild Container"
   - Check Docker permissions on Linux: `sudo usermod -aG docker $USER` (logout/login required)

2. **Build Issues**:
   - **Boost auto_ptr warnings**: Use the provided CXXFLAGS with `-Wno-deprecated-declarations`
   - **Placement new warnings**: Use the provided CXXFLAGS with `-Wno-placement-new` 
   - **Missing fontconfig patch**: The build system will automatically create the required empty patch file
   - **Memory issues**: Use `-j1` instead of `-j$(nproc)` for single-threaded builds on low-memory systems

3. **DevContainer vs Local Builds**:
   - DevContainer builds are more reproducible and portable
   - Local builds may have different results depending on system configuration
   - For release builds, always use DevContainer environment

### Environment Details
- **DevContainer Base**: Ubuntu 20.04 LTS
- **Compiler**: GCC 9.4+
- **Cross-compiler**: MinGW-w64 for Windows builds
- **Dependencies**: All pre-installed and version-locked for consistency

### Getting Help

- GitHub Issues: https://github.com/Bitcoinclassicxbt/classic2/issues
- Website: https://www.classicxbt.com
