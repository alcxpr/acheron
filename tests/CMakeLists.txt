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

acheron_executable(argument-test
        SOURCES argument.cc
        LIBS Acheron::Acheron
)

acheron_test(bitfield-test
        SOURCES bitfield.cc
        LIBS Acheron::Acheron
)

acheron_test(codecvt.cc
        SOURCES codecvt.cc
        LIBS Acheron::Acheron
)

acheron_test(cstring_view-test
        SOURCES cstring_view.cc
        LIBS Acheron::Acheron
)

acheron_test(defer-test
        SOURCES defer.cc
        LIBS Acheron::Acheron
)

acheron_executable(diagnostic-test
        SOURCES diagnostic.cc
        LIBS Acheron::Acheron
)

acheron_test(freelist-test
        SOURCES freelist.cc
        LIBS Acheron::Acheron
)

acheron_test(utility-test
        SOURCES utility.cc
        LIBS Acheron::Acheron
        FLAGS -fsanitize=thread,undefined -fno-omit-frame-pointer
)

acheron_test(variant-test
        SOURCES variant.cc
        LIBS Acheron::Acheron
)
