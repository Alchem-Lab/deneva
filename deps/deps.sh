#!/bin/bash
trap "trap - ERR; return" ERR

librdma="librdma-0.9.0"
DENEVA_ROOT=${HOME}/git_repos/deneva/

write_log(){
    divider='-----------------------------'
    echo $divider >> install_deps.log
    date '+%Y-%m-%d %H:%M:%S' >> install_deps.log
    echo "on installing $1:" >> install_deps.log
}

install_librdma(){
    trap "return" ERR
    echo "Installing ${librdma}..."
    write_log "${librdma}"
    cd "$DENEVA_ROOT/deps"
    if [ ! -d "${librdma}-install" ]; then
        mkdir "${librdma}-install"
        cd "$DENEVA_ROOT/deps/${librdma}"
        trap - ERR
        ./configure --prefix="$DENEVA_ROOT/deps/${librdma}-install/" 2>>install_deps.log
        make 2>>install_deps.log
        make install 2>>install_deps.log
    else
        trap - ERR
        echo "found librdma."
    fi
    if [ $( echo "${CPATH}" | grep "${librdma}-install" | wc -l ) -eq 0 ]; then
        echo '# librdma configuration' >> ~/.bashrc
        echo "export CPATH=\$DENEVA_ROOT/deps/${librdma}-install/include:\$CPATH" >> ~/.bashrc
        echo "export LIBRARY_PATH=\$DENEVA_ROOT/deps/${librdma}-install/lib:\$LIBRARY_PATH" >> ~/.bashrc
        echo "export LD_LIBRARY_PATH=\$DENEVA_ROOT/deps/${librdma}-install/lib:\$LD_LIBRARY_PATH" >> ~/.bashrc
        source ~/.bashrc
    fi
}

# handle options
if [ $DENEVA_ROOT ]; then
    if [ "$1" == "clean" ]; then
        clean_deps "$@"
    else
#        install_mpi
#        install_boost
#        install_tbb
#        install_zeromq
#        install_hwloc
        if [ "$1" == "no-rdma" ]; then
            echo 'librdma will not be installed.'
        else
            install_librdma
        fi
    fi
    cd "$DENEVA_ROOT/deps"
    source ~/.bashrc
else
    echo 'Please set DENEVA_ROOT first.'
fi
trap - ERR

