rm -rf unittests_* *.gcda *.gcno
echo Building unittests_coverage ...
g++-4.8 -std=c++0x -O1 -DNDEBUG -DRUNNING_ON_VALGRIND=1 -g -gdwarf-2 -o unittests_coverage unittests.cpp -lrt -fprofile-arcs -ftest-coverage -fno-omit-frame-pointer -fno-optimize-sibling-calls -fno-elide-constructors -fno-inline
echo Building unittests_valgrind ...
g++-4.8 -std=c++0x -O1 -DNDEBUG -g -gdwarf-2 -o unittests_valgrind unittests.cpp -lrt -fno-omit-frame-pointer -fno-optimize-sibling-calls -fno-elide-constructors -fno-inline -DBOOST_SPINLOCK_ENABLE_VALGRIND
echo Building unittests_1 ...
g++-4.8 -std=c++0x -O3 -DNDEBUG -g -gdwarf-2 -o unittests_1 unittests.cpp -lrt
echo Building unittests_N ...
g++-4.8 -std=c++0x -O3 -DNDEBUG -g -fopenmp -gdwarf-2 -o unittests_N unittests.cpp -lrt
echo Building unittests_stm_1 ...
g++-4.9 -std=c++0x -O3 -DNDEBUG -g -fgnu-tm -DBOOST_HAVE_TRANSACTIONAL_MEMORY_COMPILER -o unittests_stm_1 unittests.cpp -lrt
echo Building unittests_stm_N ...
g++-4.9 -std=c++0x -O3 -DNDEBUG -g -fopenmp -fgnu-tm -DBOOST_HAVE_TRANSACTIONAL_MEMORY_COMPILER -o unittests_stm_N unittests.cpp -lrt
echo Building unittests_sanitise ...
#g++-4.8 -std=c++0x -fsanitize=thread -fPIC -pie -O0 -DNDEBUG -g -fopenmp -gdwarf-2 -o unittests_sanitise unittests.cpp -lrt -ltsan -fno-omit-frame-pointer -fno-optimize-sibling-calls -fno-elide-constructors
#clang++-3.4 -std=c++0x -fsanitize=thread -O2 -DNDEBUG -g -o unittests_sanitise unittests.cpp -lrt -fno-omit-frame-pointer -fno-optimize-sibling-calls -fno-elide-constructors -fno-inline
g++-4.9 -std=c++0x -fsanitize=undefined -fsanitize=thread -fPIC -pie -O1 -DNDEBUG -DRUNNING_ON_VALGRIND=1 -g -o unittests_sanitise unittests.cpp -lrt -ltsan -fno-omit-frame-pointer -fno-optimize-sibling-calls -fno-elide-constructors -fno-inline