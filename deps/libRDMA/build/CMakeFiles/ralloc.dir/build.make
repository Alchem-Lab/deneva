# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.7

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /home/chao/install/cmake-3.7.1-Linux-x86_64/bin/cmake

# The command to remove a file.
RM = /home/chao/install/cmake-3.7.1-Linux-x86_64/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/chao/git_repos/deneva/deps/libRDMA

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/chao/git_repos/deneva/deps/libRDMA/build

# Utility rule file for ralloc.

# Include the progress variables for this target.
include CMakeFiles/ralloc.dir/progress.make

CMakeFiles/ralloc: CMakeFiles/ralloc-complete


CMakeFiles/ralloc-complete: ralloc-prefix/src/ralloc-stamp/ralloc-install
CMakeFiles/ralloc-complete: ralloc-prefix/src/ralloc-stamp/ralloc-mkdir
CMakeFiles/ralloc-complete: ralloc-prefix/src/ralloc-stamp/ralloc-download
CMakeFiles/ralloc-complete: ralloc-prefix/src/ralloc-stamp/ralloc-update
CMakeFiles/ralloc-complete: ralloc-prefix/src/ralloc-stamp/ralloc-patch
CMakeFiles/ralloc-complete: ralloc-prefix/src/ralloc-stamp/ralloc-configure
CMakeFiles/ralloc-complete: ralloc-prefix/src/ralloc-stamp/ralloc-build
CMakeFiles/ralloc-complete: ralloc-prefix/src/ralloc-stamp/ralloc-install
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/home/chao/git_repos/deneva/deps/libRDMA/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Completed 'ralloc'"
	/home/chao/install/cmake-3.7.1-Linux-x86_64/bin/cmake -E make_directory /home/chao/git_repos/deneva/deps/libRDMA/build/CMakeFiles
	/home/chao/install/cmake-3.7.1-Linux-x86_64/bin/cmake -E touch /home/chao/git_repos/deneva/deps/libRDMA/build/CMakeFiles/ralloc-complete
	/home/chao/install/cmake-3.7.1-Linux-x86_64/bin/cmake -E touch /home/chao/git_repos/deneva/deps/libRDMA/build/ralloc-prefix/src/ralloc-stamp/ralloc-done

ralloc-prefix/src/ralloc-stamp/ralloc-install: ralloc-prefix/src/ralloc-stamp/ralloc-build
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/home/chao/git_repos/deneva/deps/libRDMA/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Performing install step for 'ralloc'"
	cd /home/chao/git_repos/deneva/deps/libRDMA/ralloc && make install
	cd /home/chao/git_repos/deneva/deps/libRDMA/ralloc && /home/chao/install/cmake-3.7.1-Linux-x86_64/bin/cmake -E touch /home/chao/git_repos/deneva/deps/libRDMA/build/ralloc-prefix/src/ralloc-stamp/ralloc-install

ralloc-prefix/src/ralloc-stamp/ralloc-mkdir:
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/home/chao/git_repos/deneva/deps/libRDMA/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_3) "Creating directories for 'ralloc'"
	/home/chao/install/cmake-3.7.1-Linux-x86_64/bin/cmake -E make_directory /home/chao/git_repos/deneva/deps/libRDMA/ralloc
	/home/chao/install/cmake-3.7.1-Linux-x86_64/bin/cmake -E make_directory /home/chao/git_repos/deneva/deps/libRDMA/ralloc
	/home/chao/install/cmake-3.7.1-Linux-x86_64/bin/cmake -E make_directory /home/chao/git_repos/deneva/deps/libRDMA/build/ralloc-prefix
	/home/chao/install/cmake-3.7.1-Linux-x86_64/bin/cmake -E make_directory /home/chao/git_repos/deneva/deps/libRDMA/build/ralloc-prefix/tmp
	/home/chao/install/cmake-3.7.1-Linux-x86_64/bin/cmake -E make_directory /home/chao/git_repos/deneva/deps/libRDMA/build/ralloc-prefix/src/ralloc-stamp
	/home/chao/install/cmake-3.7.1-Linux-x86_64/bin/cmake -E make_directory /home/chao/git_repos/deneva/deps/libRDMA/build/ralloc-prefix/src
	/home/chao/install/cmake-3.7.1-Linux-x86_64/bin/cmake -E touch /home/chao/git_repos/deneva/deps/libRDMA/build/ralloc-prefix/src/ralloc-stamp/ralloc-mkdir

