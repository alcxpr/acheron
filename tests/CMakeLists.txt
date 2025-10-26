# this file is part of the Acheron library project, licensed under the MIT license. see `LICENSE.txt` for details

acheron_test(algorithm-test
        SOURCES algorithm.cc
        LIBS Acheron::Acheron
        FLAGS -fsanitize=undefined -fno-omit-frame-pointer
)

acheron_test(allocator-test
        SOURCES allocator.cc
        LIBS Acheron::Acheron
        FLAGS -fsanitize=address,undefined -fno-omit-frame-pointer
)

acheron_test(bitfield-test
        SOURCES bitfield.cc
        LIBS Acheron::Acheron
)

acheron_test(defer-test
        SOURCES defer.cc
        LIBS Acheron::Acheron
)