ralloc-prefix/src/ralloc-stamp/ralloc-download: ralloc-prefix/src/ralloc-stamp/ralloc-mkdir
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/home/chao/git_repos/deneva/deps/libRDMA/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_4) "No download step for 'ralloc'"
	/home/chao/install/cmake-3.7.1-Linux-x86_64/bin/cmake -E echo_append
	/home/chao/install/cmake-3.7.1-Linux-x86_64/bin/cmake -E touch /home/chao/git_repos/deneva/deps/libRDMA/build/ralloc-prefix/src/ralloc-stamp/ralloc-download

ralloc-prefix/src/ralloc-stamp/ralloc-update: ralloc-prefix/src/ralloc-stamp/ralloc-download
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/home/chao/git_repos/deneva/deps/libRDMA/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_5) "No update step for 'ralloc'"
	/home/chao/install/cmake-3.7.1-Linux-x86_64/bin/cmake -E echo_append
	/home/chao/install/cmake-3.7.1-Linux-x86_64/bin/cmake -E touch /home/chao/git_repos/deneva/deps/libRDMA/build/ralloc-prefix/src/ralloc-stamp/ralloc-update

ralloc-prefix/src/ralloc-stamp/ralloc-patch: ralloc-prefix/src/ralloc-stamp/ralloc-download
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/home/chao/git_repos/deneva/deps/libRDMA/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_6) "No patch step for 'ralloc'"
	/home/chao/install/cmake-3.7.1-Linux-x86_64/bin/cmake -E echo_append
	/home/chao/install/cmake-3.7.1-Linux-x86_64/bin/cmake -E touch /home/chao/git_repos/deneva/deps/libRDMA/build/ralloc-prefix/src/ralloc-stamp/ralloc-patch

ralloc-prefix/src/ralloc-stamp/ralloc-configure: ralloc-prefix/tmp/ralloc-cfgcmd.txt
ralloc-prefix/src/ralloc-stamp/ralloc-configure: ralloc-prefix/src/ralloc-stamp/ralloc-update
ralloc-prefix/src/ralloc-stamp/ralloc-configure: ralloc-prefix/src/ralloc-stamp/ralloc-patch
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/home/chao/git_repos/deneva/deps/libRDMA/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_7) "Performing configure step for 'ralloc'"
	cd /home/chao/git_repos/deneva/deps/libRDMA/ralloc && mkdir -p /home/chao/git_repos/deneva/deps/libRDMA/lib
	cd /home/chao/git_repos/deneva/deps/libRDMA/ralloc && /home/chao/install/cmake-3.7.1-Linux-x86_64/bin/cmake -E touch /home/chao/git_repos/deneva/deps/libRDMA/build/ralloc-prefix/src/ralloc-stamp/ralloc-configure

ralloc-prefix/src/ralloc-stamp/ralloc-build: ralloc-prefix/src/ralloc-stamp/ralloc-configure
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/home/chao/git_repos/deneva/deps/libRDMA/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_8) "Performing build step for 'ralloc'"
	cd /home/chao/git_repos/deneva/deps/libRDMA/ralloc && make
	cd /home/chao/git_repos/deneva/deps/libRDMA/ralloc && /home/chao/install/cmake-3.7.1-Linux-x86_64/bin/cmake -E touch /home/chao/git_repos/deneva/deps/libRDMA/build/ralloc-prefix/src/ralloc-stamp/ralloc-build

ralloc: CMakeFiles/ralloc
ralloc: CMakeFiles/ralloc-complete
ralloc: ralloc-prefix/src/ralloc-stamp/ralloc-install
ralloc: ralloc-prefix/src/ralloc-stamp/ralloc-mkdir
ralloc: ralloc-prefix/src/ralloc-stamp/ralloc-download
ralloc: ralloc-prefix/src/ralloc-stamp/ralloc-update
ralloc: ralloc-prefix/src/ralloc-stamp/ralloc-patch
ralloc: ralloc-prefix/src/ralloc-stamp/ralloc-configure
ralloc: ralloc-prefix/src/ralloc-stamp/ralloc-build
ralloc: CMakeFiles/ralloc.dir/build.make

.PHONY : ralloc

# Rule to build all files generated by this target.
CMakeFiles/ralloc.dir/build: ralloc

.PHONY : CMakeFiles/ralloc.dir/build

CMakeFiles/ralloc.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/ralloc.dir/cmake_clean.cmake
.PHONY : CMakeFiles/ralloc.dir/clean

CMakeFiles/ralloc.dir/depend:
	cd /home/chao/git_repos/deneva/deps/libRDMA/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/chao/git_repos/deneva/deps/libRDMA /home/chao/git_repos/deneva/deps/libRDMA /home/chao/git_repos/deneva/deps/libRDMA/build /home/chao/git_repos/deneva/deps/libRDMA/build /home/chao/git_repos/deneva/deps/libRDMA/build/CMakeFiles/ralloc.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/ralloc.dir/depend
